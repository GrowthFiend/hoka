#pragma once
#include <chrono>
#include <string>

struct KeyPress {
  std::string appName;
  std::string keyCombination;
  unsigned long timestamp;

  // Конструктор, определенный прямо в заголовке
  KeyPress(const std::string &app, const std::string &keys, unsigned long time)
      : appName(app), keyCombination(keys), timestamp(time) {}

  // Дополнительный конструктор с текущим временем
  KeyPress(const std::string &app, const std::string &keys)
      : appName(app), keyCombination(keys),
        timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count()) {}

  // Метод для удобного форматирования времени
  std::string getFormattedTime() const {
    // Можно добавить форматирование времени здесь
    return std::to_string(timestamp);
  }

  // Метод для сравнения (полезно для сортировки)
  bool operator<(const KeyPress &other) const {
    return timestamp < other.timestamp;
  }
};