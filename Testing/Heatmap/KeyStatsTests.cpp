#include <gtest/gtest.h>

#include <cmath>

#include "Heatmap/KeyStats.h"

using namespace heatmap;

namespace {

int64_t keyCount(const HeatmapStats &s, KeyCode code) {
  return s.keyCounts[static_cast<int>(code)];
}

int64_t modCount(const HeatmapStats &s, KeyCode code) {
  return s.modifierCounts[static_cast<int>(code)];
}

} // namespace

// ---------------------------------------------------------------------------
// Подсчёт
// ---------------------------------------------------------------------------

TEST(HeatmapStatsTest, MainKeyCountsInEveryRecord) {
  // Решение п. 2: и «S», и «Ctrl+S» дают вклад в S
  HeatmapStats s = computeHeatmapStats({{"S", 10}, {"Ctrl+S", 5}}, true);
  EXPECT_EQ(keyCount(s, KeyCode::KC_S), 15);
  EXPECT_EQ(s.countFor(KeyCode::KC_S), 15);
  EXPECT_EQ(s.maxKeyCount, 15);
}

TEST(HeatmapStatsTest, ModifierCountIsSumOfCombinationsWithIt) {
  HeatmapStats s = computeHeatmapStats({{"Ctrl+S", 5},
                                        {"Ctrl+Shift+Z", 2},
                                        {"Alt+Tab", 7},
                                        {"Win+E", 1},
                                        {"Shift+A", 3},
                                        {"A", 100}},
                                       true);
  EXPECT_EQ(modCount(s, KeyCode::KC_LCTL), 7);
  EXPECT_EQ(modCount(s, KeyCode::KC_LSFT), 5);
  EXPECT_EQ(modCount(s, KeyCode::KC_LALT), 7);
  EXPECT_EQ(modCount(s, KeyCode::KC_LGUI), 1);
  EXPECT_EQ(s.maxModifierCount, 7);
  // Модификаторы не попадают в шкалу обычных клавиш
  EXPECT_EQ(s.maxKeyCount, 103);
  EXPECT_EQ(keyCount(s, KeyCode::KC_LCTL), 0);
}

TEST(HeatmapStatsTest, LeftAndRightModifierPaintedTheSame) {
  HeatmapStats s = computeHeatmapStats({{"Ctrl+C", 4}, {"Ctrl+V", 6}}, true);
  EXPECT_EQ(s.countFor(KeyCode::KC_LCTL), 10);
  EXPECT_EQ(s.countFor(KeyCode::KC_RCTL), 10);
  KeyAppearance left = appearanceFor(KeyCode::KC_LCTL, s);
  KeyAppearance right = appearanceFor(KeyCode::KC_RCTL, s);
  EXPECT_EQ(left.fill, right.fill);
  EXPECT_EQ(left.count, right.count);
  EXPECT_TRUE(left.isModifier);
}

TEST(HeatmapStatsTest, NumpadPlusAndHexKeys) {
  HeatmapStats s = computeHeatmapStats(
      {{"Ctrl++", 3}, {"VK_0xbd", 4}, {"Ctrl+VK_0xbb", 2}, {"VK_0x60", 1}},
      true);
  EXPECT_EQ(keyCount(s, KeyCode::KC_PPLS), 3);
  EXPECT_EQ(keyCount(s, KeyCode::KC_MINS), 4);
  EXPECT_EQ(keyCount(s, KeyCode::KC_EQL), 2);
  EXPECT_EQ(keyCount(s, KeyCode::KC_P0), 1);
  EXPECT_EQ(modCount(s, KeyCode::KC_LCTL), 5);
  EXPECT_EQ(s.unrecognizedCombinations, 0);
}

