# TileLauncher C++

> Lanceur d'applications en 8 tuiles configurable — Windows 11 x64  
> Application native **Qt6 / C++20**, démarrage instantané.

![Build](../../actions/workflows/build.yml/badge.svg)

---

## Téléchargement

👉 **[Dernière Release](../../releases/latest)** — téléchargez `TileLauncher_Cpp_Win11_x64.zip`

Extrayez le ZIP et double-cliquez sur `TileLauncher.exe`. Aucune installation requise.

---

## Fonctionnalités

- **8 tuiles** configurables en grille 4×2 sur un quart d'écran
- **Icône automatique** extraite de l'exe si aucune image n'est définie
- **Fenêtre en arrière-plan** (dernier plan, toujours visible mais non intrusive)
- **Barre de titre céladon** (`#00FFDA`) déplaçable par glisser
- **Police Gilroy** si présente dans `fonts/`, sinon Segoe UI
- **Rechargement à chaud** de la configuration (bouton ↺)
- **Journal horodaté** de toutes les actions (`TileLauncher.log`)
- **Compteur permanent** de temps d'utilisation (`TileLauncher.stats`)

---

## Performances vs version Python

| Critère           | Python / PyInstaller | C++ natif Qt6 |
|-------------------|:--------------------:|:-------------:|
| Taille exe        | ~40 Mo               | ~2 Mo         |
| Démarrage         | ~1.8 s               | **< 200 ms**  |
| RAM au démarrage  | ~80 Mo               | ~15 Mo        |
| Dépendances       | Aucune (onefile)     | DLL Qt (~30 Mo)|

---

## Structure du projet

```
TileLauncher/
├── src/
│   ├── main.cpp              Point d'entrée
│   ├── TileLauncher.h        Déclarations (fenêtre + tuiles)
│   ├── TileLauncher.cpp      Implémentation UI et logique
│   ├── Logger.h              Déclaration du logger singleton
│   └── Logger.cpp            Journal + compteur d'utilisation
├── CMakeLists.txt            Build Qt6 / CMake / MSVC ou MinGW
├── tiles.ini                 Configuration des 8 tuiles
├── build.yml                 Copie du workflow GitHub Actions
└── .github/workflows/
    └── build.yml             Pipeline CI/CD complet
```

---

## Configuration — `tiles.ini`

```ini
[App]
title = ⚡ Mon Lanceur          ; Titre affiché dans la barre céladon

[Tile1]
label   = Explorateur           ; Texte affiché en bas de la tuile
icon    =                       ; Chemin image (.png/.ico) — vide = icône de l'exe
command = C:\Windows\explorer.exe
args    =                       ; Arguments optionnels

[Tile2]
label   = VS Code
icon    = C:\icons\vscode.png
command = C:\Program Files\Microsoft VS Code\Code.exe
args    = C:\MonProjet

; ... jusqu'à [Tile8]
```

> Le bouton **↺** recharge `tiles.ini` sans redémarrer l'application.

---

## Fichiers de log

### `TileLauncher.log` — journal horodaté

```
╔══════════════════════════════════════════════════════╗
║  DÉMARRAGE  2025-05-14 10:23:45.123                 ║
║  Temps total d'utilisation cumulé : 2h 34m 12s      ║
╚══════════════════════════════════════════════════════╝
[2025-05-14 10:23:45.124] [INFO   ] Application démarrée
[2025-05-14 10:24:12.008] [ACTION ] Lancement tuile [Explorateur] → C:\Windows\explorer.exe
[2025-05-14 10:31:22.770] [INFO   ] Session terminée — durée : 0h 07m 37s
╔══════════════════════════════════════════════════════╗
║  FERMETURE  2025-05-14 10:31:22.771                 ║
║  Durée session          : 0h 07m 37s                ║
║  Temps total cumulé     : 2h 41m 49s                ║
╚══════════════════════════════════════════════════════╝
```

### `TileLauncher.stats` — compteur permanent

```ini
[usage]
total_seconds=9709
formatted=2h 41m 49s
last_updated=2025-05-14T10:31:22
```

---

## Police Gilroy

Placez les fichiers `Gilroy-*.ttf` dans un sous-dossier `fonts/` à côté de `TileLauncher.exe`.  
Si absente, la police système **Segoe UI** est utilisée automatiquement.

---

## Build via GitHub Actions

Le pipeline compile, teste et publie automatiquement à chaque push sur `main`.

```
build ──► test ──► release
```

| Job | Rôle |
|-----|------|
| `build` | Compile avec Qt 6.8.3 + MinGW sur `windows-2025-vs2026` |
| `test` | Vérifie l'exe, mesure le temps de lancement, capture un screenshot |
| `release` | Publie le ZIP dans les Releases GitHub |

---

## Build local

**Prérequis :** Qt 6.x, CMake 3.22+, Visual Studio 2022 ou MinGW

```cmd
cmake -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build --parallel
```

L'exe et les DLL Qt déployées se trouvent dans `build\`.
