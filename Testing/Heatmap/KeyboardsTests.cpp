#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>

#include "Heatmap/Keyboards.h"
#include "Heatmap/KeyStats.h"

using namespace heatmap;

// ---------------------------------------------------------------------------
// Геометрия
// ---------------------------------------------------------------------------

TEST(GeometryTest, RotationIsClockwiseLikeKle) {
  // Ось y направлена вниз: поворот на +90° переводит «вправо» в «вниз»
  Point p = rotatePoint({2, 1}, 90, 1, 1);
  EXPECT_NEAR(p.x, 1, 1e-9);
  EXPECT_NEAR(p.y, 2, 1e-9);
  Point same = rotatePoint({5, 7}, 0, 1, 1);
  EXPECT_DOUBLE_EQ(same.x, 5);
  EXPECT_DOUBLE_EQ(same.y, 7);
}

TEST(GeometryTest, RotatedKeyHitTest) {
  // Клавиша 1x1 в (0, 0), повёрнутая на 45° вокруг своего центра: ромб
  KeyDef key{KC_A, 0, 0, 1, 1, 45, 0.5f, 0.5f};
  EXPECT_TRUE(pointInKey(key, {0.5, 0.5}));
  EXPECT_TRUE(pointInKey(key, {0.5, -0.15})); // верхняя вершина ромба выше y=0
  EXPECT_FALSE(pointInKey(key, {0.05, 0.05})); // угол исходного квадрата
  Point c = keyCenter(key);
  EXPECT_NEAR(c.x, 0.5, 1e-9);
  EXPECT_NEAR(c.y, 0.5, 1e-9);
}

TEST(GeometryTest, SecondaryRectLikeIsoEnter) {
  KeyDef enter{KC_ENT, 13.75f, 1, 1.25f, 2, 0, 0, 0, nullptr, -0.25f, 0, 1.5f, 1};
  ASSERT_EQ(keyQuads(enter).size(), 2u);
  EXPECT_TRUE(pointInKey(enter, {13.6, 1.5}));  // верхняя широкая часть
  EXPECT_FALSE(pointInKey(enter, {13.6, 2.5})); // под ней — клавиша слева
  EXPECT_TRUE(pointInKey(enter, {14.5, 2.5}));
}

// ---------------------------------------------------------------------------
// Общие проверки для каждой клавиатуры
// ---------------------------------------------------------------------------

namespace {

struct Rect {
  double x0, y0, x1, y1;
};

std::vector<Rect> rectsOf(const KeyDef &key) {
  std::vector<Rect> rects;
  for (const Quad &q : keyQuads(key)) {
    Rect r{q[0].x, q[0].y, q[0].x, q[0].y};
    for (const Point &p : q) {
      r.x0 = std::min(r.x0, p.x);
      r.y0 = std::min(r.y0, p.y);
      r.x1 = std::max(r.x1, p.x);
      r.y1 = std::max(r.y1, p.y);
    }
    rects.push_back(r);
  }
  return rects;
}

bool overlaps(const Rect &a, const Rect &b) {
  const double eps = 1e-3; // касание краями не считается пересечением
  return a.x0 < b.x1 - eps && b.x0 < a.x1 - eps && a.y0 < b.y1 - eps &&
         b.y0 < a.y1 - eps;
}

std::string describe(const KeyDef &key) {
  std::ostringstream ss;
  ss << keyCodeName(key.code) << " '" << labelOf(key) << "' at (" << key.x
     << ", " << key.y << ")";
  return ss.str();
}

} // namespace

class KeyboardLayoutTest
    : public ::testing::TestWithParam<const KeyboardLayout *> {};

TEST_P(KeyboardLayoutTest, HasMetadata) {
  const KeyboardLayout &layout = *GetParam();
  EXPECT_NE(std::string(layout.id), "");
  EXPECT_NE(std::string(layout.name), "");
  EXPECT_NE(std::string(layout.source), "") << "У таблицы должен быть источник";
  EXPECT_FALSE(layout.keys.empty());
  EXPECT_EQ(findKeyboard(layout.id), &layout);
}

TEST_P(KeyboardLayoutTest, AllKeysHaveValidLogicalId) {
  const KeyboardLayout &layout = *GetParam();
  for (const KeyDef &key : layout.keys) {
    EXPECT_TRUE(isValidKeyCode(key.code)) << describe(key);
    EXPECT_NE(std::string(keyCodeName(key.code)), "") << describe(key);
    EXPECT_NE(labelOf(key), nullptr);
    if (key.code == KC_LAYER) {
      // У переключателя слоя подпись должна говорить, какой это слой
      EXPECT_NE(key.label, nullptr) << describe(key);
    }
    EXPECT_GT(key.w, 0) << describe(key);
    EXPECT_GT(key.h, 0) << describe(key);
  }
}