TEST(HeatmapStatsTest, UnrecognizedRowsAreCountedSeparately) {
  HeatmapStats s = computeHeatmapStats(
      {{"Foo", 3}, {"Ctrl+VK_0x6c", 2}, {"A", 1}}, true);
  EXPECT_EQ(s.unrecognizedCombinations, 2);
  EXPECT_EQ(s.unrecognizedPresses, 5);
  EXPECT_EQ(s.maxKeyCount, 1);
  // Ctrl в нераспознанном сочетании всё равно был
  EXPECT_EQ(modCount(s, KeyCode::KC_LCTL), 2);
}

TEST(HeatmapStatsTest, LegacyBareModifiersCountOnlyAsModifiers) {
  HeatmapStats s =
      computeHeatmapStats({{"Ctrl", 4}, {"Shift+Ctrl", 1}, {"Z", 2}}, true);
  EXPECT_EQ(modCount(s, KeyCode::KC_LCTL), 5);
  EXPECT_EQ(modCount(s, KeyCode::KC_LSFT), 1);
  EXPECT_EQ(s.maxKeyCount, 2);
  EXPECT_EQ(s.unrecognizedCombinations, 0);
}

TEST(HeatmapStatsTest, NonPositiveCountsIgnored) {
  HeatmapStats s = computeHeatmapStats({{"A", 0}, {"B", -3}}, true);
  EXPECT_EQ(s.maxKeyCount, 0);
  EXPECT_EQ(s.unrecognizedCombinations, 0);
}

TEST(HeatmapStatsTest, EmptyData) {
  HeatmapStats s = computeHeatmapStats({}, true);
  EXPECT_EQ(s.maxKeyCount, 0);
  EXPECT_EQ(s.maxModifierCount, 0);
  KeyAppearance a = appearanceFor(KeyCode::KC_A, s);
  EXPECT_EQ(a.count, 0);
  EXPECT_EQ(a.fill, neutralColor());
}

TEST(HeatmapStatsTest, IsoBackslashGetsBackslashCount) {
  HeatmapStats s = computeHeatmapStats({{"\\", 7}}, true);
  EXPECT_EQ(s.countFor(KeyCode::KC_BSLS), 7);
  EXPECT_EQ(s.countFor(KeyCode::KC_NUHS), 7);
}

TEST(HeatmapStatsTest, ServiceKeysHaveNoCount) {
  HeatmapStats s = computeHeatmapStats({{"A", 7}}, true);
  EXPECT_EQ(s.countFor(KeyCode::KC_NO), 0);
  EXPECT_EQ(s.countFor(KeyCode::KC_LAYER), 0);
  KeyAppearance a = appearanceFor(KeyCode::KC_LAYER, s);
  EXPECT_TRUE(a.disabled);
  EXPECT_EQ(a.fill, neutralColor());
}

// ---------------------------------------------------------------------------
// Флажок «Учитывать модификаторы»
// ---------------------------------------------------------------------------

TEST(ModifierToggleTest, DisabledModifiersAreNeutralAndNotCounted) {
  const KeyCountRows rows = {{"Ctrl+S", 5}, {"Shift+A", 3}, {"S", 1}};
  HeatmapStats on = computeHeatmapStats(rows, true);
  HeatmapStats off = computeHeatmapStats(rows, false);

  EXPECT_FALSE(off.includeModifiers);
  EXPECT_EQ(off.maxModifierCount, 0);
  for (int64_t c : off.modifierCounts) {
    EXPECT_EQ(c, 0);
  }
  EXPECT_EQ(off.countFor(KeyCode::KC_LCTL), 0);

  KeyAppearance ctrl = appearanceFor(KeyCode::KC_RCTL, off);
  EXPECT_TRUE(ctrl.disabled);
  EXPECT_EQ(ctrl.count, 0);
  EXPECT_EQ(ctrl.fill, neutralColor());

  // Основные клавиши считаются одинаково при любом положении флажка
  EXPECT_EQ(on.keyCounts, off.keyCounts);
  EXPECT_EQ(on.maxKeyCount, off.maxKeyCount);
  EXPECT_EQ(appearanceFor(KeyCode::KC_S, on).fill,
            appearanceFor(KeyCode::KC_S, off).fill);
}

