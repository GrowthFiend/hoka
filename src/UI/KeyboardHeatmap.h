#pragma once
#include "Heatmap/KeyStats.h"
#include "Heatmap/Keyboards.h"

#include <FL/Fl_Widget.H>

#include <string>
#include <vector>

// Тепловая карта использования клавиш на выбранной физической клавиатуре.
// Только FLTK 1.3, без windows.h: виджет собирается и на Linux.
class KeyboardHeatmap : public Fl_Widget {
public:
  KeyboardHeatmap(int X, int Y, int W, int H, const char *L = nullptr);
  ~KeyboardHeatmap() override;

  void setKeyboard(const heatmap::KeyboardLayout *layout);
  const heatmap::KeyboardLayout *keyboard() const { return layout_; }

  // Пары (комбинация, число нажатий) выбранного приложения; пустое имя — нет
  // выбранного приложения
  void setKeyCounts(const std::string &appName, heatmap::KeyCountRows rows);

  void setIncludeModifiers(bool include);
  bool includeModifiers() const { return includeModifiers_; }

  const heatmap::HeatmapStats &stats() const { return stats_; }

  void draw() override;
  int handle(int event) override;

private:
  struct Area {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(int px, int py) const {
      return px >= x && py >= y && px < x + w && py < y + h;
    }
  };

  const heatmap::KeyboardLayout *layout_ = nullptr;
  std::string appName_;
  heatmap::KeyCountRows rows_;
  bool includeModifiers_ = true;

  // Производные от данных: пересчитываются в recompute()
  heatmap::HeatmapStats stats_;
  std::vector<heatmap::KeyAppearance> appearances_; // по клавишам layout_
  std::vector<std::string> keyTooltips_;             // по клавишам layout_
  std::vector<heatmap::MissingKey> missing_;
  std::string missingTooltip_;
  std::string legendTooltip_;

  // Геометрия последней отрисовки: для попадания мышью
  double originX_ = 0, originY_ = 0, unit_ = 0;
  Area missingArea_;
  Area legendArea_;
  int hoveredKey_ = -1;
  int tooltipTarget_ = -2; // -2 нет подсказки, -1 список, -3 легенда, >=0 клавиша

  void recompute();
  int keyAt(int px, int py) const;
  void updateTooltip(int px, int py);

  // Части отрисовки
  int footerHeight(int width) const;
  void drawKeyboard(int X, int Y, int W, int H);
  void drawKey(size_t index);
  void drawLegend(int X, int Y, int W);
  void drawMissing(int X, int Y, int W, int H);
  std::vector<std::string> missingItems() const;
};