TEST_P(KeyboardLayoutTest, UnrotatedKeysDoNotOverlap) {
  const KeyboardLayout &layout = *GetParam();
  const auto &keys = layout.keys;
  for (size_t i = 0; i < keys.size(); ++i) {
    if (keys[i].r != 0) {
      continue;
    }
    for (size_t j = i + 1; j < keys.size(); ++j) {
      if (keys[j].r != 0) {
        continue;
      }
      for (const Rect &a : rectsOf(keys[i])) {
        for (const Rect &b : rectsOf(keys[j])) {
          EXPECT_FALSE(overlaps(a, b))
              << layout.id << ": " << describe(keys[i]) << " overlaps "
              << describe(keys[j]);
        }
      }
    }
  }
}

TEST_P(KeyboardLayoutTest, BoundsStartNearOrigin) {
  const KeyboardLayout &layout = *GetParam();
  Bounds b = layoutBounds(layout);
  EXPECT_GT(b.width(), 1.0);
  EXPECT_GT(b.height(), 1.0);
  EXPECT_GT(b.minX, -2.0);
  EXPECT_GT(b.minY, -2.0);
}

TEST_P(KeyboardLayoutTest, SplitsDescribeTheirKeymap) {
  const KeyboardLayout &layout = *GetParam();
  if (layout.isSplit) {
    EXPECT_NE(std::string(layout.source).find("keymaps/default"),
              std::string::npos)
        << "Для сплита источник должен называть default keymap";
  }
}

INSTANTIATE_TEST_SUITE_P(
    AllKeyboards, KeyboardLayoutTest, ::testing::ValuesIn(allKeyboards()),
    [](const ::testing::TestParamInfo<const KeyboardLayout *> &info) {
      return std::string(info.param->id);
    });

TEST(KeyboardRegistryTest, IdsAreUnique) {
  std::set<std::string> ids;
  for (const KeyboardLayout *layout : allKeyboards()) {
    EXPECT_TRUE(ids.insert(layout->id).second) << layout->id;
  }
  EXPECT_EQ(findKeyboard("no-such-keyboard"), nullptr);
}

TEST(KeyboardRegistryTest, NamesAreSafeMenuLabels) {
  // Названия уходят в Fl_Choice::add(): там '/' — подменю, '&' — горячая
  // клавиша, '_' — разделитель, '\t' — сочетание, '|' — разделитель пунктов
  for (const KeyboardLayout *layout : allKeyboards()) {
    const std::string name = layout->name;
    EXPECT_EQ(name.find_first_of("/&_\t|\\"), std::string::npos) << name;
  }
}

// ---------------------------------------------------------------------------
// Количество клавиш у известных клавиатур
// ---------------------------------------------------------------------------

namespace {

size_t keyCount(const char *id) {
  const KeyboardLayout *layout = findKeyboard(id);
  return layout ? layout->keys.size() : 0;
}

} // namespace

TEST(KeyboardRegistryTest, KeyCounts) {
  EXPECT_EQ(keyCount("ansi104"), 104u);
  EXPECT_EQ(keyCount("ansi87"), 87u);
  EXPECT_EQ(keyCount("iso105"), 105u);
  EXPECT_EQ(keyCount("sofle"), 58u);  // без двух нажатий энкодеров
  EXPECT_EQ(keyCount("corne"), 42u);  // 3×6 + 3 на половину
  EXPECT_EQ(keyCount("lily58"), 58u);
}

TEST(KeyboardRegistryTest, SplitsAreMarked) {
  for (const char *id : {"sofle", "corne", "lily58"}) {
    ASSERT_NE(findKeyboard(id), nullptr) << id;
    EXPECT_TRUE(findKeyboard(id)->isSplit) << id;
  }
  for (const char *id : {"ansi104", "ansi87", "iso105"}) {
    ASSERT_NE(findKeyboard(id), nullptr) << id;
    EXPECT_FALSE(findKeyboard(id)->isSplit) << id;
  }
}

// Клавиши, которые должны попасть в «Нет на этой клавиатуре»
TEST(KeyboardRegistryTest, MissingKeysOnSplitsAndTkl) {
  HeatmapStats s = computeHeatmapStats(
      {{"F5", 3}, {"\xE2\x86\x91", 2}, {"Home", 1}, {"VK_0x61", 4}, {"A", 9},
       {"Ctrl+S", 1}},
      true);
  auto missingOn = [&](const char *id) {
    std::set<KeyCode> result;
    for (const MissingKey &m : missingKeys(s, presentStatKeys(*findKeyboard(id)))) {
      result.insert(m.key);
    }
    return result;
  };
  // Corne: нет F-ряда, стрелок, навигации и цифрового блока
  EXPECT_EQ(missingOn("corne"), (std::set<KeyCode>{KC_F5, KC_UP, KC_HOME, KC_P1}));
  // TKL: нет только цифрового блока
  EXPECT_EQ(missingOn("ansi87"), (std::set<KeyCode>{KC_P1}));
  // Полноразмерная: есть всё
  EXPECT_TRUE(missingOn("ansi104").empty());
  EXPECT_TRUE(missingOn("iso105").empty());
}

TEST(KeyboardRegistryTest, IsoHasBothIsoKeys) {
  std::set<KeyCode> iso = presentStatKeys(*findKeyboard("iso105"));
  EXPECT_TRUE(iso.count(KC_NUBS));
  EXPECT_TRUE(iso.count(KC_BSLS)); // через KC_NUHS
  std::set<KeyCode> ansi = presentStatKeys(*findKeyboard("ansi104"));
  EXPECT_FALSE(ansi.count(KC_NUBS));
}

