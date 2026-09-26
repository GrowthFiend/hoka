#pragma once
#include "KeyCodes.h"

#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Подсчёт для тепловой карты: из пар (комбинация, count) — счётчики по
// логическим клавишам, модификаторы отдельно, нормировка и цвет.
// Модуль не зависит ни от FLTK, ни от Windows.
namespace heatmap {

using KeyCountRows = std::vector<std::pair<std::string, int>>;

struct HeatmapStats {
  // Обычные клавиши: ключ — statisticKey(), значение — сумма press_count всех
  // записей с этой основной клавишей (и «S», и «Ctrl+S» дают вклад в S)
  std::vector<int64_t> keyCounts = std::vector<int64_t>(kKeyCodeCount, 0);
  // Модификаторы: сумма press_count сочетаний, в которых модификатор есть.
  // Заполняется только при includeModifiers.
  std::vector<int64_t> modifierCounts = std::vector<int64_t>(kKeyCodeCount, 0);

  int64_t maxKeyCount = 0;
  int64_t maxModifierCount = 0;
  bool includeModifiers = true;

  // Записи, основную клавишу которых не удалось распознать
  int unrecognizedCombinations = 0;
  int64_t unrecognizedPresses = 0;

  // Счёт клавиши с учётом её типа (KC_RCTL -> KC_LCTL и т. п.).
  // Для выключенных модификаторов и служебных клавиш — 0.
  int64_t countFor(KeyCode physicalKey) const;
};

HeatmapStats computeHeatmapStats(const KeyCountRows &rows,
                                 bool includeModifiers);

// Логарифмическая шкала: log(1 + count) / log(1 + max), в пределах [0, 1]
double intensity(int64_t count, int64_t max);

struct Rgb {
  uint8_t r = 0, g = 0, b = 0;
  bool operator==(const Rgb &o) const {
    return r == o.r && g == o.g && b == o.b;
  }
  bool operator!=(const Rgb &o) const { return !(*this == o); }
};

enum class ColorScale {
  Keys,      // тёплая: от светлой до насыщенной красно-оранжевой
  Modifiers, // синяя
};

// Нейтральный светло-серый: клавиши без нажатий и выключенные модификаторы
Rgb neutralColor();
// Цвет шкалы для интенсивности t в [0, 1]
Rgb scaleColor(ColorScale scale, double t);
// Относительная яркость и контраст по WCAG 2.x
double relativeLuminance(Rgb c);
double contrastRatio(Rgb a, Rgb b);
// Чёрный или белый — что контрастнее на данном фоне
Rgb labelColorFor(Rgb background);

// Всё, что нужно, чтобы нарисовать одну физическую клавишу
struct KeyAppearance {
  KeyCode statKey = KeyCode::KC_NO; // клавиша, под которой ведётся счёт
  int64_t count = 0;
  double intensity = 0.0;
  bool isModifier = false;
  bool disabled = false; // модификатор при выключенном флажке или служебная клавиша
  Rgb fill;
  Rgb text;
};

KeyAppearance appearanceFor(KeyCode physicalKey, const HeatmapStats &stats);

// Клавиши со статистикой, которых нет среди presentStatKeys
// (набор statisticKey() клавиш выбранной клавиатуры). Сортировка: по
// убыванию счёта, при равенстве — по порядку KeyCode. Модификаторы попадают
// в список только при включённом флажке.
struct MissingKey {
  KeyCode key;
  int64_t count;
  bool isModifier;
};
std::vector<MissingKey> missingKeys(const HeatmapStats &stats,
                                    const std::set<KeyCode> &presentStatKeys);

} // namespace heatmap
