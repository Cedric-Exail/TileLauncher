# TileLauncher C++

Application native Qt6/C++ — lanceur d'applications en 8 tuiles configurable.

## Avantages vs version Python

| | Python/PyInstaller | C++ natif |
|--|--|--|
| Taille exe | ~40 Mo | ~2-5 Mo |
| Démarrage | ~1.8s | < 200ms |
| RAM | ~80 Mo | ~15 Mo |
| Dépendances | Aucune (--onefile) | DLL Qt (~30 Mo) |

## Fichiers

```
TileLauncher/
├── src/
│   ├── main.cpp
│   ├── TileLauncher.h
│   └── TileLauncher.cpp
├── CMakeLists.txt
├── tiles.ini
└── .github/workflows/build.yml
```

## Build via GitHub Actions

Pushez le repo — le workflow compile, teste et publie automatiquement.

## Build local (Windows)

Prérequis : Qt6, CMake, Visual Studio 2022

```cmd
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build --parallel
```

## Configuration tiles.ini

```ini
[App]
title = Mon Lanceur

[Tile1]
label   = Explorateur
icon    =                          ; vide = icône extraite de l'exe
command = C:\Windows\explorer.exe
args    =
```

## Police Gilroy

Placer les fichiers `Gilroy-*.ttf` dans `fonts/` à côté de `TileLauncher.exe`.
