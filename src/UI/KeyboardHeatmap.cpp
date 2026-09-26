// FL/fl_draw.H на Windows подключает windows.h; без NOMINMAX его макросы
// min/max ломают std::min/std::max ниже
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "KeyboardHeatmap.h"

#include <FL/Fl.H>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cmath>

// Весь модуль heatmap, но KeyCode пишем с префиксом: в X11 (FL/x.H на
// Linux) есть глобальный typedef KeyCode
using namespace heatmap;

namespace {

const int kPad = 6;          // отступ от краёв виджета
const int kFontSize = 11;    // шрифт легенды и списка
const int kRowHeight = 18;   // строка легенды
const int kChipHeight = 16;  // «фишка» в списке «Нет на этой клавиатуре»
const int kChipGap = 4;
const int kBarWidth = 70;    // градиент в легенде
const int kBarHeight = 10;
const int kMaxMissingLines = 3;
const double kMaxUnit = 64.0; // крупнее клавиши не рисуем даже на большом экране

const char *const kMissingTitle = "Нет на этой клавиатуре:";

Fl_Color toFl(Rgb c) { return fl_rgb_color(c.r, c.g, c.b); }

// 12345 -> "12 345"
std::string formatCount(int64_t value) {
  std::string digits = std::to_string(value);
  std::string result;
  int n = static_cast<int>(digits.size());
  for (int i = 0; i < n; ++i) {
    result += digits[i];
    int rest = n - 1 - i;
    if (rest > 0 && rest % 3 == 0 && digits[i] != '-') {
      result += ' ';
    }
  }
  return result;
}

bool isNumpadKey(heatmap::KeyCode code) {
  return (code >= KC_PSLS && code <= KC_PDOT) || code == KC_NUM;
}

// Короткое имя клавиши для списка «Нет на этой клавиатуре»
std::string shortName(heatmap::KeyCode code) {
  if (isNumpadKey(code) && code != KC_NUM) {
    return std::string("Num ") + keyLabel(code);
  }
  if (code == KC_NUBS) {
    return "ISO <>";
  }
  if (isMediaKey(code)) {
    return keyDescription(code);
  }
  return keyLabel(code);
}

int textWidth(const std::string &text) {
  return static_cast<int>(std::ceil(fl_width(text.c_str())));
}

// Раскладка «фишек» по строкам: сколько строк нужно при данной ширине
int flowLines(const std::vector<int> &widths, int width) {
  int lines = 1;
  int x = 0;
  for (int w : widths) {
    if (x > 0 && x + w > width) {
      ++lines;
      x = 0;
    }
    x += w + kChipGap;
  }
  return lines;
}

} // namespace

KeyboardHeatmap::KeyboardHeatmap(int X, int Y, int W, int H, const char *L)
    : Fl_Widget(X, Y, W, H, L) {
  box(FL_FLAT_BOX);
  color(FL_WHITE);
  recompute();
}

KeyboardHeatmap::~KeyboardHeatmap() {
  if (Fl_Tooltip::current() == this) {
    Fl_Tooltip::current(nullptr);
  }
}

void KeyboardHeatmap::setKeyboard(const KeyboardLayout *layout) {
  if (layout == layout_) {
    return;
  }
  layout_ = layout;
  hoveredKey_ = -1;
  recompute();
}

void KeyboardHeatmap::setKeyCounts(const std::string &appName,
                                   KeyCountRows rows) {
  appName_ = appName;
  rows_ = std::move(rows);
  recompute();
}

void KeyboardHeatmap::setIncludeModifiers(bool include) {
  if (include == includeModifiers_) {
    return;
  }
  includeModifiers_ = include;
  recompute();
}

