cmake_minimum_required(VERSION 3.22)
project(TileLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Qt6 ───────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/TileLauncher.cpp
    src/Logger.cpp
    src/AppData.cpp
)
set(HEADERS
    src/TileLauncher.h
    src/Logger.h
    src/AppData.h
)
set(RESOURCES resources/app.qrc)

# Icone Windows pour l exe (via .rc)
if(WIN32)
    list(APPEND SOURCES resources/app.rc)
endif()

# ── Exécutable Windows sans console ──────────────────────────────────────────
if(WIN32)
    add_executable(TileLauncher WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    add_executable(TileLauncher ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

# Icone de l exe via CMake
if(WIN32)
    set_target_properties(TileLauncher PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ── Liens ─────────────────────────────────────────────────────────────────────
target_link_libraries(TileLauncher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
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
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO>
)

# Manifest : demander l elevation normale (pas admin) avec priorité normale
# Le process sera eleve via SetPriorityClass dans main()
if(WIN32)
    set_target_properties(TileLauncher PROPERTIES
        VS_DPI_AWARE "PerMonitor"
    )
endif()

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
