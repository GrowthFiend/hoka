#include <gtest/gtest.h>

#include "Heatmap/KeyCodes.h"

using namespace heatmap;

// ---------------------------------------------------------------------------
// Разбор комбинаций
// ---------------------------------------------------------------------------

TEST(ParseCombinationTest, PlainKeyHasNoModifiers) {
  ParsedCombination p = parseCombination("S");
  EXPECT_EQ(p.modifiers, ModNone);
  EXPECT_EQ(p.mainKey, "S");
}

TEST(ParseCombinationTest, AllPrefixesInKeyLoggerOrder) {
  ParsedCombination p = parseCombination("Ctrl+Shift+Alt+Win+F5");
  EXPECT_EQ(p.modifiers, ModCtrl | ModShift | ModAlt | ModWin);
  EXPECT_EQ(p.mainKey, "F5");
}

TEST(ParseCombinationTest, SinglePrefixes) {
  EXPECT_EQ(parseCombination("Ctrl+S").modifiers, ModCtrl);
  EXPECT_EQ(parseCombination("Shift+A").modifiers, ModShift);
  EXPECT_EQ(parseCombination("Alt+Tab").modifiers, ModAlt);
  EXPECT_EQ(parseCombination("Win+E").modifiers, ModWin);
  EXPECT_EQ(parseCombination("Ctrl+Alt+Delete").modifiers, ModCtrl | ModAlt);
  EXPECT_EQ(parseCombination("Ctrl+Alt+Delete").mainKey, "Delete");
}

TEST(ParseCombinationTest, NumpadPlusIsNotASeparator) {
  ParsedCombination p = parseCombination("Ctrl++");
  EXPECT_EQ(p.modifiers, ModCtrl);
  EXPECT_EQ(p.mainKey, "+");

  p = parseCombination("Ctrl+Shift++");
  EXPECT_EQ(p.modifiers, ModCtrl | ModShift);
  EXPECT_EQ(p.mainKey, "+");

  p = parseCombination("+");
  EXPECT_EQ(p.modifiers, ModNone);
  EXPECT_EQ(p.mainKey, "+");
}

TEST(ParseCombinationTest, HexVirtualKeysKeepTheirName) {
  ParsedCombination p = parseCombination("Ctrl+VK_0xbd");
  EXPECT_EQ(p.modifiers, ModCtrl);
  EXPECT_EQ(p.mainKey, "VK_0xbd");
}

TEST(ParseCombinationTest, UnicodeArrows) {
  ParsedCombination p = parseCombination("Shift+\xE2\x86\x91");
  EXPECT_EQ(p.modifiers, ModShift);
  EXPECT_EQ(p.mainKey, "\xE2\x86\x91");
}

TEST(ParseCombinationTest, PrefixesOnlyInFixedOrder) {
  // KeyLogger так не пишет: Ctrl после Win не отщепляется
  ParsedCombination p = parseCombination("Win+Ctrl+X");
  EXPECT_EQ(p.modifiers, ModWin);
  EXPECT_EQ(p.mainKey, "Ctrl+X");
}

TEST(ParseCombinationTest, LegacyModifierAsMainKey) {
  // До коммита «Mod keys no more main keys» записывались и одиночные модификаторы
  ParsedCombination p = parseCombination("Shift+Ctrl");
  EXPECT_EQ(p.modifiers, ModShift);
  EXPECT_EQ(p.mainKey, "Ctrl");

  p = parseCombination("Ctrl");
  EXPECT_EQ(p.modifiers, ModNone);
  EXPECT_EQ(p.mainKey, "Ctrl");
}

TEST(ParseCombinationTest, BarePrefixIsKeptAsMainKey) {
  ParsedCombination p = parseCombination("Ctrl+");
  EXPECT_EQ(p.modifiers, ModNone);
  EXPECT_EQ(p.mainKey, "Ctrl+");
  EXPECT_EQ(parseCombination("").mainKey, "");
}

// ---------------------------------------------------------------------------
// Строка из БД -> логическая клавиша
// ---------------------------------------------------------------------------

TEST(KeyFromDbStringTest, LettersDigitsAndFunctionKeys) {
  EXPECT_EQ(keyFromDbString("A"), KeyCode::KC_A);
  EXPECT_EQ(keyFromDbString("Z"), KeyCode::KC_Z);
  EXPECT_EQ(keyFromDbString("0"), KeyCode::KC_0);
  EXPECT_EQ(keyFromDbString("1"), KeyCode::KC_1);
  EXPECT_EQ(keyFromDbString("9"), KeyCode::KC_9);
  EXPECT_EQ(keyFromDbString("F1"), KeyCode::KC_F1);
  EXPECT_EQ(keyFromDbString("F12"), KeyCode::KC_F12);
  EXPECT_EQ(keyFromDbString("F24"), KeyCode::KC_F24);
}