void KeyboardHeatmap::recompute() {
  stats_ = computeHeatmapStats(rows_, includeModifiers_);

  appearances_.clear();
  keyTooltips_.clear();
  missing_.clear();

  const std::string splitNote =
      "\n\nПоказан базовый слой default keymap QMK \xE2\x80\x94 у вас "
      "раскладка может отличаться.";

  if (layout_) {
    for (const KeyDef &key : layout_->keys) {
      KeyAppearance a = appearanceFor(key.code, stats_);
      appearances_.push_back(a);

      std::string tip;
      if (key.code == KC_LAYER) {
        tip = std::string(labelOf(key)) +
              "\nПереключение слоя QMK \xE2\x80\x94 не считается";
      } else if (key.code == KC_NO) {
        tip = "Клавиша без назначения (KC_NO)";
      } else {
        tip = std::string(keyDescription(key.code)) + " (" +
              keyCodeName(key.code) + ")";
        if (a.isModifier && !includeModifiers_) {
          tip += "\nМодификаторы не учитываются";
        } else if (a.isModifier) {
          tip += "\nВ сочетаниях: " + formatCount(a.count) +
                 "\n(счёт модификатора \xE2\x80\x94 число сочетаний с ним; "
                 "одиночные нажатия модификаторов не записываются)";
        } else if (a.count > 0) {
          tip += "\nНажатий: " + formatCount(a.count);
        } else {
          tip += "\nНажатий нет";
        }
      }
      if (layout_->isSplit) {
        tip += splitNote;
      }
      keyTooltips_.push_back(tip);
    }
    missing_ = missingKeys(stats_, presentStatKeys(*layout_));
  }

  missingTooltip_.clear();
  for (const MissingKey &m : missing_) {
    if (!missingTooltip_.empty()) {
      missingTooltip_ += '\n';
    }
    missingTooltip_ += shortName(m.key) + ": " + formatCount(m.count);
    if (m.isModifier) {
      missingTooltip_ += " (в сочетаниях)";
    }
  }
  if (!missingTooltip_.empty()) {
    missingTooltip_ =
        "Клавиши со статистикой, которых нет на этой клавиатуре:\n" +
        missingTooltip_;
    if (layout_ && layout_->isSplit) {
      missingTooltip_ += splitNote;
    }
  }

  legendTooltip_ =
      "Цвет клавиши \xE2\x80\x94 логарифм числа нажатий относительно самой "
      "частой клавиши.\nОбычные клавиши и модификаторы нормируются отдельно.\n"
      "Счёт модификатора \xE2\x80\x94 число сочетаний с ним: одиночные "
      "нажатия модификаторов не записываются.";

  // Текст подсказки сменился: если она показана, перестроим её при
  // следующем движении мыши
  tooltipTarget_ = -2;
  redraw();
}

// ---------------------------------------------------------------------------
// Отрисовка
// ---------------------------------------------------------------------------

std::vector<std::string> KeyboardHeatmap::missingItems() const {
  std::vector<std::string> items;
  for (const MissingKey &m : missing_) {
    items.push_back(shortName(m.key) + ": " + formatCount(m.count));
  }
  return items;
}

int KeyboardHeatmap::footerHeight(int width) const {
  fl_font(FL_HELVETICA, kFontSize);
  int height = 0;

  // Легенда: одна строка, если обе шкалы помещаются, иначе две
  const int scaleWidth = kBarWidth + 90 + textWidth("Модификаторы (сочетаний):");
  height += (width >= 2 * scaleWidth) ? kRowHeight : 2 * kRowHeight;

  if (!missing_.empty()) {
    std::vector<int> widths = {textWidth(kMissingTitle)};
    for (const std::string &item : missingItems()) {
      widths.push_back(textWidth(item) + 8);
    }
    int lines = std::min(flowLines(widths, width), kMaxMissingLines);
    height += 4 + lines * (kChipHeight + 2);
  }
  if (stats_.unrecognizedCombinations > 0) {
    height += kRowHeight;
  }
  return height;
}

