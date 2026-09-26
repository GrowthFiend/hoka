#pragma once
#include "KeyCodes.h"

#include <array>
#include <set>
#include <string>
#include <vector>

// Клавиатуры для тепловой карты — чистые данные: для каждой клавиши логический
// кейкод, подпись и геометрия в единицах клавиши (1u = ширина обычной клавиши).
// Модуль не зависит ни от FLTK, ни от Windows.
namespace heatmap {

// Одна физическая клавиша. Семантика полей как в KLE и в layouts QMK
// info.json: (x, y, w, h) — прямоугольник до поворота в абсолютных
// координатах; r — поворот в градусах по часовой стрелке вокруг (rx, ry).
struct KeyDef {
  KeyCode code = KC_NO;
  float x = 0, y = 0;
  float w = 1, h = 1;
  float r = 0;
  float rx = 0, ry = 0;
  // Подпись; nullptr — стандартная подпись кейкода (keyLabel)
  const char *label = nullptr;
  // Вторая часть фигуры (ISO Enter): смещение от (x, y) и размер, как
  // x2/y2/w2/h2 в KLE. w2 == 0 — второй части нет.
  float x2 = 0, y2 = 0, w2 = 0, h2 = 0;
};

// Клавиатура. Сейчас у каждой клавиши один кейкод — базовый слой. Слои можно
// добавить позже отдельным полем (например, вектор кейкодов на слой с тем же
// порядком, что и keys), не трогая геометрию.
struct KeyboardLayout {
  const char *id;     // стабильный идентификатор для настроек: "ansi104"
  const char *name;   // название в списке «Клавиатура»
  const char *source; // откуда взята геометрия и keymap
  bool isSplit;       // сплит: показан default keymap QMK
  std::vector<KeyDef> keys;
};

// Все клавиатуры в порядке показа в списке. Новая клавиатура — это новая
// таблица KeyboardLayout и одна строка в allKeyboards().
const std::vector<const KeyboardLayout *> &allKeyboards();
const KeyboardLayout *findKeyboard(const std::string &id);

// Подпись клавиши с учётом значения по умолчанию
const char *labelOf(const KeyDef &key);

// Набор statisticKey() всех клавиш клавиатуры — для списка «Нет на этой
// клавиатуре»
std::set<KeyCode> presentStatKeys(const KeyboardLayout &layout);

// Геометрия в единицах клавиши с учётом поворота
struct Point {
  double x = 0, y = 0;
};
using Quad = std::array<Point, 4>; // по часовой стрелке от левого верхнего

// Поворот точки на r градусов по часовой стрелке (ось y направлена вниз)
// вокруг (rx, ry) — как в KLE
Point rotatePoint(Point p, double r, double rx, double ry);
// Одна или две части клавиши (вторая — у ISO Enter) после поворота
std::vector<Quad> keyQuads(const KeyDef &key);
// Центр основной части клавиши после поворота — точка для подписи
Point keyCenter(const KeyDef &key);
bool pointInQuad(const Quad &quad, Point p);
bool pointInKey(const KeyDef &key, Point p);

struct Bounds {
  double minX = 0, minY = 0, maxX = 0, maxY = 0;
  double width() const { return maxX - minX; }
  double height() const { return maxY - minY; }
};
Bounds layoutBounds(const KeyboardLayout &layout);

// Таблицы клавиатур (Heatmap/Layouts)
const KeyboardLayout &ansi104Layout();
const KeyboardLayout &ansiTklLayout();
const KeyboardLayout &iso105Layout();
const KeyboardLayout &sofleLayout();
const KeyboardLayout &corneLayout();
const KeyboardLayout &lily58Layout();

} // namespace heatmap
