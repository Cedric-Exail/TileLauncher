cmake_minimum_required(VERSION 3.22)
project(TileLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Sources
set(SOURCES
    src/main.cpp
    src/TileLauncher.cpp
    src/Logger.cpp
)
set(HEADERS
    src/TileLauncher.h
    src/Logger.h
)

# Exécutable Windows (sans console)
if(WIN32)
    add_executable(TileLauncher WIN32 ${SOURCES} ${HEADERS})
    target_link_libraries(TileLauncher PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets
        shell32 user32 gdi32    # Windows API pour ExtractIconEx, SetWindowPos
    )
else()
    add_executable(TileLauncher ${SOURCES} ${HEADERS})
    target_link_libraries(TileLauncher PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
endif()

# Optimisation release
target_compile_options(TileLauncher PRIVATE
    $<$<CONFIG:Release>:/O2 /GL>          # MSVC
    $<$<CXX_COMPILER_ID:GNU>:-O3>         # MinGW
)
target_link_options(TileLauncher PRIVATE
    $<$<CONFIG:Release>:/LTCG>            # Link-time code generation MSVC
)

# Déploiement Qt (windeployqt)
if(WIN32)
    add_custom_command(TARGET TileLauncher POST_BUILD
        COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
                --release
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                "$<TARGET_FILE:TileLauncher>"
        COMMENT "Deploiement Qt..."
    )
endif()