void KeyboardHeatmap::draw() {
  fl_push_clip(x(), y(), w(), h());
  fl_color(color());
  fl_rectf(x(), y(), w(), h());

  const int innerW = std::max(0, w() - 2 * kPad);
  const int footer = footerHeight(innerW);
  const int keyboardH = h() - footer - 3 * kPad;

  unit_ = 0;
  if (layout_ && keyboardH > 10 && innerW > 10) {
    drawKeyboard(x() + kPad, y() + kPad, innerW, keyboardH);
  }

  int fy = y() + h() - kPad - footer;
  drawLegend(x() + kPad, fy, innerW);
  fl_font(FL_HELVETICA, kFontSize);
  const int scaleWidth = kBarWidth + 90 + textWidth("Модификаторы (сочетаний):");
  fy += (innerW >= 2 * scaleWidth) ? kRowHeight : 2 * kRowHeight;
  drawMissing(x() + kPad, fy, innerW, y() + h() - kPad - fy);

  fl_pop_clip();
}

void KeyboardHeatmap::drawKeyboard(int X, int Y, int W, int H) {
  const Bounds b = layoutBounds(*layout_);
  if (b.width() <= 0 || b.height() <= 0) {
    return;
  }
  unit_ = std::min({W / b.width(), H / b.height(), kMaxUnit});
  originX_ = X + (W - b.width() * unit_) / 2.0 - b.minX * unit_;
  originY_ = Y + (H - b.height() * unit_) / 2.0 - b.minY * unit_;

  for (size_t i = 0; i < layout_->keys.size(); ++i) {
    if (static_cast<int>(i) != hoveredKey_) {
      drawKey(i);
    }
  }
  // Клавишу под курсором рисуем последней, чтобы рамка была поверх соседей
  if (hoveredKey_ >= 0 && hoveredKey_ < static_cast<int>(layout_->keys.size())) {
    drawKey(static_cast<size_t>(hoveredKey_));
  }
}

