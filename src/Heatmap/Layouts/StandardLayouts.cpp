// Стандартные клавиатуры: ANSI 104, ANSI TKL 87, ISO 105.
//
// Геометрия — qmk_firmware, layouts/default/<layout>/info.json (массивы
// "layout"; в этих файлах нет подписей, порядок клавиш — по рядам слева
// направо). Кейкоды — стандартные надписи на клавишах в этих позициях.
#include "Heatmap/Keyboards.h"

namespace heatmap {

// qmk_firmware: layouts/default/fullsize_ansi/info.json (104 клавиши)
const KeyboardLayout &ansi104Layout() {
  static const KeyboardLayout layout{
      "ansi104",
      "ANSI 104 (полноразмерная)",
      "qmk_firmware: layouts/default/fullsize_ansi/info.json",
      false,
      {
          // Ряд функциональных клавиш
          {KC_ESC, 0, 0},
          {KC_F1, 2, 0}, {KC_F2, 3, 0}, {KC_F3, 4, 0}, {KC_F4, 5, 0},
          {KC_F5, 6.5f, 0}, {KC_F6, 7.5f, 0}, {KC_F7, 8.5f, 0}, {KC_F8, 9.5f, 0},
          {KC_F9, 11, 0}, {KC_F10, 12, 0}, {KC_F11, 13, 0}, {KC_F12, 14, 0},
          {KC_PSCR, 15.25f, 0}, {KC_SCRL, 16.25f, 0}, {KC_PAUS, 17.25f, 0},

          // Цифровой ряд
          {KC_GRV, 0, 1.25f},
          {KC_1, 1, 1.25f}, {KC_2, 2, 1.25f}, {KC_3, 3, 1.25f},
          {KC_4, 4, 1.25f}, {KC_5, 5, 1.25f}, {KC_6, 6, 1.25f},
          {KC_7, 7, 1.25f}, {KC_8, 8, 1.25f}, {KC_9, 9, 1.25f},
          {KC_0, 10, 1.25f}, {KC_MINS, 11, 1.25f}, {KC_EQL, 12, 1.25f},
          {KC_BSPC, 13, 1.25f, 2},
          {KC_INS, 15.25f, 1.25f}, {KC_HOME, 16.25f, 1.25f}, {KC_PGUP, 17.25f, 1.25f},
          {KC_NUM, 18.5f, 1.25f}, {KC_PSLS, 19.5f, 1.25f},
          {KC_PAST, 20.5f, 1.25f}, {KC_PMNS, 21.5f, 1.25f},

          // Верхний буквенный ряд
          {KC_TAB, 0, 2.25f, 1.5f},
          {KC_Q, 1.5f, 2.25f}, {KC_W, 2.5f, 2.25f}, {KC_E, 3.5f, 2.25f},
          {KC_R, 4.5f, 2.25f}, {KC_T, 5.5f, 2.25f}, {KC_Y, 6.5f, 2.25f},
          {KC_U, 7.5f, 2.25f}, {KC_I, 8.5f, 2.25f}, {KC_O, 9.5f, 2.25f},
          {KC_P, 10.5f, 2.25f}, {KC_LBRC, 11.5f, 2.25f}, {KC_RBRC, 12.5f, 2.25f},
          {KC_BSLS, 13.5f, 2.25f, 1.5f},
          {KC_DEL, 15.25f, 2.25f}, {KC_END, 16.25f, 2.25f}, {KC_PGDN, 17.25f, 2.25f},
          {KC_P7, 18.5f, 2.25f}, {KC_P8, 19.5f, 2.25f}, {KC_P9, 20.5f, 2.25f},
          {KC_PPLS, 21.5f, 2.25f, 1, 2},

          // Средний ряд
          {KC_CAPS, 0, 3.25f, 1.75f},
          {KC_A, 1.75f, 3.25f}, {KC_S, 2.75f, 3.25f}, {KC_D, 3.75f, 3.25f},
          {KC_F, 4.75f, 3.25f}, {KC_G, 5.75f, 3.25f}, {KC_H, 6.75f, 3.25f},
          {KC_J, 7.75f, 3.25f}, {KC_K, 8.75f, 3.25f}, {KC_L, 9.75f, 3.25f},
          {KC_SCLN, 10.75f, 3.25f}, {KC_QUOT, 11.75f, 3.25f},
          {KC_ENT, 12.75f, 3.25f, 2.25f},
          {KC_P4, 18.5f, 3.25f}, {KC_P5, 19.5f, 3.25f}, {KC_P6, 20.5f, 3.25f},

          // Нижний буквенный ряд
          {KC_LSFT, 0, 4.25f, 2.25f},
          {KC_Z, 2.25f, 4.25f}, {KC_X, 3.25f, 4.25f}, {KC_C, 4.25f, 4.25f},
          {KC_V, 5.25f, 4.25f}, {KC_B, 6.25f, 4.25f}, {KC_N, 7.25f, 4.25f},
          {KC_M, 8.25f, 4.25f}, {KC_COMM, 9.25f, 4.25f}, {KC_DOT, 10.25f, 4.25f},
          {KC_SLSH, 11.25f, 4.25f},
          {KC_RSFT, 12.25f, 4.25f, 2.75f},
          {KC_UP, 16.25f, 4.25f},
          {KC_P1, 18.5f, 4.25f}, {KC_P2, 19.5f, 4.25f}, {KC_P3, 20.5f, 4.25f},
          {KC_PENT, 21.5f, 4.25f, 1, 2},

          // Ряд пробела
          {KC_LCTL, 0, 5.25f, 1.25f}, {KC_LGUI, 1.25f, 5.25f, 1.25f},
          {KC_LALT, 2.5f, 5.25f, 1.25f}, {KC_SPC, 3.75f, 5.25f, 6.25f},
          {KC_RALT, 10, 5.25f, 1.25f}, {KC_RGUI, 11.25f, 5.25f, 1.25f},
          {KC_APP, 12.5f, 5.25f, 1.25f}, {KC_RCTL, 13.75f, 5.25f, 1.25f},
          {KC_LEFT, 15.25f, 5.25f}, {KC_DOWN, 16.25f, 5.25f}, {KC_RGHT, 17.25f, 5.25f},
          {KC_P0, 18.5f, 5.25f, 2}, {KC_PDOT, 20.5f, 5.25f},
      }};
  return layout;
}

} // namespace heatmap
