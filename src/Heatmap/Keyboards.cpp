#include "Keyboards.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace heatmap {

const std::vector<const KeyboardLayout *> &allKeyboards() {
  static const std::vector<const KeyboardLayout *> keyboards = {
      &ansi104Layout(), &ansiTklLayout(), &iso105Layout(),
      &sofleLayout(),   &corneLayout(),   &lily58Layout(),
  };
  return keyboards;
}

const KeyboardLayout *findKeyboard(const std::string &id) {
  for (const KeyboardLayout *layout : allKeyboards()) {
    if (id == layout->id) {
      return layout;
    }
  }
  return nullptr;
}

const char *labelOf(const KeyDef &key) {
  return key.label ? key.label : keyLabel(key.code);
}

std::set<KeyCode> presentStatKeys(const KeyboardLayout &layout) {
  std::set<KeyCode> keys;
  for (const KeyDef &key : layout.keys) {
    if (!isNonStatisticKey(key.code)) {
      keys.insert(statisticKey(key.code));
    }
  }
  return keys;
}

Point rotatePoint(Point p, double r, double rx, double ry) {
  if (r == 0.0) {
    return p;
  }
  // M_PI не определён в строгом режиме стандарта, поэтому своя константа
  constexpr double kPi = 3.14159265358979323846;
  const double a = r * kPi / 180.0;
  const double c = std::cos(a);
  const double s = std::sin(a);
  const double dx = p.x - rx;
  const double dy = p.y - ry;
  return {rx + dx * c - dy * s, ry + dx * s + dy * c};
}

namespace {

Quad rotatedRect(const KeyDef &key, double x, double y, double w, double h) {
  Quad q = {Point{x, y}, Point{x + w, y}, Point{x + w, y + h},
            Point{x, y + h}};
  for (Point &p : q) {
    p = rotatePoint(p, key.r, key.rx, key.ry);
  }
  return q;
}

} // namespace

std::vector<Quad> keyQuads(const KeyDef &key) {
  std::vector<Quad> quads;
  quads.push_back(rotatedRect(key, key.x, key.y, key.w, key.h));
  if (key.w2 > 0 && key.h2 > 0) {
    quads.push_back(
        rotatedRect(key, key.x + key.x2, key.y + key.y2, key.w2, key.h2));
  }
  return quads;
}

Point keyCenter(const KeyDef &key) {
  return rotatePoint({key.x + key.w / 2.0, key.y + key.h / 2.0}, key.r, key.rx,
                     key.ry);
}

bool pointInQuad(const Quad &quad, Point p) {
  // Луч вправо: считаем пересечения с рёбрами
  bool inside = false;
  for (size_t i = 0, j = quad.size() - 1; i < quad.size(); j = i++) {
    const Point &a = quad[i];
    const Point &b = quad[j];
    if ((a.y > p.y) != (b.y > p.y)) {
      const double xCross = a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y);
      if (p.x < xCross) {
        inside = !inside;
      }
    }
  }
  return inside;
}

bool pointInKey(const KeyDef &key, Point p) {
  for (const Quad &quad : keyQuads(key)) {
    if (pointInQuad(quad, p)) {
      return true;
    }
  }
  return false;
}

Bounds layoutBounds(const KeyboardLayout &layout) {
  if (layout.keys.empty()) {
    return {};
  }
  Bounds b{std::numeric_limits<double>::max(),
           std::numeric_limits<double>::max(),
           std::numeric_limits<double>::lowest(),
           std::numeric_limits<double>::lowest()};
  for (const KeyDef &key : layout.keys) {
    for (const Quad &quad : keyQuads(key)) {
      for (const Point &p : quad) {
        b.minX = std::min(b.minX, p.x);
        b.minY = std::min(b.minY, p.y);
        b.maxX = std::max(b.maxX, p.x);
        b.maxY = std::max(b.maxY, p.y);
      }
    }
  }
  return b;
}

} // namespace heatmap
