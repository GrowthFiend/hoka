// Corne (crkbd), раскладка 3×6+3 — базовый слой default keymap QMK.
#include "Heatmap/Keyboards.h"

namespace heatmap {

// Геометрия: qmk_firmware, keyboards/crkbd/info.json, "LAYOUT_split_3x6_3".
// Кейкоды: keyboards/crkbd/keymaps/default/keymap.c, слой 0.
// Поворотов в источнике нет. MO(1) и MO(2) — переключатели слоёв.
const KeyboardLayout &corneLayout() {
  static const KeyboardLayout layout{
      "corne",
      "Corne (crkbd 3×6+3)",
      "qmk_firmware: keyboards/crkbd/info.json (LAYOUT_split_3x6_3), "
      "keymaps/default/keymap.c (слой 0)",
      true,
      {
          // Ряд 0
          {KC_TAB, 0, 0.3f}, {KC_Q, 1, 0.3f}, {KC_W, 2, 0.1f},
          {KC_E, 3, 0}, {KC_R, 4, 0.1f}, {KC_T, 5, 0.2f},
          {KC_Y, 9, 0.2f}, {KC_U, 10, 0.1f}, {KC_I, 11, 0},
          {KC_O, 12, 0.1f}, {KC_P, 13, 0.3f}, {KC_BSPC, 14, 0.3f},
          // Ряд 1
          {KC_LCTL, 0, 1.3f}, {KC_A, 1, 1.3f}, {KC_S, 2, 1.1f},
          {KC_D, 3, 1}, {KC_F, 4, 1.1f}, {KC_G, 5, 1.2f},
          {KC_H, 9, 1.2f}, {KC_J, 10, 1.1f}, {KC_K, 11, 1},
          {KC_L, 12, 1.1f}, {KC_SCLN, 13, 1.3f}, {KC_QUOT, 14, 1.3f},
          // Ряд 2
          {KC_LSFT, 0, 2.3f}, {KC_Z, 1, 2.3f}, {KC_X, 2, 2.1f},
          {KC_C, 3, 2}, {KC_V, 4, 2.1f}, {KC_B, 5, 2.2f},
          {KC_N, 9, 2.2f}, {KC_M, 10, 2.1f}, {KC_COMM, 11, 2},
          {KC_DOT, 12, 2.1f}, {KC_SLSH, 13, 2.3f}, {KC_ESC, 14, 2.3f},
          // Большие пальцы
          {KC_LGUI, 4, 3.7f},
          {KC_LAYER, 5, 3.7f, 1, 1, 0, 0, 0, "MO(1)"},
          {KC_SPC, 6, 3.2f, 1, 1.5f},
          {KC_ENT, 8, 3.2f, 1, 1.5f},
          {KC_LAYER, 9, 3.7f, 1, 1, 0, 0, 0, "MO(2)"},
          {KC_RALT, 10, 3.7f},
      }};
  return layout;
}

} // namespace heatmap