// ---------------------------------------------------------------------------
// Нормировка
// ---------------------------------------------------------------------------

TEST(IntensityTest, LogarithmicScale) {
  EXPECT_DOUBLE_EQ(intensity(0, 100), 0.0);
  EXPECT_DOUBLE_EQ(intensity(100, 100), 1.0);
  EXPECT_DOUBLE_EQ(intensity(1, 1), 1.0);
  EXPECT_NEAR(intensity(9, 99), 0.5, 1e-12); // log(10) / log(100)
  EXPECT_NEAR(intensity(1, 1000), std::log(2.0) / std::log(1001.0), 1e-12);
  EXPECT_DOUBLE_EQ(intensity(5, 0), 0.0);
  EXPECT_DOUBLE_EQ(intensity(-1, 10), 0.0);
  EXPECT_DOUBLE_EQ(intensity(20, 10), 1.0); // не выходит за 1
}

TEST(IntensityTest, KeysAndModifiersNormalizedSeparately) {
  HeatmapStats s = computeHeatmapStats({{"A", 1000}, {"Ctrl+B", 1}}, true);
  // Единственный модификатор — максимум своей шкалы
  KeyAppearance ctrl = appearanceFor(KeyCode::KC_LCTL, s);
  EXPECT_DOUBLE_EQ(ctrl.intensity, 1.0);
  EXPECT_EQ(ctrl.fill, scaleColor(ColorScale::Modifiers, 1.0));
  // Обычные клавиши — по своему максимуму
  EXPECT_DOUBLE_EQ(appearanceFor(KeyCode::KC_A, s).intensity, 1.0);
  EXPECT_NEAR(appearanceFor(KeyCode::KC_B, s).intensity,
              std::log(2.0) / std::log(1001.0), 1e-12);
}

TEST(IntensityTest, MoreModifierPressesMeansMoreSaturated) {
  HeatmapStats s =
      computeHeatmapStats({{"Ctrl+A", 50}, {"Alt+A", 5}, {"Win+A", 1}}, true);
  double ctrl = relativeLuminance(appearanceFor(KeyCode::KC_LCTL, s).fill);
  double alt = relativeLuminance(appearanceFor(KeyCode::KC_LALT, s).fill);
  double win = relativeLuminance(appearanceFor(KeyCode::KC_LGUI, s).fill);
  EXPECT_LT(ctrl, alt);
  EXPECT_LT(alt, win);
  EXPECT_EQ(appearanceFor(KeyCode::KC_LSFT, s).fill, neutralColor());
}

// ---------------------------------------------------------------------------
// Цвета
// ---------------------------------------------------------------------------

TEST(ColorTest, ScaleEndpoints) {
  EXPECT_EQ(scaleColor(ColorScale::Keys, 1.0), (Rgb{215, 48, 31}));
  EXPECT_EQ(scaleColor(ColorScale::Keys, 0.0), (Rgb{254, 232, 200}));
  EXPECT_EQ(scaleColor(ColorScale::Modifiers, 1.0), (Rgb{8, 81, 156}));
  // Значения за пределами [0, 1] прижимаются к краям
  EXPECT_EQ(scaleColor(ColorScale::Keys, 2.0), scaleColor(ColorScale::Keys, 1.0));
  EXPECT_EQ(scaleColor(ColorScale::Keys, -1.0),
            scaleColor(ColorScale::Keys, 0.0));
}

TEST(ColorTest, ScalesGetDarkerAndKeepTheirHue) {
  for (ColorScale scale : {ColorScale::Keys, ColorScale::Modifiers}) {
    double prev = 2.0;
    for (int i = 0; i <= 100; ++i) {
      Rgb c = scaleColor(scale, i / 100.0);
      double lum = relativeLuminance(c);
      EXPECT_LE(lum, prev + 1e-9) << "t=" << i / 100.0;
      prev = lum;
      if (scale == ColorScale::Keys) {
        // тёплая гамма: красный преобладает, синий минимален
        EXPECT_GE(c.r, c.g);
        EXPECT_GE(c.g, c.b);
      } else {
        // синяя гамма
        EXPECT_GE(c.b, c.g);
        EXPECT_GE(c.b, c.r);
      }
    }
  }
}

