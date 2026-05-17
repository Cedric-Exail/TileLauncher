# TileLauncher C++

> Launcher d’applications en 8 tuiles configurable — Windows 11 x64  
> Application native **Qt6 / C++20**, démarrage instantané.

![Build](../../actions/workflows/build.yml/badge.svg)

-----

## Téléchargement

👉 **[Dernière Release](../../releases/latest)** — téléchargez `TileLauncher_Cpp_Win11_x64.zip`

-----

## Installation

1. Extraire le ZIP dans le dossier de votre choix
1. Double-cliquer sur **`TileLauncher.bat`**

Aucune installation, aucun prérequis.

-----

## Structure du dossier

```
TileLauncher/
├── TileLauncher.bat          ← Lanceur (double-clic)
└── _internal/
    ├── TileLauncher.exe      ← Exécutable principal
    ├── tiles.ini             ← Configuration des tuiles
    ├── TileLauncher.dat      ← Données persistantes (position, stats)
    ├── TileLauncher.log      ← Journal (créé au premier lancement)
    ├── Qt6Core.dll
    ├── Qt6Gui.dll
    ├── Qt6Widgets.dll
    ├── vcruntime140.dll
    ├── msvcp140.dll
    ├── ...
    ├── platforms/
    ├── styles/
    └── imageformats/
```

-----

## Interface

|Élément           |Description                                      |
|------------------|-------------------------------------------------|
|**8 tuiles**      |Grille 4×2, redimensionnable à la souris         |
|**Barre de titre**|Couleur céladon `#00FFDA`, déplaçable par glisser|
|**Bouton ↺**      |Recharge `tiles.ini` sans redémarrer             |
|**Bouton ✕**      |Ferme l’application                              |
|**Bords et coins**|Redimensionnement par glisser (curseur adaptatif)|

La fenêtre se place en **arrière-plan** (dernier plan) et **restaure sa position et taille** au prochain lancement.

-----

## Configuration — `tiles.ini`

```ini
[App]
title = Mon Lanceur            ; Titre affiché dans la barre céladon

[Tile1]
label   = Explorateur          ; Texte affiché en bas de la tuile
icon    =                      ; Chemin image (.png/.ico) — optionnel
command = C:\Windows\explorer.exe
args    =                      ; Arguments optionnels

[Tile2]
label   = VS Code
icon    = C:\icons\vscode.png  ; Si défini, priorité sur l'icône de l'exe
command = C:\Program Files\Microsoft VS Code\Code.exe
args    = C:\MonProjet

; ... jusqu'à [Tile8]
```

### Priorité des icônes

1. Image définie dans `icon=` (si le fichier existe)
1. Icône native extraite de l’exécutable (`command=`)
1. Initiale du `label` sur fond noir

-----

## Fichiers persistants

### `TileLauncher.dat` — données binaires (80 octets)

Fichier binaire structuré avec magic number et CRC32. Contient :

|Champ            |Description                                  |
|-----------------|---------------------------------------------|
|Position X/Y     |Dernière position de la fenêtre sur le bureau|
|Largeur/Hauteur  |Dernière taille de la fenêtre                |
|Temps total      |Cumul du temps d’utilisation toutes sessions |
|Nb lancements    |Compteur permanent de lancements             |
|Dernier lancement|Timestamp Unix                               |

La position et la taille sont **restaurées au démarrage**, avec vérification que l’écran cible existe toujours (support multi-écrans).

### `TileLauncher.log` — journal texte

```
╔══════════════════════════════════════════════════════╗
║  STARTUP    2025-05-14 10:23:45.123                 ║
║  Launch #3         Total usage : 1h 12m 05s         ║
║  Last position     : X=1920  Y=0                    ║
╚══════════════════════════════════════════════════════╝
[2025-05-14 10:23:45.124] [INFO   ] Application started
[2025-05-14 10:24:12.008] [ACTION ] Tile launch [Explorer] -> C:\Windows\explorer.exe
[2025-05-14 10:31:22.770] [INFO   ] Session ended - duration: 0h 07m 37s
╔══════════════════════════════════════════════════════╗
║  SHUTDOWN   2025-05-14 10:31:22.771                 ║
║  Session duration   : 0h 07m 37s                    ║
║  Total usage        : 1h 19m 42s                    ║
║  Position saved     : X=1920  Y=0                   ║
║  Size saved         : W=960   H=540                 ║
╚══════════════════════════════════════════════════════╝
```

-----

## Performances

|Critère   |Python / PyInstaller|C++ natif Qt6|
|----------|:------------------:|:-----------:|
|Taille exe|~40 Mo              |~2 Mo        |
|Démarrage |~1.8 s              |**< 200 ms** |
|RAM       |~80 Mo              |~15 Mo       |

-----

## Pipeline CI/CD — GitHub Actions

```
build  ──►  test  ──►  release
```

|Job      |Détail                                                 |
|---------|-------------------------------------------------------|
|`build`  |Qt 6.8.3 + MSVC 2022 sur `windows-2025-vs2026`         |
|`test`   |Vérifie l’exe, mesure le temps de lancement, screenshot|
|`release`|Publie le ZIP dans les Releases GitHub                 |

Le build se déclenche automatiquement à chaque push sur `main`.  
Déclenchement manuel possible via **Actions → Run workflow**.

-----

## Build local

**Prérequis :** Qt 6.x, CMake 3.22+, Visual Studio 2022

```cmd
cmake -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER=cl ^
  -DCMAKE_CXX_COMPILER=cl ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build --parallel
```

-----

## Sources

```
src/
├── main.cpp          Point d'entrée, mode screenshot CI
├── TileLauncher.h    Déclarations (fenêtre, tuiles, redimensionnement)
├── TileLauncher.cpp  UI, logique, extraction icônes exe
├── Logger.h/.cpp     Journal texte (singleton)
└── AppData.h/.cpp    Persistance binaire CRC32 (position, stats)
```