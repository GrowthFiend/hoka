#include "KeyStats.h"

#include <algorithm>
#include <cmath>

namespace heatmap {

namespace {

struct ColorStop {
  double t;
  Rgb color;
};

// Тёплая шкала (по мотивам ColorBrewer OrRd): светлая -> красно-оранжевая
const ColorStop kKeyStops[] = {
    {0.00, {254, 232, 200}}, // #FEE8C8
    {0.35, {253, 187, 132}}, // #FDBB84
    {0.70, {252, 141, 89}},  // #FC8D59
    {1.00, {215, 48, 31}},   // #D7301F
};

// Синяя шкала (по мотивам ColorBrewer Blues)
const ColorStop kModifierStops[] = {
    {0.00, {198, 219, 239}}, // #C6DBEF
    {0.35, {158, 202, 225}}, // #9ECAE1
    {0.70, {66, 146, 198}},  // #4292C6
    {1.00, {8, 81, 156}},    // #08519C
};

uint8_t lerpChannel(uint8_t a, uint8_t b, double f) {
  return static_cast<uint8_t>(std::lround(a + (b - a) * f));
}

template <size_t N> Rgb interpolate(const ColorStop (&stops)[N], double t) {
  t = std::clamp(t, 0.0, 1.0);
  for (size_t i = 1; i < N; ++i) {
    if (t <= stops[i].t) {
      const ColorStop &lo = stops[i - 1];
      const ColorStop &hi = stops[i];
      double f = (t - lo.t) / (hi.t - lo.t);
      return {lerpChannel(lo.color.r, hi.color.r, f),
              lerpChannel(lo.color.g, hi.color.g, f),
              lerpChannel(lo.color.b, hi.color.b, f)};
    }
  }
  return stops[N - 1].color;
}

double channelLuminance(uint8_t c) {
  double s = c / 255.0;
  return s <= 0.04045 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
}

int index(KeyCode code) { return static_cast<int>(code); }

} // namespace

int64_t HeatmapStats::countFor(KeyCode physicalKey) const {
  if (isNonStatisticKey(physicalKey)) {
    return 0;
  }
  KeyCode stat = statisticKey(physicalKey);
  if (isModifier(stat)) {
    return includeModifiers ? modifierCounts[index(stat)] : 0;
  }
  return keyCounts[index(stat)];
}

HeatmapStats computeHeatmapStats(const KeyCountRows &rows,
                                 bool includeModifiers) {
  HeatmapStats stats;
  stats.includeModifiers = includeModifiers;

  for (const auto &[combination, pressCount] : rows) {
    if (pressCount <= 0) {
      continue;
    }
    const ParsedCombination parsed = parseCombination(combination);
    const std::optional<KeyCode> main = keyFromDbString(parsed.mainKey);

    uint8_t modifiers = parsed.modifiers;
    if (!main) {
      // Основная клавиша не распознана: на карту она не попадает, но
      // модификаторы из префиксов в сочетании всё равно были
      stats.unrecognizedCombinations++;
      stats.unrecognizedPresses += pressCount;
    } else if (isModifier(*main)) {
      // Старые версии записывали и одиночные модификаторы ("Ctrl",
      // "Shift+Ctrl"): это сочетание только из модификаторов
      modifiers |= modifierFlagOf(*main);
    } else {
      stats.keyCounts[index(statisticKey(*main))] += pressCount;
    }

    if (includeModifiers) {
      for (ModifierFlag flag : {ModCtrl, ModShift, ModAlt, ModWin}) {
        if (modifiers & flag) {
          stats.modifierCounts[index(modifierKeyOf(flag))] += pressCount;
        }
      }
    }
  }

  stats.maxKeyCount =
      *std::max_element(stats.keyCounts.begin(), stats.keyCounts.end());
  stats.maxModifierCount = *std::max_element(stats.modifierCounts.begin(),
                                             stats.modifierCounts.end());
  return stats;
}

double intensity(int64_t count, int64_t max) {
  if (count <= 0 || max <= 0) {
    return 0.0;
  }
  double value = std::log1p(static_cast<double>(count)) /
                 std::log1p(static_cast<double>(max));
  return std::clamp(value, 0.0, 1.0);
}

Rgb neutralColor() { return {226, 226, 226}; } // #E2E2E2

Rgb scaleColor(ColorScale scale, double t) {
  return scale == ColorScale::Keys ? interpolate(kKeyStops, t)
                                   : interpolate(kModifierStops, t);
}

double relativeLuminance(Rgb c) {
  return 0.2126 * channelLuminance(c.r) + 0.7152 * channelLuminance(c.g) +
         0.0722 * channelLuminance(c.b);
}

double contrastRatio(Rgb a, Rgb b) {
  double la = relativeLuminance(a);
  double lb = relativeLuminance(b);
  if (la < lb) {
    std::swap(la, lb);
  }
  return (la + 0.05) / (lb + 0.05);
}

Rgb labelColorFor(Rgb background) {
  const Rgb black{0, 0, 0};
  const Rgb white{255, 255, 255};
  return contrastRatio(background, black) >= contrastRatio(background, white)
             ? black
             : white;
}

KeyAppearance appearanceFor(KeyCode physicalKey, const HeatmapStats &stats) {
  KeyAppearance a;
  a.statKey = statisticKey(physicalKey);
  a.isModifier = isModifier(a.statKey);
  a.disabled = isNonStatisticKey(physicalKey) ||
               (a.isModifier && !stats.includeModifiers);
  a.count = a.disabled ? 0 : stats.countFor(physicalKey);

  if (a.count > 0) {
    if (a.isModifier) {
      a.intensity = intensity(a.count, stats.maxModifierCount);
      a.fill = scaleColor(ColorScale::Modifiers, a.intensity);
    } else {
      a.intensity = intensity(a.count, stats.maxKeyCount);
      a.fill = scaleColor(ColorScale::Keys, a.intensity);
    }
  } else {
    a.fill = neutralColor();
  }
  a.text = labelColorFor(a.fill);
  return a;
}

std::vector<MissingKey> missingKeys(const HeatmapStats &stats,
                                    const std::set<KeyCode> &presentStatKeys) {
  std::vector<MissingKey> result;
  for (int i = 0; i < kKeyCodeCount; ++i) {
    const KeyCode code = static_cast<KeyCode>(i);
    const bool modifier = isModifier(code);
    const int64_t count =
        modifier ? (stats.includeModifiers ? stats.modifierCounts[i] : 0)
                 : stats.keyCounts[i];
    if (count > 0 && presentStatKeys.count(code) == 0) {
      result.push_back({code, count, modifier});
    }
  }
  std::stable_sort(result.begin(), result.end(),
                   [](const MissingKey &a, const MissingKey &b) {
                     return a.count > b.count;
                   });
  return result;
}

} // namespace heatmap
