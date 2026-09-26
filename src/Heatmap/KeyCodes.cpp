#include "KeyCodes.h"

#include <cstdlib>
#include <unordered_map>

namespace heatmap {

namespace {

struct KeyCodeInfo {
  const char *name;
  const char *label;
  const char *description;
};

const KeyCodeInfo kKeyCodeInfo[] = {
#define HOKA_KEYCODE_INFO(id, label, description) {#id, label, description},
    HOKA_KEYCODE_LIST(HOKA_KEYCODE_INFO)
#undef HOKA_KEYCODE_INFO
};

static_assert(sizeof(kKeyCodeInfo) / sizeof(kKeyCodeInfo[0]) ==
                  static_cast<size_t>(kKeyCodeCount),
              "Таблица KeyCodeInfo должна совпадать с перечислением KeyCode");

const KeyCodeInfo &infoOf(KeyCode code) {
  static const KeyCodeInfo invalid{"", "", ""};
  return isValidKeyCode(code) ? kKeyCodeInfo[static_cast<int>(code)] : invalid;
}

// Строки основных клавиш, которые выдаёт KeyLogger::virtualKeyToString.
// Имена в той функции не переименовываем: в базе уже лежат данные под ними.
const std::unordered_map<std::string, KeyCode> &namedKeys() {
  static const std::unordered_map<std::string, KeyCode> table = [] {
    std::unordered_map<std::string, KeyCode> t = {
        {"Space", KeyCode::KC_SPC},
        {"Enter", KeyCode::KC_ENT}, // Enter цифрового блока даёт тот же VK_RETURN
        {"Backspace", KeyCode::KC_BSPC},
        {"Tab", KeyCode::KC_TAB},
        {"Esc", KeyCode::KC_ESC},
        {"Ctrl", KeyCode::KC_LCTL},
        {"Shift", KeyCode::KC_LSFT},
        {"Alt", KeyCode::KC_LALT},
        {"Win", KeyCode::KC_LGUI},
        {"\xE2\x86\x91", KeyCode::KC_UP},   // ↑
        {"\xE2\x86\x93", KeyCode::KC_DOWN}, // ↓
        {"\xE2\x86\x90", KeyCode::KC_LEFT}, // ←
        {"\xE2\x86\x92", KeyCode::KC_RGHT}, // →
        {"Insert", KeyCode::KC_INS},
        {"Delete", KeyCode::KC_DEL},
        {"Home", KeyCode::KC_HOME},
        {"End", KeyCode::KC_END},
        {"PageUp", KeyCode::KC_PGUP},
        {"PageDown", KeyCode::KC_PGDN},
        // VK_ADD, VK_SUBTRACT, VK_MULTIPLY — клавиши цифрового блока.
        // Минус основного ряда хранится как VK_0xbd (см. ниже).
        {"+", KeyCode::KC_PPLS},
        {"-", KeyCode::KC_PMNS},
        {"*", KeyCode::KC_PAST},
        // "/" выдают и VK_DIVIDE, и VK_OEM_2; относим к основному ряду
        {"/", KeyCode::KC_SLSH},
        // OEM-клавиши: позиции US-раскладки Windows. Виртуальные коды привязаны
        // к тем же физическим клавишам и при русской раскладке.
        {".", KeyCode::KC_DOT},
        {",", KeyCode::KC_COMM},
        {";", KeyCode::KC_SCLN},
        {"`", KeyCode::KC_GRV},
        {"[", KeyCode::KC_LBRC},
        {"\\", KeyCode::KC_BSLS},
        {"]", KeyCode::KC_RBRC},
        {"'", KeyCode::KC_QUOT},
        {"CapsLock", KeyCode::KC_CAPS},
        {"NumLock", KeyCode::KC_NUM},
        {"ScrollLock", KeyCode::KC_SCRL},
        // "PrintScreen" выдают VK_SNAPSHOT и VK_PRINT; это клавиша PrtSc
        {"PrintScreen", KeyCode::KC_PSCR},
        {"Pause", KeyCode::KC_PAUS},
        {"Menu", KeyCode::KC_APP},
        {"VolumeMute", KeyCode::KC_MUTE},
        {"VolumeDown", KeyCode::KC_VOLD},
        {"VolumeUp", KeyCode::KC_VOLU},
        {"NextTrack", KeyCode::KC_MNXT},
        {"PrevTrack", KeyCode::KC_MPRV},
        {"MediaStop", KeyCode::KC_MSTP},
        {"PlayPause", KeyCode::KC_MPLY},
    };
    // Буквы и цифры: одиночный символ
    for (int i = 0; i < 26; ++i) {
      t[std::string(1, static_cast<char>('A' + i))] =
          static_cast<KeyCode>(static_cast<int>(KeyCode::KC_A) + i);
    }
    const KeyCode digits[] = {KeyCode::KC_0, KeyCode::KC_1, KeyCode::KC_2,
                              KeyCode::KC_3, KeyCode::KC_4, KeyCode::KC_5,
                              KeyCode::KC_6, KeyCode::KC_7, KeyCode::KC_8,
                              KeyCode::KC_9};
    for (int i = 0; i < 10; ++i) {
      t[std::string(1, static_cast<char>('0' + i))] = digits[i];
    }
    // F1..F24
    for (int i = 0; i < 24; ++i) {
      t["F" + std::to_string(i + 1)] =
          static_cast<KeyCode>(static_cast<int>(KeyCode::KC_F1) + i);
    }
    return t;
  }();
  return table;
}

// Виртуальные коды, у которых в virtualKeyToString нет имени: они хранятся
// как "VK_0x" + код в шестнадцатеричном виде (std::hex, строчные буквы).
std::optional<KeyCode> keyFromUnnamedVirtualKey(unsigned long vk) {
  if (vk >= 0x60 && vk <= 0x69) { // VK_NUMPAD0..VK_NUMPAD9
    const KeyCode numpad[] = {KeyCode::KC_P0, KeyCode::KC_P1, KeyCode::KC_P2,
                              KeyCode::KC_P3, KeyCode::KC_P4, KeyCode::KC_P5,
                              KeyCode::KC_P6, KeyCode::KC_P7, KeyCode::KC_P8,
                              KeyCode::KC_P9};
    return numpad[vk - 0x60];
  }
  switch (vk) {
  case 0x0C: // VK_CLEAR: цифра 5 цифрового блока при выключенном NumLock
    return KeyCode::KC_P5;
  case 0x6E: // VK_DECIMAL
    return KeyCode::KC_PDOT;
  case 0xBB: // VK_OEM_PLUS: клавиша "=" основного ряда
    return KeyCode::KC_EQL;
  case 0xBD: // VK_OEM_MINUS: клавиша "-" основного ряда
    return KeyCode::KC_MINS;
  case 0xE2: // VK_OEM_102: дополнительная клавиша ISO слева от Z
    return KeyCode::KC_NUBS;
  default:
    return std::nullopt;
  }
}

bool startsWith(const std::string &s, const char *prefix, size_t prefixLen) {
  return s.size() >= prefixLen && s.compare(0, prefixLen, prefix) == 0;
}

} // namespace

bool isValidKeyCode(KeyCode code) {
  return static_cast<int>(code) < kKeyCodeCount; // KeyCode беззнаковый
}

const char *keyCodeName(KeyCode code) { return infoOf(code).name; }

const char *keyLabel(KeyCode code) { return infoOf(code).label; }

const char *keyDescription(KeyCode code) { return infoOf(code).description; }

std::optional<KeyCode> keyCodeFromName(const std::string &name) {
  static const std::unordered_map<std::string, KeyCode> byName = [] {
    std::unordered_map<std::string, KeyCode> m;
    for (int i = 0; i < kKeyCodeCount; ++i) {
      m[kKeyCodeInfo[i].name] = static_cast<KeyCode>(i);
    }
    return m;
  }();
  auto it = byName.find(name);
  if (it == byName.end()) {
    return std::nullopt;
  }
  return it->second;
}

ModifierFlag modifierFlagOf(KeyCode code) {
  switch (code) {
  case KeyCode::KC_LCTL:
  case KeyCode::KC_RCTL:
    return ModCtrl;
  case KeyCode::KC_LSFT:
  case KeyCode::KC_RSFT:
    return ModShift;
  case KeyCode::KC_LALT:
  case KeyCode::KC_RALT:
    return ModAlt;
  case KeyCode::KC_LGUI:
  case KeyCode::KC_RGUI:
    return ModWin;
  default:
    return ModNone;
  }
}

KeyCode modifierKeyOf(ModifierFlag flag) {
  switch (flag) {
  case ModCtrl:
    return KeyCode::KC_LCTL;
  case ModShift:
    return KeyCode::KC_LSFT;
  case ModAlt:
    return KeyCode::KC_LALT;
  case ModWin:
    return KeyCode::KC_LGUI;
  default:
    return KeyCode::KC_NO;
  }
}

bool isModifier(KeyCode code) { return modifierFlagOf(code) != ModNone; }

bool isMediaKey(KeyCode code) {
  switch (code) {
  case KeyCode::KC_MUTE:
  case KeyCode::KC_VOLU:
  case KeyCode::KC_VOLD:
  case KeyCode::KC_MNXT:
  case KeyCode::KC_MPRV:
  case KeyCode::KC_MSTP:
  case KeyCode::KC_MPLY:
    return true;
  default:
    return false;
  }
}

bool isNonStatisticKey(KeyCode code) {
  return code == KeyCode::KC_NO || code == KeyCode::KC_LAYER ||
         !isValidKeyCode(code);
}

KeyCode statisticKey(KeyCode code) {
  ModifierFlag flag = modifierFlagOf(code);
  if (flag != ModNone) {
    return modifierKeyOf(flag);
  }
  if (code == KeyCode::KC_NUHS) {
    return KeyCode::KC_BSLS;
  }
  return code;
}

std::optional<KeyCode> keyFromDbString(const std::string &keyName) {
  const auto &named = namedKeys();
  auto it = named.find(keyName);
  if (it != named.end()) {
    return it->second;
  }

  // "VK_0x" + шестнадцатеричный код
  static const char kHexPrefix[] = "VK_0x";
  const size_t prefixLen = sizeof(kHexPrefix) - 1;
  if (startsWith(keyName, kHexPrefix, prefixLen) &&
      keyName.size() > prefixLen && keyName.size() <= prefixLen + 4) {
    unsigned long vk = 0;
    for (size_t i = prefixLen; i < keyName.size(); ++i) {
      char c = keyName[i];
      int digit;
      if (c >= '0' && c <= '9') {
        digit = c - '0';
      } else if (c >= 'a' && c <= 'f') {
        digit = c - 'a' + 10;
      } else if (c >= 'A' && c <= 'F') {
        digit = c - 'A' + 10;
      } else {
        return std::nullopt;
      }
      vk = vk * 16 + static_cast<unsigned long>(digit);
    }
    return keyFromUnnamedVirtualKey(vk);
  }

  return std::nullopt;
}

ParsedCombination parseCombination(const std::string &combination) {
  struct Prefix {
    const char *text;
    size_t length;
    ModifierFlag flag;
  };
  // Порядок фиксирован: так префиксы пишет KeyLogger::keyboardProc
  static const Prefix kPrefixes[] = {
      {"Ctrl+", 5, ModCtrl},
      {"Shift+", 6, ModShift},
      {"Alt+", 4, ModAlt},
      {"Win+", 4, ModWin},
  };

  ParsedCombination result;
  size_t pos = 0;
  for (const auto &prefix : kPrefixes) {
    // Префикс отщепляем, только если после него что-то остаётся:
    // основная клавиша в комбинации есть всегда
    if (combination.size() > pos + prefix.length &&
        combination.compare(pos, prefix.length, prefix.text) == 0) {
      result.modifiers |= prefix.flag;
      pos += prefix.length;
    }
  }
  result.mainKey = combination.substr(pos);
  return result;
}

} // namespace heatmap
