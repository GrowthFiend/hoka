cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Vcpkg target triplet")

project(hoka VERSION 0.1.0 LANGUAGES C CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ищем пакеты с правильными именами
find_package(FLTK CONFIG REQUIRED)  # Обратите внимание: FLTK, а не fltk
find_package(unofficial-sqlite3 CONFIG REQUIRED)  # Для SQLite3

# Windows resource file support
if(WIN32)
    enable_language(RC)
    set(RESOURCE_FILES 
        src/resource.rc
        src/hoka.manifest
    )
endif()

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/Database/Database.cpp
    src/KeyLogger/KeyLogger.cpp
    src/UI/MainWindow.cpp
    src/UI/SystemTray.cpp
)
# Заголовочные файлы (для IDE)
set(HEADERS
    src/Database/Database.h
    src/KeyLogger/KeyLogger.h
    src/UI/MainWindow.h
    src/UI/SystemTray.h
    src/Models/KeyStatistics.h
    src/Models/KeyPress.h
)

add_executable(hoka WIN32 
    ${SOURCES}
    ${HEADERS}
    ${RESOURCE_FILES}  # Добавляем ресурсы
)

target_compile_definitions(hoka PRIVATE
    UNICODE
    _UNICODE
    NTDDI_VERSION=0x06000000  # Ensures Vista+ features like NOTIFYICON_VERSION_4
)

target_include_directories(hoka PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Правильное указание целей (как указано в выводе vcpkg)
target_link_libraries(hoka PRIVATE 
    fltk  # Основная библиотека FLTK
    fltk_gl  # OpenGL поддержка
    fltk_forms  # Формы
    fltk_images  # Изображения
    unofficial::sqlite3::sqlite3  # SQLite3
)

# Дополнительные библиотеки для Windows (расширенный список)
if(WIN32)
    target_link_libraries(hoka PRIVATE 
        psapi      # Для GetModuleBaseName (определение имени процесса)
        user32     # Для Windows API (хуки клавиатуры, окна)
        kernel32   # Основные системные функции
        shell32    # Для системного трея (Shell_NotifyIcon)
        gdi32      # Для создания иконок и графики
        comctl32   # Для современных элементов управления
    )
    
    # Настройки для Windows-сборки
    set_target_properties(hoka PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # Манифест для требования прав администратора
    if(MSVC)
        set_target_properties(hoka PROPERTIES
            LINK_FLAGS "/MANIFEST:EMBED /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
        )
    else()
        # Для MinGW/GCC
        set_target_properties(hoka PROPERTIES
            LINK_FLAGS "-Wl,--subsystem,windows"
        )
    endif()
    
    # Для release-сборки убираем консоль полностью
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(MSVC)
            set_target_properties(hoka PROPERTIES
                LINK_FLAGS "${LINK_FLAGS} /SUBSYSTEM:WINDOWS"
            )
        endif()
    endif()
endif()

# Компилятор-специфичные опции
if(MSVC)
    target_compile_options(hoka PRIVATE /W4)
    # Отключаем предупреждения о "небезопасных" функциях MSVC
    target_compile_definitions(hoka PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
    )
else()
    target_compile_options(hoka PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug/Release конфигурации
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hoka PRIVATE DEBUG _DEBUG)
    if(WIN32 AND NOT MSVC)
        # Для debug-версии в MinGW показываем консоль
        set_target_properties(hoka PROPERTIES
            LINK_FLAGS "-Wl,--subsystem,console"
        )
    endif()
else()
    target_compile_definitions(hoka PRIVATE NDEBUG)
endif()

# Группировка файлов для IDE
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

# Установка файлов
if(WIN32)
    install(TARGETS hoka
        RUNTIME DESTINATION bin
    )
    
    # Копируем необходимые DLL (если используются динамические библиотеки)
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/hoka.exe\" \"\" \"${CMAKE_BINARY_DIR};${CMAKE_PREFIX_PATH}/bin\")
    ")
endif()

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)