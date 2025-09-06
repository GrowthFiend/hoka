cmake_minimum_required(VERSION 3.15)

# Vcpkg configuration
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Vcpkg target triplet")

project(hoka
    VERSION 0.1.0
    LANGUAGES C CXX
)

# C++ standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Package dependencies
find_package(FLTK CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Windows-specific resources
if(WIN32)
    enable_language(RC)
    set(RESOURCE_FILES 
        src/resource.rc 
        src/hoka.manifest 
    )
endif()

# Source files organization
set(SOURCES
    src/main.cpp
    src/Database/Database.cpp
    src/KeyLogger/KeyLogger.cpp
    src/UI/MainWindow.cpp
    src/UI/SystemTray.cpp
)

set(HEADERS
    src/Database/Database.h
    src/KeyLogger/KeyLogger.h
    src/UI/MainWindow.h
    src/UI/SystemTray.h
    src/Models/KeyStatistics.h
    src/Models/KeyPress.h
)

set(TEST_SOURCES
    Testing/main_test.cpp
    Testing/Database/DatabaseTests.cpp
)

# Main executable
add_executable(hoka WIN32 
    ${SOURCES}
    ${HEADERS}
    ${RESOURCE_FILES}
)

# Test executable
add_executable(hoka_tests 
    ${TEST_SOURCES}
    src/Database/Database.cpp
    ${HEADERS}
)

# Compile definitions
target_compile_definitions(hoka PRIVATE 
    UNICODE 
    _UNICODE 
    NTDDI_VERSION=0x06000000
)

target_compile_definitions(hoka_tests PRIVATE 
    UNICODE 
    _UNICODE 
    NTDDI_VERSION=0x06000000
)

# Include directories
target_include_directories(hoka PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(hoka_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(hoka PRIVATE 
    fltk 
    fltk_gl 
    fltk_forms 
    fltk_images 
    unofficial::sqlite3::sqlite3 
    psapi 
    user32 
    kernel32 
    shell32 
    gdi32 
    comctl32
)

target_link_libraries(hoka_tests PRIVATE 
    GTest::gtest 
    GTest::gtest_main 
    unofficial::sqlite3::sqlite3 
    psapi 
    user32 
    kernel32 
    shell32 
    gdi32 
    comctl32
)

# Tests configuration
add_test(NAME DatabaseTests COMMAND hoka_tests)

# Windows-specific properties
if(WIN32)
    set_target_properties(hoka PROPERTIES 
        WIN32_EXECUTABLE TRUE
    )
    
    if(MSVC)
        set_target_properties(hoka PROPERTIES 
            LINK_FLAGS "/MANIFEST:EMBED /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
        )
    else()
        set_target_properties(hoka PROPERTIES 
            LINK_FLAGS "-Wl,--subsystem,windows"
        )
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(MSVC)
            set_target_properties(hoka PROPERTIES 
                LINK_FLAGS "/SUBSYSTEM:WINDOWS"
            )
        endif()
    endif()
endif()

# Compiler warnings and options
if(MSVC)
    target_compile_options(hoka PRIVATE /W4)
    target_compile_definitions(hoka PRIVATE 
        _CRT_SECURE_NO_WARNINGS 
        _CRT_NONSTDC_NO_DEPRECATE
    )
else()
    target_compile_options(hoka PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
    target_compile_options(hoka_tests PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# Build type specific configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hoka PRIVATE 
        DEBUG 
        _DEBUG
    )
    target_compile_definitions(hoka_tests PRIVATE 
        DEBUG 
        _DEBUG
    )
    
    if(WIN32 AND NOT MSVC)
        set_target_properties(hoka PROPERTIES 
            LINK_FLAGS "-Wl,--subsystem,console"
        )
    endif()
else()
    target_compile_definitions(hoka PRIVATE NDEBUG)
    target_compile_definitions(hoka_tests PRIVATE NDEBUG)
endif()

# Source groups for IDE organization
source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})
source_group("Resource Files" FILES ${RESOURCE_FILES})
source_group("Database" FILES 
    src/Database/Database.cpp 
    src/Database/Database.h
)
source_group("KeyLogger" FILES 
    src/KeyLogger/KeyLogger.cpp 
    src/KeyLogger/KeyLogger.h
)
source_group("UI" FILES 
    src/UI/MainWindow.cpp 
    src/UI/MainWindow.h 
    src/UI/SystemTray.cpp 
    src/UI/SystemTray.h
)
source_group("Models" FILES 
    src/Models/KeyStatistics.h 
    src/Models/KeyPress.h
)
source_group("Test Files" FILES ${TEST_SOURCES})

# Installation configuration
if(WIN32)
    install(TARGETS hoka 
        RUNTIME DESTINATION bin
    )
    
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(
            \"${CMAKE_INSTALL_PREFIX}/bin/hoka.exe\" 
            \"\" 
            \"${CMAKE_BINARY_DIR};${CMAKE_PREFIX_PATH}/bin\"
        )
    ")
endif()

# Testing and packaging
include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)