void KeyboardHeatmap::drawKey(size_t index) {
  const KeyDef &key = layout_->keys[index];
  const KeyAppearance &a = appearances_[index];
  const bool hovered = static_cast<int>(index) == hoveredKey_;
  const Fl_Color fill = toFl(a.fill);
  const Fl_Color border = hovered ? FL_BLACK : fl_rgb_color(140, 140, 140);
  const int thickness = hovered ? 2 : 1;
  // Зазор между клавишами, в единицах клавиши
  const double gap = std::max(1.0, unit_ * 0.05) / unit_;

  if (key.r == 0) {
    // Неповёрнутая клавиша: прямоугольники (у ISO Enter их два)
    struct PixelRect {
      int x, y, w, h;
    };
    std::vector<PixelRect> parts;
    auto addPart = [&](double kx, double ky, double kw, double kh) {
      int x0 = static_cast<int>(std::lround(originX_ + (kx + gap) * unit_));
      int y0 = static_cast<int>(std::lround(originY_ + (ky + gap) * unit_));
      int x1 = static_cast<int>(std::lround(originX_ + (kx + kw - gap) * unit_));
      int y1 = static_cast<int>(std::lround(originY_ + (ky + kh - gap) * unit_));
      parts.push_back({x0, y0, std::max(1, x1 - x0), std::max(1, y1 - y0)});
    };
    addPart(key.x, key.y, key.w, key.h);
    if (key.w2 > 0 && key.h2 > 0) {
      addPart(key.x + key.x2, key.y + key.y2, key.w2, key.h2);
    }

    fl_color(fill);
    for (const PixelRect &p : parts) {
      fl_rectf(p.x, p.y, p.w, p.h);
    }
    fl_color(border);
    for (const PixelRect &p : parts) {
      for (int t = 0; t < thickness; ++t) {
        fl_rect(p.x + t, p.y + t, p.w - 2 * t, p.h - 2 * t);
      }
    }
    if (parts.size() > 1) {
      // Стираем внутренние линии там, где части перекрываются
      fl_color(fill);
      for (const PixelRect &p : parts) {
        fl_rectf(p.x + thickness, p.y + thickness, p.w - 2 * thickness,
                 p.h - 2 * thickness);
      }
    }
  } else {
    // Повёрнутая клавиша: многоугольник в системе координат клавиатуры.
    // В KLE и QMK угол положительный по часовой стрелке, а fl_rotate()
    // поворачивает против часовой, поэтому знак меняется.
    const double x0 = key.x + gap, y0 = key.y + gap;
    const double x1 = key.x + key.w - gap, y1 = key.y + key.h - gap;
    fl_push_matrix();
    fl_translate(originX_, originY_);
    fl_scale(unit_);
    fl_translate(key.rx, key.ry);
    fl_rotate(-key.r);
    fl_translate(-key.rx, -key.ry);

    fl_color(fill);
    fl_begin_complex_polygon();
    fl_vertex(x0, y0);
    fl_vertex(x1, y0);
    fl_vertex(x1, y1);
    fl_vertex(x0, y1);
    fl_end_complex_polygon();

    fl_color(border);
    fl_line_style(FL_SOLID, thickness);
    fl_begin_loop();
    fl_vertex(x0, y0);
    fl_vertex(x1, y0);
    fl_vertex(x1, y1);
    fl_vertex(x0, y1);
    fl_end_loop();
    fl_line_style(0);
    fl_pop_matrix();
  }

  // Подпись — горизонтально в центре клавиши
  const Point c = keyCenter(key);
  const int cx = static_cast<int>(std::lround(originX_ + c.x * unit_));
  const int cy = static_cast<int>(std::lround(originY_ + c.y * unit_));
  const int boxW = static_cast<int>(key.w * unit_) - 4;
  const int boxH = static_cast<int>(key.h * unit_) - 2;
  const char *label = labelOf(key);
  if (!label || !*label || boxW < 6) {
    return;
  }

  int fontSize = std::clamp(static_cast<int>(unit_ * 0.30), 6, 14);
  fl_font(FL_HELVETICA, fontSize);
  while (fontSize > 6 && fl_width(label) > boxW) {
    fl_font(FL_HELVETICA, --fontSize);
  }
  if (fl_width(label) > boxW) {
    return; // даже мелким шрифтом не помещается: подробности в подсказке
  }

  fl_color(toFl(a.text));
  const bool showCount = a.count > 0 && unit_ >= 36;
  if (showCount) {
    const int countSize = std::max(6, fontSize - 2);
    fl_draw(label, cx - boxW / 2, cy - fontSize - 1, boxW, fontSize + 2,
            FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    fl_font(FL_HELVETICA, countSize);
    std::string count = formatCount(a.count);
    if (fl_width(count.c_str()) <= boxW) {
      fl_draw(count.c_str(), cx - boxW / 2, cy + 1, boxW, countSize + 2,
              FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    }
  } else {
    fl_draw(label, cx - boxW / 2, cy - boxH / 2, boxW, boxH,
            FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
  }
}

void KeyboardHeatmap::drawLegend(int X, int Y, int W) {
  fl_font(FL_HELVETICA, kFontSize);
  const int scaleWidth = kBarWidth + 90 + textWidth("Модификаторы (сочетаний):");
  const bool oneRow = W >= 2 * scaleWidth;
  legendArea_ = {X, Y, oneRow ? 2 * scaleWidth : scaleWidth,
                 oneRow ? kRowHeight : 2 * kRowHeight};

  auto drawScale = [&](int sx, int sy, const char *title, ColorScale scale,
                       int64_t max, bool enabled) {
    const int baseline = sy + kRowHeight / 2 + kFontSize / 2 - 1;
    fl_color(FL_BLACK);
    fl_font(FL_HELVETICA, kFontSize);
    fl_draw(title, sx, baseline);
    int x = sx + textWidth(title) + 6;
    const int barY = sy + (kRowHeight - kBarHeight) / 2;

    if (!enabled) {
      fl_color(toFl(neutralColor()));
      fl_rectf(x, barY, 14, kBarHeight);
      fl_color(fl_rgb_color(140, 140, 140));
      fl_rect(x, barY, 14, kBarHeight);
      fl_color(FL_DARK3);
      fl_draw("не учитываются", x + 20, baseline);
      return;
    }
    if (max <= 0) {
      fl_color(FL_DARK3);
      fl_draw("нет нажатий", x, baseline);
      return;
    }

    fl_color(FL_DARK3);
    fl_draw("мало", x, baseline);
    x += textWidth("мало") + 4;
    for (int i = 0; i < kBarWidth; ++i) {
      fl_color(toFl(scaleColor(scale, i / double(kBarWidth - 1))));
      fl_yxline(x + i, barY, barY + kBarHeight - 1);
    }
    fl_color(fl_rgb_color(140, 140, 140));
    fl_rect(x - 1, barY - 1, kBarWidth + 2, kBarHeight + 2);
    x += kBarWidth + 4;
    fl_color(FL_DARK3);
    std::string most = "много (" + formatCount(max) + ")";
    fl_draw(most.c_str(), x, baseline);
  };

  drawScale(X, Y, "Клавиши:", ColorScale::Keys, stats_.maxKeyCount, true);
  if (oneRow) {
    drawScale(X + scaleWidth, Y, "Модификаторы (сочетаний):",
              ColorScale::Modifiers, stats_.maxModifierCount,
              includeModifiers_);
  } else {
    drawScale(X, Y + kRowHeight, "Модификаторы (сочетаний):",
              ColorScale::Modifiers, stats_.maxModifierCount,
              includeModifiers_);
  }

  if (appName_.empty()) {
    // Нет выбранного приложения: подсказка вместо пустой легенды справа
    fl_color(FL_DARK3);
    fl_font(FL_HELVETICA_ITALIC, kFontSize);
    fl_draw("Выберите приложение", X, Y, W, kRowHeight,
            FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
  }
}

void KeyboardHeatmap::drawMissing(int X, int Y, int W, int H) {
  missingArea_ = {};
  fl_font(FL_HELVETICA, kFontSize);
  int y = Y;

  if (!missing_.empty()) {
    y += 4;
    const std::vector<std::string> items = missingItems();
    std::vector<int> widths = {textWidth(kMissingTitle)};
    for (const std::string &item : items) {
      widths.push_back(textWidth(item) + 8);
    }

    // Сколько фишек показать, чтобы уложиться в kMaxMissingLines строк
    size_t shown = items.size();
    std::string more;
    if (flowLines(widths, W) > kMaxMissingLines) {
      while (shown > 0) {
        --shown;
        more = "\xE2\x80\xA6 ещё " + std::to_string(items.size() - shown);
        std::vector<int> trial(widths.begin(), widths.begin() + 1 + shown);
        trial.push_back(textWidth(more) + 8);
        if (flowLines(trial, W) <= kMaxMissingLines) {
          break;
        }
      }
    }

    const int lineH = kChipHeight + 2;
    int x = X;
    int line = 0;
    auto place = [&](int w) {
      if (x > X && x + w > X + W) {
        ++line;
        x = X;
      }
      int px = x;
      x += w + kChipGap;
      return px;
    };

    // Заголовок
    int px = place(widths[0]);
    fl_color(FL_BLACK);
    fl_draw(kMissingTitle, px, y + line * lineH + kChipHeight / 2 + kFontSize / 2 - 1);

    for (size_t i = 0; i < shown; ++i) {
      const MissingKey &m = missing_[i];
      const int w = widths[i + 1];
      px = place(w);
      const int py = y + line * lineH;
      const int64_t max = m.isModifier ? stats_.maxModifierCount : stats_.maxKeyCount;
      const Rgb bg = scaleColor(m.isModifier ? ColorScale::Modifiers
                                             : ColorScale::Keys,
                                intensity(m.count, max));
      fl_color(toFl(bg));
      fl_rectf(px, py, w, kChipHeight);
      fl_color(fl_rgb_color(140, 140, 140));
      fl_rect(px, py, w, kChipHeight);
      fl_color(toFl(labelColorFor(bg)));
      fl_draw(items[i].c_str(), px, py, w, kChipHeight,
              FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    }
    if (shown < items.size()) {
      const int w = textWidth(more) + 8;
      px = place(w);
      const int py = y + line * lineH;
      fl_color(FL_DARK3);
      fl_draw(more.c_str(), px, py, w, kChipHeight,
              FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    }
    missingArea_ = {X, y, W, (line + 1) * lineH};
    y += (line + 1) * lineH;
  }

  if (stats_.unrecognizedCombinations > 0 && y + kRowHeight <= Y + H + 2) {
    std::string text = "Не распознано сочетаний: " +
                       formatCount(stats_.unrecognizedCombinations) +
                       " (нажатий: " + formatCount(stats_.unrecognizedPresses) +
                       ") \xE2\x80\x94 на карте не показаны";
    fl_color(FL_DARK3);
    fl_font(FL_HELVETICA_ITALIC, kFontSize);
    fl_draw(text.c_str(), X, y, W, kRowHeight, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  }
}

// ---------------------------------------------------------------------------
// Мышь и подсказки
// ---------------------------------------------------------------------------

int KeyboardHeatmap::keyAt(int px, int py) const {
  if (!layout_ || unit_ <= 0) {
    return -1;
  }
  const Point p{(px - originX_) / unit_, (py - originY_) / unit_};
  // Сначала клавиша под курсором: у повёрнутых клавиш соседи могут
  // перекрываться ограничивающими прямоугольниками
  for (size_t i = layout_->keys.size(); i-- > 0;) {
    if (pointInKey(layout_->keys[i], p)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void KeyboardHeatmap::updateTooltip(int px, int py) {
  int target = -2;
  const char *tip = nullptr;
  Area area;

  const int key = keyAt(px, py);
  if (key >= 0) {
    target = key;
    tip = keyTooltips_[key].c_str();
    // Подсказка держится, пока курсор в ограничивающем прямоугольнике клавиши
    double minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const Quad &q : keyQuads(layout_->keys[key])) {
      for (const Point &p : q) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
      }
    }
    area = {static_cast<int>(originX_ + minX * unit_),
            static_cast<int>(originY_ + minY * unit_),
            static_cast<int>((maxX - minX) * unit_) + 1,
            static_cast<int>((maxY - minY) * unit_) + 1};
  } else if (missingArea_.contains(px, py) && !missingTooltip_.empty()) {
    target = -1;
    tip = missingTooltip_.c_str();
    area = missingArea_;
  } else if (legendArea_.contains(px, py)) {
    target = -3;
    tip = legendTooltip_.c_str();
    area = legendArea_;
  }

  if (target == tooltipTarget_) {
    return;
  }
  tooltipTarget_ = target;
  if (!tip) {
    Fl_Tooltip::enter_area(this, 0, 0, 0, 0, nullptr);
    return;
  }
  Fl_Tooltip::enter_area(this, area.x, area.y, area.w, area.h, tip);
}

int KeyboardHeatmap::handle(int event) {
  switch (event) {
  case FL_ENTER:
  case FL_MOVE: {
    const int key = keyAt(Fl::event_x(), Fl::event_y());
    if (key != hoveredKey_) {
      hoveredKey_ = key;
      redraw();
    }
    updateTooltip(Fl::event_x(), Fl::event_y());
    return 1;
  }
  case FL_LEAVE:
  case FL_HIDE:
    if (hoveredKey_ != -1) {
      hoveredKey_ = -1;
      redraw();
    }
    tooltipTarget_ = -2;
    Fl_Tooltip::exit(this);
    return event == FL_LEAVE ? 1 : Fl_Widget::handle(event);
  default:
    return Fl_Widget::handle(event);
  }
}