TEST(KeyFromDbStringTest, NumpadOperatorsAreNumpadKeys) {
  EXPECT_EQ(keyFromDbString("+"), KeyCode::KC_PPLS);
  EXPECT_EQ(keyFromDbString("-"), KeyCode::KC_PMNS);
  EXPECT_EQ(keyFromDbString("*"), KeyCode::KC_PAST);
}

TEST(KeyFromDbStringTest, AmbiguousStrings) {
  // Решение п. 5: "/" — клавиша основного ряда, "PrintScreen" — PrtSc
  EXPECT_EQ(keyFromDbString("/"), KeyCode::KC_SLSH);
  EXPECT_EQ(keyFromDbString("PrintScreen"), KeyCode::KC_PSCR);
  // Enter цифрового блока в данных не отличается от основного
  EXPECT_EQ(keyFromDbString("Enter"), KeyCode::KC_ENT);
}

TEST(KeyFromDbStringTest, HexVirtualKeys) {
  EXPECT_EQ(keyFromDbString("VK_0xbd"), KeyCode::KC_MINS); // VK_OEM_MINUS
  EXPECT_EQ(keyFromDbString("VK_0xbb"), KeyCode::KC_EQL);  // VK_OEM_PLUS
  EXPECT_EQ(keyFromDbString("VK_0xe2"), KeyCode::KC_NUBS); // VK_OEM_102
  EXPECT_EQ(keyFromDbString("VK_0x60"), KeyCode::KC_P0);
  EXPECT_EQ(keyFromDbString("VK_0x61"), KeyCode::KC_P1);
  EXPECT_EQ(keyFromDbString("VK_0x65"), KeyCode::KC_P5);
  EXPECT_EQ(keyFromDbString("VK_0x69"), KeyCode::KC_P9);
  EXPECT_EQ(keyFromDbString("VK_0x6e"), KeyCode::KC_PDOT); // VK_DECIMAL
  EXPECT_EQ(keyFromDbString("VK_0xc"), KeyCode::KC_P5);    // VK_CLEAR
  // std::hex пишет строчными, но и заглавные разбираем
  EXPECT_EQ(keyFromDbString("VK_0xBD"), KeyCode::KC_MINS);
}

TEST(KeyFromDbStringTest, OemKeysByUsPositions) {
  EXPECT_EQ(keyFromDbString("`"), KeyCode::KC_GRV);
  EXPECT_EQ(keyFromDbString("["), KeyCode::KC_LBRC);
  EXPECT_EQ(keyFromDbString("]"), KeyCode::KC_RBRC);
  EXPECT_EQ(keyFromDbString("\\"), KeyCode::KC_BSLS);
  EXPECT_EQ(keyFromDbString(";"), KeyCode::KC_SCLN);
  EXPECT_EQ(keyFromDbString("'"), KeyCode::KC_QUOT);
  EXPECT_EQ(keyFromDbString(","), KeyCode::KC_COMM);
  EXPECT_EQ(keyFromDbString("."), KeyCode::KC_DOT);
}

TEST(KeyFromDbStringTest, ArrowsAreUtf8) {
  EXPECT_EQ(keyFromDbString("\xE2\x86\x91"), KeyCode::KC_UP);
  EXPECT_EQ(keyFromDbString("\xE2\x86\x93"), KeyCode::KC_DOWN);
  EXPECT_EQ(keyFromDbString("\xE2\x86\x90"), KeyCode::KC_LEFT);
  EXPECT_EQ(keyFromDbString("\xE2\x86\x92"), KeyCode::KC_RGHT);
}

TEST(KeyFromDbStringTest, NavigationAndSystemKeys) {
  EXPECT_EQ(keyFromDbString("Space"), KeyCode::KC_SPC);
  EXPECT_EQ(keyFromDbString("Backspace"), KeyCode::KC_BSPC);
  EXPECT_EQ(keyFromDbString("Tab"), KeyCode::KC_TAB);
  EXPECT_EQ(keyFromDbString("Esc"), KeyCode::KC_ESC);
  EXPECT_EQ(keyFromDbString("Insert"), KeyCode::KC_INS);
  EXPECT_EQ(keyFromDbString("Delete"), KeyCode::KC_DEL);
  EXPECT_EQ(keyFromDbString("Home"), KeyCode::KC_HOME);
  EXPECT_EQ(keyFromDbString("End"), KeyCode::KC_END);
  EXPECT_EQ(keyFromDbString("PageUp"), KeyCode::KC_PGUP);
  EXPECT_EQ(keyFromDbString("PageDown"), KeyCode::KC_PGDN);
  EXPECT_EQ(keyFromDbString("CapsLock"), KeyCode::KC_CAPS);
  EXPECT_EQ(keyFromDbString("NumLock"), KeyCode::KC_NUM);
  EXPECT_EQ(keyFromDbString("ScrollLock"), KeyCode::KC_SCRL);
  EXPECT_EQ(keyFromDbString("Pause"), KeyCode::KC_PAUS);
  EXPECT_EQ(keyFromDbString("Menu"), KeyCode::KC_APP);
}

