cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Vcpkg target triplet")
project(hoka VERSION 0.1.0 LANGUAGES C CXX)
set(fltk_DIR "C:/Users/Jeronimo/Documents/projects/hoka/vcpkg_installed/x64-windows/share/fltk")
find_package(fltk CONFIG REQUIRED)

add_executable(hoka WIN32 main.cpp)

target_link_libraries(hoka PRIVATE fltk)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
