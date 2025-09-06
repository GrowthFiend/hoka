cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Vcpkg target triplet")

project(hoka VERSION 0.1.0 LANGUAGES C CXX)

# Ищем пакеты с правильными именами
find_package(FLTK CONFIG REQUIRED)  # Обратите внимание: FLTK, а не fltk
find_package(unofficial-sqlite3 CONFIG REQUIRED)  # Для SQLite3

add_executable(hoka WIN32 main.cpp)

# Правильное указание целей (как указано в выводе vcpkg)
target_link_libraries(hoka PRIVATE 
    fltk  # Основная библиотека FLTK
    fltk_gl  # OpenGL поддержка
    fltk_forms  # Формы
    fltk_images  # Изображения
    unofficial::sqlite3::sqlite3  # SQLite3
)

# Дополнительные библиотеки для Windows
if(WIN32)
    target_link_libraries(hoka PRIVATE psapi user32)
endif()

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)