TEST(ColorTest, NeutralIsGreyAndDistinctFromScales) {
  Rgb n = neutralColor();
  EXPECT_EQ(n.r, n.g);
  EXPECT_EQ(n.g, n.b);
  EXPECT_NE(n, scaleColor(ColorScale::Keys, 0.0));
  EXPECT_NE(n, scaleColor(ColorScale::Modifiers, 0.0));
}

TEST(ColorTest, LabelsReadableOnAnyBackground) {
  for (ColorScale scale : {ColorScale::Keys, ColorScale::Modifiers}) {
    for (int i = 0; i <= 100; ++i) {
      Rgb bg = scaleColor(scale, i / 100.0);
      EXPECT_GE(contrastRatio(bg, labelColorFor(bg)), 4.5)
          << "t=" << i / 100.0;
    }
  }
  Rgb n = neutralColor();
  EXPECT_GE(contrastRatio(n, labelColorFor(n)), 4.5);
  // Тёмный конец шкалы — белая подпись, светлый — чёрная
  EXPECT_EQ(labelColorFor(scaleColor(ColorScale::Modifiers, 1.0)),
            (Rgb{255, 255, 255}));
  EXPECT_EQ(labelColorFor(scaleColor(ColorScale::Keys, 0.0)), (Rgb{0, 0, 0}));
}

TEST(ColorTest, ContrastRatioBasics) {
  EXPECT_NEAR(contrastRatio({0, 0, 0}, {255, 255, 255}), 21.0, 1e-9);
  EXPECT_NEAR(contrastRatio({10, 20, 30}, {10, 20, 30}), 1.0, 1e-9);
}

// ---------------------------------------------------------------------------
// «Нет на этой клавиатуре»
// ---------------------------------------------------------------------------

TEST(MissingKeysTest, KeysAbsentFromLayoutAreListedByCount) {
  HeatmapStats s = computeHeatmapStats(
      {{"F5", 3}, {"\xE2\x86\x91", 10}, {"A", 50}, {"Ctrl+A", 2},
       {"Alt+F4", 1}},
      true);
  // Условная клавиатура без F-ряда, стрелок и Alt
  std::set<KeyCode> present = {KeyCode::KC_A, KeyCode::KC_LCTL};
  std::vector<MissingKey> missing = missingKeys(s, present);
  ASSERT_EQ(missing.size(), 4u);
  EXPECT_EQ(missing[0].key, KeyCode::KC_UP);
  EXPECT_EQ(missing[0].count, 10);
  EXPECT_EQ(missing[1].key, KeyCode::KC_F5);
  EXPECT_EQ(missing[1].count, 3);
  // F4 и Alt по 1: при равенстве — порядок кейкодов (F4 раньше модификаторов)
  EXPECT_EQ(missing[2].key, KeyCode::KC_F4);
  EXPECT_EQ(missing[3].key, KeyCode::KC_LALT);
  EXPECT_TRUE(missing[3].isModifier);
}

TEST(MissingKeysTest, ModifiersSkippedWhenToggleOff) {
  HeatmapStats s = computeHeatmapStats({{"Alt+F4", 1}}, false);
  std::vector<MissingKey> missing = missingKeys(s, {});
  ASSERT_EQ(missing.size(), 1u);
  EXPECT_EQ(missing[0].key, KeyCode::KC_F4);
}

TEST(MissingKeysTest, NothingMissingWhenAllPresent) {
  HeatmapStats s = computeHeatmapStats({{"A", 1}, {"Ctrl+B", 1}}, true);
  std::set<KeyCode> present = {KeyCode::KC_A, KeyCode::KC_B, KeyCode::KC_LCTL};
  EXPECT_TRUE(missingKeys(s, present).empty());
}