TEST(KeyFromDbStringTest, ModifiersAndMedia) {
  EXPECT_EQ(keyFromDbString("Ctrl"), KeyCode::KC_LCTL);
  EXPECT_EQ(keyFromDbString("Shift"), KeyCode::KC_LSFT);
  EXPECT_EQ(keyFromDbString("Alt"), KeyCode::KC_LALT);
  EXPECT_EQ(keyFromDbString("Win"), KeyCode::KC_LGUI);
  EXPECT_EQ(keyFromDbString("VolumeMute"), KeyCode::KC_MUTE);
  EXPECT_EQ(keyFromDbString("PlayPause"), KeyCode::KC_MPLY);
}

TEST(KeyFromDbStringTest, UnknownStrings) {
  EXPECT_FALSE(keyFromDbString(""));
  EXPECT_FALSE(keyFromDbString("a")); // KeyLogger пишет буквы заглавными
  EXPECT_FALSE(keyFromDbString("Foo"));
  EXPECT_FALSE(keyFromDbString("VK_0x"));
  EXPECT_FALSE(keyFromDbString("VK_0xzz"));
  EXPECT_FALSE(keyFromDbString("VK_0x6c")); // VK_SEPARATOR — не сопоставляем
  EXPECT_FALSE(keyFromDbString("VK_0x12345"));
  EXPECT_FALSE(keyFromDbString("Ctrl+S")); // это комбинация, а не клавиша
}

// ---------------------------------------------------------------------------
// Свойства кейкодов
// ---------------------------------------------------------------------------

TEST(KeyCodeTest, NamesRoundTrip) {
  for (int i = 0; i < kKeyCodeCount; ++i) {
    KeyCode code = static_cast<KeyCode>(i);
    std::string name = keyCodeName(code);
    ASSERT_FALSE(name.empty()) << i;
    EXPECT_EQ(name.rfind("KC_", 0), 0u) << name;
    EXPECT_EQ(keyCodeFromName(name), code) << name;
    EXPECT_NE(keyDescription(code), nullptr);
  }
  EXPECT_FALSE(keyCodeFromName("KC_UNKNOWN"));
  EXPECT_STREQ(keyCodeName(KeyCode::KC_P0), "KC_P0");
}

TEST(KeyCodeTest, LeftAndRightModifiersShareOneStatisticKey) {
  EXPECT_EQ(statisticKey(KeyCode::KC_RCTL), KeyCode::KC_LCTL);
  EXPECT_EQ(statisticKey(KeyCode::KC_RSFT), KeyCode::KC_LSFT);
  EXPECT_EQ(statisticKey(KeyCode::KC_RALT), KeyCode::KC_LALT);
  EXPECT_EQ(statisticKey(KeyCode::KC_RGUI), KeyCode::KC_LGUI);
  EXPECT_EQ(statisticKey(KeyCode::KC_LCTL), KeyCode::KC_LCTL);
  EXPECT_TRUE(isModifier(KeyCode::KC_RGUI));
  EXPECT_FALSE(isModifier(KeyCode::KC_A));
}

TEST(KeyCodeTest, IsoBackslashSharesStatisticKey) {
  EXPECT_EQ(statisticKey(KeyCode::KC_NUHS), KeyCode::KC_BSLS);
  // Остальные клавиши считаются под собой
  EXPECT_EQ(statisticKey(KeyCode::KC_PENT), KeyCode::KC_PENT);
  EXPECT_EQ(statisticKey(KeyCode::KC_PSLS), KeyCode::KC_PSLS);
  EXPECT_EQ(statisticKey(KeyCode::KC_NUBS), KeyCode::KC_NUBS);
}

TEST(KeyCodeTest, NonStatisticKeys) {
  EXPECT_TRUE(isNonStatisticKey(KeyCode::KC_NO));
  EXPECT_TRUE(isNonStatisticKey(KeyCode::KC_LAYER));
  EXPECT_FALSE(isNonStatisticKey(KeyCode::KC_A));
  EXPECT_FALSE(isValidKeyCode(static_cast<KeyCode>(kKeyCodeCount)));
}
