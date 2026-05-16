cmake_minimum_required(VERSION 3.22)

# ── vcpkg toolchain (doit être défini avant project()) ────────────────────────
# Passé via -DCMAKE_TOOLCHAIN_FILE en ligne de commande
# ou automatiquement si VCPKG_ROOT est défini

project(TileLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Liaison statique runtime MSVC (/MT au lieu de /MD) ────────────────────────
# Évite la dépendance à vcruntime140.dll et msvcp140.dll
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Qt6 statique via vcpkg
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

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

# Exécutable Windows sans console
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

# Plugins Qt statiques — requis pour que la fenêtre s'affiche
qt_import_plugins(TileLauncher
    INCLUDE Qt6::QWindowsIntegrationPlugin
            Qt6::QWindowsVistaStylePlugin
    EXCLUDE_BY_TYPE imageformats
    EXCLUDE_BY_TYPE sqldrivers
    EXCLUDE_BY_TYPE networkinformation
)

# ── Optimisation MSVC release ─────────────────────────────────────────────────
target_compile_options(TileLauncher PRIVATE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2 /GL /Gy>
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU>>:-O3>
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:Clang>>:-O3>
)
target_link_options(TileLauncher PRIVATE
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/LTCG /OPT:REF /OPT:ICF>
)