TEST(KeyboardRegistryTest, BothModifierCopiesOnFullSize) {
  // Левый и правый Ctrl/Shift/Alt/Win есть и красятся одним счётом
  const KeyboardLayout *ansi = findKeyboard("ansi104");
  HeatmapStats s = computeHeatmapStats({{"Ctrl+Shift+Alt+Win+X", 5}}, true);
  int modifierKeys = 0;
  for (const KeyDef &key : ansi->keys) {
    if (isModifier(key.code)) {
      ++modifierKeys;
      EXPECT_EQ(appearanceFor(key.code, s).count, 5) << keyCodeName(key.code);
    }
  }
  EXPECT_EQ(modifierKeys, 8);
}

// ---------------------------------------------------------------------------
// Полноразмерная ANSI покрывает всё, что называет KeyLogger::virtualKeyToString
// ---------------------------------------------------------------------------

namespace {

// Строки основных клавиш, которые может выдать virtualKeyToString.
// Именованные клавиши берём прямо из исходника KeyLogger.cpp, чтобы тест
// заметил новую клавишу, добавленную туда без сопоставления здесь.
std::set<std::string> virtualKeyToStringNames() {
  std::set<std::string> names;
  for (char c = 'A'; c <= 'Z'; ++c) {
    names.insert(std::string(1, c));
  }
  for (char c = '0'; c <= '9'; ++c) {
    names.insert(std::string(1, c));
  }
  for (int i = 1; i <= 24; ++i) {
    names.insert("F" + std::to_string(i));
  }

#ifdef HOKA_SOURCE_DIR
  // u8path: путь к исходникам может содержать не-ASCII (например, кириллицу
  // в имени пользователя Windows)
  std::ifstream in(std::filesystem::u8path(std::string(HOKA_SOURCE_DIR) +
                                           "/src/KeyLogger/KeyLogger.cpp"),
                   std::ios::binary);
  if (in) {
    std::stringstream ss;
    ss << in.rdbuf();
    std::string source = ss.str();
    size_t start = source.find("KeyLogger::virtualKeyToString");
    if (start != std::string::npos) {
      std::string body = source.substr(start);
      std::regex returnString(R"re(return\s+"((?:[^"\\]|\\.)*)"\s*;)re");
      for (std::sregex_iterator it(body.begin(), body.end(), returnString), end;
           it != end; ++it) {
        std::string literal = (*it)[1];
        std::string unescaped;
        for (size_t i = 0; i < literal.size(); ++i) {
          if (literal[i] == '\\' && i + 1 < literal.size()) {
            ++i;
          }
          unescaped += literal[i];
        }
        names.insert(unescaped);
      }
    }
  }
#endif
  return names;
}

} // namespace

TEST(AnsiCoverageTest, SourceScanFindsNamedKeys) {
#ifndef HOKA_SOURCE_DIR
  GTEST_SKIP() << "HOKA_SOURCE_DIR не задан";
#else
  std::set<std::string> names = virtualKeyToStringNames();
  // Проверяем, что разбор исходника сработал
  EXPECT_TRUE(names.count("Space"));
  EXPECT_TRUE(names.count("PrintScreen"));
  EXPECT_TRUE(names.count("\\"));
  EXPECT_TRUE(names.count("\xE2\x86\x91"));
  EXPECT_TRUE(names.count("PlayPause"));
#endif
}

TEST(AnsiCoverageTest, EveryNamedKeyIsRecognized) {
  for (const std::string &name : virtualKeyToStringNames()) {
    EXPECT_TRUE(keyFromDbString(name).has_value())
        << "Нет сопоставления для строки '" << name << "'";
  }
}

TEST(AnsiCoverageTest, FullSizeAnsiCoversAllNonMediaKeys) {
  const KeyboardLayout *ansi = findKeyboard("ansi104");
  ASSERT_NE(ansi, nullptr);
  const std::set<KeyCode> present = presentStatKeys(*ansi);

  std::set<std::string> names = virtualKeyToStringNames();
  // Безымянные виртуальные коды, которые пишутся как VK_0x..
  for (const char *hex :
       {"VK_0xbd", "VK_0xbb", "VK_0x60", "VK_0x61", "VK_0x62", "VK_0x63",
        "VK_0x64", "VK_0x65", "VK_0x66", "VK_0x67", "VK_0x68", "VK_0x69",
        "VK_0x6e", "VK_0xc"}) {
    names.insert(hex);
  }

  for (const std::string &name : names) {
    std::optional<KeyCode> key = keyFromDbString(name);
    if (!key || isMediaKey(*key)) {
      continue;
    }
    // F13–F24 virtualKeyToString называет, но на 104-клавишной ANSI их нет
    if (*key >= KC_F13 && *key <= KC_F24) {
      continue;
    }
    EXPECT_TRUE(present.count(statisticKey(*key)))
        << "На ANSI 104 нет клавиши для '" << name << "' ("
        << keyCodeName(*key) << ")";
  }
}
