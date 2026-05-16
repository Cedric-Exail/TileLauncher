cmake_minimum_required(VERSION 3.22)
project(TileLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Runtime MSVC statique (/MT) — évite vcruntime140.dll ─────────────────────
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ── Qt6 ───────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/TileLauncher.cpp
    src/Logger.cpp
)
set(HEADERS
    src/TileLauncher.h
    src/Logger.h
)

# ── Exécutable Windows sans console ──────────────────────────────────────────
if(WIN32)
    add_executable(TileLauncher WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(TileLauncher ${SOURCES} ${HEADERS})
endif()

# ── Liens ─────────────────────────────────────────────────────────────────────
target_link_libraries(TileLauncher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    # Windows API
    shell32 user32 gdi32 ole32 oleaut32 uuid
    winspool winmm imm32 comdlg32 advapi32
)

# ── Optimisation MSVC release ─────────────────────────────────────────────────
target_compile_options(TileLauncher PRIVATE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2 /GL /Gy>
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU>>:-O3>
)
target_link_options(TileLauncher PRIVATE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/LTCG /OPT:REF /OPT:ICF>
)

# ── Déploiement Qt (DLL dynamiques) ──────────────────────────────────────────
if(WIN32)
    add_custom_command(TARGET TileLauncher POST_BUILD
        COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
                --release
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                "$<TARGET_FILE:TileLauncher>"
        COMMENT "windeployqt..."
    )
endif()
