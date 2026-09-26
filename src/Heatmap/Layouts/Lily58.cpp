// Lily58 (rev1) — базовый слой default keymap QMK.
#include "Heatmap/Keyboards.h"

namespace heatmap {

// Геометрия: qmk_firmware, keyboards/lily58/rev1/keyboard.json, "LAYOUT".
// Кейкоды: keyboards/lily58/keymaps/default/keymap.c, слой _QWERTY.
// Поворотов в источнике нет. MO(_LOWER) и MO(_RAISE) — переключатели слоёв.
const KeyboardLayout &lily58Layout() {
  static const KeyboardLayout layout{
      "lily58",
      "Lily58",
      "qmk_firmware: keyboards/lily58/rev1/keyboard.json (LAYOUT), "
      "keymaps/default/keymap.c (_QWERTY)",
      true,
      {
          // Ряд 0
          {KC_ESC, 0, 0.5f}, {KC_1, 1, 0.375f}, {KC_2, 2, 0.125f},
          {KC_3, 3, 0}, {KC_4, 4, 0.125f}, {KC_5, 5, 0.25f},
          {KC_6, 10.5f, 0.25f}, {KC_7, 11.5f, 0.125f}, {KC_8, 12.5f, 0},
          {KC_9, 13.5f, 0.125f}, {KC_0, 14.5f, 0.375f}, {KC_GRV, 15.5f, 0.5f},
          // Ряд 1
          {KC_TAB, 0, 1.5f}, {KC_Q, 1, 1.375f}, {KC_W, 2, 1.125f},
          {KC_E, 3, 1}, {KC_R, 4, 1.125f}, {KC_T, 5, 1.25f},
          {KC_Y, 10.5f, 1.25f}, {KC_U, 11.5f, 1.125f}, {KC_I, 12.5f, 1},
          {KC_O, 13.5f, 1.125f}, {KC_P, 14.5f, 1.375f}, {KC_MINS, 15.5f, 1.5f},
          // Ряд 2
          {KC_LCTL, 0, 2.5f}, {KC_A, 1, 2.375f}, {KC_S, 2, 2.125f},
          {KC_D, 3, 2}, {KC_F, 4, 2.125f}, {KC_G, 5, 2.25f},
          {KC_H, 10.5f, 2.25f}, {KC_J, 11.5f, 2.125f}, {KC_K, 12.5f, 2},
          {KC_L, 13.5f, 2.125f}, {KC_SCLN, 14.5f, 2.375f}, {KC_QUOT, 15.5f, 2.5f},
          // Ряд 3 и внутренние клавиши [ ]
          {KC_LSFT, 0, 3.5f}, {KC_Z, 1, 3.375f}, {KC_X, 2, 3.125f},
          {KC_C, 3, 3}, {KC_V, 4, 3.125f}, {KC_B, 5, 3.25f},
          {KC_LBRC, 6, 2.75f}, {KC_RBRC, 9.5f, 2.75f},
          {KC_N, 10.5f, 3.25f}, {KC_M, 11.5f, 3.125f}, {KC_COMM, 12.5f, 3},
          {KC_DOT, 13.5f, 3.125f}, {KC_SLSH, 14.5f, 3.375f}, {KC_RSFT, 15.5f, 3.5f},
          // Большие пальцы
          {KC_LALT, 2.5f, 4.125f}, {KC_LGUI, 3.5f, 4.15f},
          {KC_LAYER, 4.5f, 4.25f, 1, 1, 0, 0, 0, "Lower"},
          {KC_SPC, 6, 4.25f, 1, 1.5f},
          {KC_ENT, 9.5f, 4.25f, 1, 1.5f},
          {KC_LAYER, 11, 4.25f, 1, 1, 0, 0, 0, "Raise"},
          {KC_BSPC, 12, 4.15f}, {KC_RGUI, 13, 4.15f},
      }};
  return layout;
}

} // namespace heatmap
