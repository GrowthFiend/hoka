#pragma once
#include <cstdint>
#include <optional>
#include <string>

// Логические клавиши тепловой карты.
// Идентификаторы повторяют кейкоды QMK (KC_A, KC_F5, KC_LCTL, KC_P0...), чтобы
// раскладки из qmk_firmware переносились в таблицы клавиатур один в один.
// Модуль не зависит ни от FLTK, ни от Windows.
namespace heatmap {

// X(идентификатор, подпись на клавише, полное название для подсказки)
#define HOKA_KEYCODE_LIST(X)                                                   \
  /* Служебные: не клавиши статистики */                                      \
  X(KC_NO, "", "Не назначена (KC_NO)")                                         \
  X(KC_LAYER, "Layer", "Переключение слоя QMK")                                \
  /* Буквы */                                                                  \
  X(KC_A, "A", "A") X(KC_B, "B", "B") X(KC_C, "C", "C") X(KC_D, "D", "D")     \
  X(KC_E, "E", "E") X(KC_F, "F", "F") X(KC_G, "G", "G") X(KC_H, "H", "H")     \
  X(KC_I, "I", "I") X(KC_J, "J", "J") X(KC_K, "K", "K") X(KC_L, "L", "L")     \
  X(KC_M, "M", "M") X(KC_N, "N", "N") X(KC_O, "O", "O") X(KC_P, "P", "P")     \
  X(KC_Q, "Q", "Q") X(KC_R, "R", "R") X(KC_S, "S", "S") X(KC_T, "T", "T")     \
  X(KC_U, "U", "U") X(KC_V, "V", "V") X(KC_W, "W", "W") X(KC_X, "X", "X")     \
  X(KC_Y, "Y", "Y") X(KC_Z, "Z", "Z")                                          \
  /* Цифры основного ряда */                                                   \
  X(KC_1, "1", "1") X(KC_2, "2", "2") X(KC_3, "3", "3") X(KC_4, "4", "4")     \
  X(KC_5, "5", "5") X(KC_6, "6", "6") X(KC_7, "7", "7") X(KC_8, "8", "8")     \
  X(KC_9, "9", "9") X(KC_0, "0", "0")                                          \
  /* Основной блок */                                                          \
  X(KC_ENT, "Enter", "Enter")                                                  \
  X(KC_ESC, "Esc", "Esc")                                                      \
  X(KC_BSPC, "Bksp", "Backspace")                                              \
  X(KC_TAB, "Tab", "Tab")                                                      \
  X(KC_SPC, "Space", "Space")                                                  \
  X(KC_MINS, "-", "- (основной ряд)")                                          \
  X(KC_EQL, "=", "=")                                                          \
  X(KC_LBRC, "[", "[")                                                         \
  X(KC_RBRC, "]", "]")                                                         \
  X(KC_BSLS, "\\", "\\")                                                       \
  X(KC_NUHS, "\\", "\\ (ISO, у Enter)")                                        \
  X(KC_SCLN, ";", ";")                                                         \
  X(KC_QUOT, "'", "'")                                                         \
  X(KC_GRV, "`", "`")                                                          \
  X(KC_COMM, ",", ",")                                                         \
  X(KC_DOT, ".", ".")                                                          \
  X(KC_SLSH, "/", "/ (основной ряд)")                                          \
  X(KC_CAPS, "Caps", "CapsLock")                                               \
  X(KC_NUBS, "<>", "Доп. клавиша ISO слева от Z (VK_OEM_102)")                 \
  X(KC_APP, "Menu", "Menu")                                                    \
  /* Функциональные */                                                         \
  X(KC_F1, "F1", "F1") X(KC_F2, "F2", "F2") X(KC_F3, "F3", "F3")              \
  X(KC_F4, "F4", "F4") X(KC_F5, "F5", "F5") X(KC_F6, "F6", "F6")              \
  X(KC_F7, "F7", "F7") X(KC_F8, "F8", "F8") X(KC_F9, "F9", "F9")              \
  X(KC_F10, "F10", "F10") X(KC_F11, "F11", "F11") X(KC_F12, "F12", "F12")     \
  X(KC_F13, "F13", "F13") X(KC_F14, "F14", "F14") X(KC_F15, "F15", "F15")     \
  X(KC_F16, "F16", "F16") X(KC_F17, "F17", "F17") X(KC_F18, "F18", "F18")     \
  X(KC_F19, "F19", "F19") X(KC_F20, "F20", "F20") X(KC_F21, "F21", "F21")     \
  X(KC_F22, "F22", "F22") X(KC_F23, "F23", "F23") X(KC_F24, "F24", "F24")     \
  /* Навигация и системные */                                                  \
  X(KC_PSCR, "PrtSc", "PrintScreen")                                           \
  X(KC_SCRL, "ScrLk", "ScrollLock")                                            \
  X(KC_PAUS, "Pause", "Pause")                                                 \
  X(KC_INS, "Ins", "Insert")                                                   \
  X(KC_HOME, "Home", "Home")                                                   \
  X(KC_PGUP, "PgUp", "PageUp")                                                 \
  X(KC_DEL, "Del", "Delete")                                                   \
  X(KC_END, "End", "End")                                                      \
  X(KC_PGDN, "PgDn", "PageDown")                                               \
  X(KC_UP, "\xE2\x86\x91", "Стрелка вверх")                                    \
  X(KC_DOWN, "\xE2\x86\x93", "Стрелка вниз")                                   \
  X(KC_LEFT, "\xE2\x86\x90", "Стрелка влево")                                  \
  X(KC_RGHT, "\xE2\x86\x92", "Стрелка вправо")                                 \
  /* Цифровой блок */                                                          \
  X(KC_NUM, "Num", "NumLock")                                                  \
  X(KC_PSLS, "/", "Num / (в данных не отличается от / основного ряда)")        \
  X(KC_PAST, "*", "Num *")                                                     \
  X(KC_PMNS, "-", "Num -")                                                     \
  X(KC_PPLS, "+", "Num +")                                                     \
  X(KC_PENT, "Enter", "Num Enter (в данных не отличается от Enter)")           \
  X(KC_P1, "1", "Num 1") X(KC_P2, "2", "Num 2") X(KC_P3, "3", "Num 3")        \
  X(KC_P4, "4", "Num 4") X(KC_P5, "5", "Num 5") X(KC_P6, "6", "Num 6")        \
  X(KC_P7, "7", "Num 7") X(KC_P8, "8", "Num 8") X(KC_P9, "9", "Num 9")        \
  X(KC_P0, "0", "Num 0")                                                       \
  X(KC_PDOT, ".", "Num .")                                                     \
  /* Модификаторы (левый и правый в данных не различаются) */                  \
  X(KC_LCTL, "Ctrl", "Ctrl")                                                   \
  X(KC_LSFT, "Shift", "Shift")                                                 \
  X(KC_LALT, "Alt", "Alt")                                                     \
  X(KC_LGUI, "Win", "Win")                                                     \
  X(KC_RCTL, "Ctrl", "Ctrl")                                                   \
  X(KC_RSFT, "Shift", "Shift")                                                 \
  X(KC_RALT, "Alt", "Alt")                                                     \
  X(KC_RGUI, "Win", "Win")                                                     \
  /* Мультимедиа */                                                            \
  X(KC_MUTE, "Mute", "VolumeMute")                                             \
  X(KC_VOLU, "Vol+", "VolumeUp")                                               \
  X(KC_VOLD, "Vol-", "VolumeDown")                                             \
  X(KC_MNXT, "Next", "NextTrack")                                              \
  X(KC_MPRV, "Prev", "PrevTrack")                                              \
  X(KC_MSTP, "Stop", "MediaStop")                                              \
  X(KC_MPLY, "Play", "PlayPause")

// Перечисление без class: в таблицах клавиатур кейкоды пишутся как в keymap
// QMK (KC_A, KC_LCTL), а в остальном коде — KeyCode::KC_A.
enum KeyCode : uint16_t {
#define HOKA_KEYCODE_ENUM(id, label, description) id,
  HOKA_KEYCODE_LIST(HOKA_KEYCODE_ENUM)
#undef HOKA_KEYCODE_ENUM
  KC__COUNT // не клавиша: число элементов
};

constexpr int kKeyCodeCount = static_cast<int>(KC__COUNT);

// Битовые флаги модификаторов в том виде, в каком их пишет KeyLogger
enum ModifierFlag : uint8_t {
  ModNone = 0,
  ModCtrl = 1 << 0,
  ModShift = 1 << 1,
  ModAlt = 1 << 2,
  ModWin = 1 << 3,
};

// Имя кейкода в стиле QMK: "KC_A", "KC_LCTL"
const char *keyCodeName(KeyCode code);
// Короткая подпись на клавише
const char *keyLabel(KeyCode code);
// Полное название для подсказок и списка «Нет на этой клавиатуре»
const char *keyDescription(KeyCode code);
// Обратное к keyCodeName: "KC_A" -> KeyCode::KC_A
std::optional<KeyCode> keyCodeFromName(const std::string &name);

bool isValidKeyCode(KeyCode code);
bool isModifier(KeyCode code);
bool isMediaKey(KeyCode code);
// Клавиши, которые не считаются: KC_NO и переключатели слоёв
bool isNonStatisticKey(KeyCode code);

// Клавиша, под которой ведётся счёт. Данные не различают левый и правый
// модификатор, поэтому KC_RCTL -> KC_LCTL и т. д. Клавиша ISO у Enter
// (KC_NUHS) на Windows даёт тот же скан-код и VK_OEM_5, что и ANSI-клавиша
// «\», поэтому KC_NUHS -> KC_BSLS.
KeyCode statisticKey(KeyCode code);

// Флаг модификатора для KC_LCTL/KC_RCTL и т. п., иначе ModNone
ModifierFlag modifierFlagOf(KeyCode code);
// Кейкод, под которым считается модификатор (KC_LCTL, KC_LSFT, KC_LALT, KC_LGUI)
KeyCode modifierKeyOf(ModifierFlag flag);

// Строка основной клавиши из БД (результат KeyLogger::virtualKeyToString)
// -> логическая клавиша. Модификаторы ("Ctrl", "Shift", "Alt", "Win")
// сопоставляются с KC_LCTL/KC_LSFT/KC_LALT/KC_LGUI.
// Нераспознанная строка -> std::nullopt.
std::optional<KeyCode> keyFromDbString(const std::string &keyName);

// Разобранная комбинация клавиш из key_statistics.key_combination
struct ParsedCombination {
  uint8_t modifiers = ModNone;   // флаги ModifierFlag
  std::string mainKey;           // основная клавиша как строка из БД
};

// Отщепляет известные префиксы в том порядке, в котором их пишет
// KeyLogger::keyboardProc: "Ctrl+", "Shift+", "Alt+", "Win+". Строку не
// разбиваем по '+': бывает "Ctrl++" (плюс цифрового блока).
ParsedCombination parseCombination(const std::string &combination);

} // namespace heatmap
