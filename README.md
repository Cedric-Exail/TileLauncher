# TileLauncher

Lanceur d'applications en 8 tuiles configurable — Windows 11 x64 standalone.

## Téléchargement

👉 **[Dernière Release](../../releases/latest)** — téléchargez `TileLauncher_Win11_x64.zip`

## Build status

![Build](../../actions/workflows/build.yml/badge.svg)

## Fichiers du repo

```
├── tile_launcher.py          ← Code source
├── tiles.ini                 ← Configuration des tuiles
└── .github/workflows/
    └── build.yml             ← Pipeline de compilation automatique
```

## Configuration tiles.ini

```ini
[Tile1]
label   = Mon App
icon    = C:\chemin\icone.png
command = C:\Program Files\MonApp\app.exe
args    = --option
```
