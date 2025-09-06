cmake_minimum_required(VERSION 3.15)

# ==============================================================================
# VCPKG CONFIGURATION
# ==============================================================================
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Vcpkg target triplet")

# ==============================================================================
# PROJECT SETUP
# ==============================================================================
project(hoka 
    VERSION 0.1.0 
    LANGUAGES C CXX
)

# ==============================================================================
# COMPILER SETTINGS
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# DEPENDENCIES
# ==============================================================================
find_package(FLTK CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# ==============================================================================
# WINDOWS-SPECIFIC CONFIGURATION
# ==============================================================================
if(WIN32)
    enable_language(RC)
    set(RESOURCE_FILES 
        src/resource.rc
        src/hoka.manifest
    )
endif()

# ==============================================================================
# SOURCE FILES
# ==============================================================================
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

# ==============================================================================
# TARGETS
# ==============================================================================
add_executable(hoka WIN32 
    ${SOURCES}
    ${HEADERS}
    ${RESOURCE_FILES}
)

add_executable(hoka_tests
    ${TEST_SOURCES}
    src/Database/Database.cpp
    ${HEADERS}
)

# ==============================================================================
# COMPILE DEFINITIONS
# ==============================================================================
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

# ==============================================================================
# INCLUDE DIRECTORIES
# ==============================================================================
target_include_directories(hoka PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(hoka_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ==============================================================================
# LINK LIBRARIES
# ==============================================================================
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

# ==============================================================================
# TESTS
# ==============================================================================
add_test(NAME DatabaseTests COMMAND hoka_tests)

# ==============================================================================
# WINDOWS-SPECIFIC TARGET PROPERTIES
# ==============================================================================
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
        set_target_properties(hoka_tests PROPERTIES
            LINK_FLAGS "-Wl,--subsystem,console"
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

# ==============================================================================
# COMPILER-SPECIFIC SETTINGS
# ==============================================================================
if(MSVC)
    target_compile_options(hoka PRIVATE /W4)
    target_compile_definitions(hoka PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
    )
else()
    target_compile_options(hoka PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(hoka_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ==============================================================================
# BUILD TYPE CONFIGURATION
# ==============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hoka PRIVATE DEBUG _DEBUG)
    target_compile_definitions(hoka_tests PRIVATE DEBUG _DEBUG)
    
    if(WIN32 AND NOT MSVC)
        set_target_properties(hoka PROPERTIES
            LINK_FLAGS "-Wl,--subsystem,console"
        )
    endif()
else()
    target_compile_definitions(hoka PRIVATE NDEBUG)
    target_compile_definitions(hoka_tests PRIVATE NDEBUG)
endif()

# ==============================================================================
# SOURCE GROUP ORGANIZATION
# ==============================================================================
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

# ==============================================================================
# INSTALLATION RULES
# ==============================================================================
if(WIN32)
    install(TARGETS hoka
        RUNTIME DESTINATION bin
    )
    
    # Install additional files (documentation, etc.)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION share/hoka
    )
    
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/hoka.exe\" \"\" \"${CMAKE_BINARY_DIR};${CMAKE_PREFIX_PATH}/bin\")
    ")
endif()

# ==============================================================================
# CPACK CONFIGURATION
# ==============================================================================
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "Hoka")
set(CPACK_PACKAGE_VENDOR "YourName")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hoka Key Analyzer - A tool to track and analyze keyboard usage")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Hoka")

set(CPACK_NSIS_DISPLAY_NAME "Hoka Key Analyzer")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

set(CPACK_NSIS_MENU_LINKS
    "bin/hoka.exe" "Hoka Key Analyzer"
    "share/hoka/README.md" "Readme"
    "share/hoka/LICENSE" "License"
)

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# ==============================================================================
# TESTING
# ==============================================================================
include(CTest)
enable_testing()

# ==============================================================================
# FINAL CPACK INCLUSION
# ==============================================================================
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)