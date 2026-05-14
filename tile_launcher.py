"""
TileLauncher - Lanceur d'applications en tuiles configurable
Optimisé pour un démarrage rapide.
Dépendances : PyQt5
"""

import sys
import os
import subprocess
import configparser
import ctypes
from PyQt5.QtWidgets import (
    QApplication, QWidget, QGridLayout, QPushButton,
    QLabel, QVBoxLayout, QHBoxLayout, QFrame, QSizePolicy
)
from PyQt5.QtGui import (
    QPixmap, QFont, QColor, QFontDatabase,
    QPainter, QLinearGradient, QBrush
)
from PyQt5.QtCore import Qt, QRect, QTimer

# ── Chemin de base ────────────────────────────────────────────────────────────
BASE_DIR = os.path.dirname(os.path.abspath(
    sys.executable if getattr(sys, 'frozen', False) else __file__
))
INI_PATH = os.path.join(BASE_DIR, "tiles.ini")

# ── Constantes layout ─────────────────────────────────────────────────────────
TILE_ROWS   = 2
TILE_COLS   = 4
NUM_TILES   = TILE_ROWS * TILE_COLS
LABEL_H     = 22
TITLEBAR_H  = 36
MARGIN      = 6
SPACING     = 5

# ── Palette ───────────────────────────────────────────────────────────────────
BG_COLOR      = "#0f1117"
TITLEBAR_BG   = "#5f9ea0"
TITLEBAR_TEXT = "#ffffff"
TILE_BG       = "#1a1d27"
TILE_HOVER    = "#252840"
TILE_PRESS    = "#1e2235"
ACCENT        = "#5c6ef8"
ACCENT2       = "#8b5cf6"
TEXT_COLOR    = "#e8eaf6"
BORDER_COLOR  = "#2a2d3e"

# ── Styles précalculés (une seule fois, pas à chaque widget) ──────────────────
_STYLE_NORMAL = (
    f"TileButton{{background:{TILE_BG};border:1.5px solid {BORDER_COLOR};border-radius:10px;}}"
)
_STYLE_HOVER  = (
    f"TileButton{{background:{TILE_HOVER};border:1.5px solid {ACCENT};border-radius:10px;}}"
)
_STYLE_PRESS  = (
    f"TileButton{{background:{TILE_PRESS};border:1.5px solid {ACCENT};border-radius:10px;}}"
)
_STYLE_LABEL  = (
    f"QLabel{{color:{TEXT_COLOR};background:rgba(10,12,20,0.72);"
    f"border-bottom-left-radius:9px;border-bottom-right-radius:9px;padding:0 4px;}}"
)

# ── Police : chargée une seule fois au niveau module ──────────────────────────
def _init_font() -> str:
    """Charge Gilroy si dispo, sinon Segoe UI. Rapide : arrête au 1er match."""
    fonts_dir = os.path.join(BASE_DIR, "fonts")
    if os.path.isdir(fonts_dir):
        for fname in os.listdir(fonts_dir):
            if fname.lower().startswith("gilroy") and fname.lower().endswith(".ttf"):
                fid = QFontDatabase.addApplicationFont(
                    os.path.join(fonts_dir, fname)
                )
                if fid >= 0:
                    fams = QFontDatabase.applicationFontFamilies(fid)
                    if fams:
                        # Charger les autres variantes en arrière-plan (pas bloquant)
                        return fams[0]
    return "Segoe UI"


# ── Config ────────────────────────────────────────────────────────────────────
def load_config():
    """Retourne (app_cfg, tiles_list). Lecture unique du .ini."""
    cfg = configparser.ConfigParser()
    cfg.read(INI_PATH, encoding="utf-8")
    app = {"title": cfg.get("App", "title", fallback="TileLauncher")}
    tiles = []
    for i in range(1, NUM_TILES + 1):
        s = f"Tile{i}"
        tiles.append({
            "label":   cfg.get(s, "label",   fallback=f"Tuile {i}"),
            "icon":    cfg.get(s, "icon",    fallback="") if cfg.has_section(s) else "",
            "command": cfg.get(s, "command", fallback="") if cfg.has_section(s) else "",
            "args":    cfg.get(s, "args",    fallback="") if cfg.has_section(s) else "",
        })
    return app, tiles


def launch(command, args):
    if not command:
        return
    try:
        cmd = f'"{command}"'
        if args:
            cmd += f" {args}"
        subprocess.Popen(cmd, shell=True)
    except Exception as e:
        print(f"Erreur lancement : {e}")


# ── Cache d'icônes : évite de recharger le même fichier plusieurs fois ────────
_PIX_CACHE: dict = {}

def _make_default_pixmap(w: int, h: int, label: str, font_fam: str) -> QPixmap:
    pix = QPixmap(w, h)
    pix.fill(Qt.transparent)
    p = QPainter(pix)
    p.setRenderHint(QPainter.Antialiasing)
    grad = QLinearGradient(0, 0, w, h)
    grad.setColorAt(0, QColor(ACCENT))
    grad.setColorAt(1, QColor(ACCENT2))
    p.setBrush(QBrush(grad))
    p.setPen(Qt.NoPen)
    p.drawRect(0, 0, w, h)
    p.setPen(QColor("white"))
    p.setFont(QFont(font_fam, max(18, min(w, h) // 3), QFont.Bold))
    p.drawText(QRect(0, 0, w, h), Qt.AlignCenter,
               label[0].upper() if label else "?")
    p.end()
    return pix

def _get_pixmap(icon_path: str, w: int, h: int,
                label: str, font_fam: str) -> QPixmap:
    key = (icon_path, w, h)
    if key in _PIX_CACHE:
        return _PIX_CACHE[key]

    if icon_path:
        if not os.path.isabs(icon_path):
            icon_path = os.path.join(BASE_DIR, icon_path)
        if os.path.isfile(icon_path):
            src = QPixmap(icon_path)
            if not src.isNull():
                pix = src.scaled(w, h, Qt.KeepAspectRatioByExpanding,
                                 Qt.SmoothTransformation)
                if pix.width() > w or pix.height() > h:
                    x = max(0, (pix.width()  - w) // 2)
                    y = max(0, (pix.height() - h) // 2)
                    pix = pix.copy(x, y, min(w, pix.width()),
                                         min(h, pix.height()))
                _PIX_CACHE[key] = pix
                return pix

    pix = _make_default_pixmap(w, h, label, font_fam)
    _PIX_CACHE[key] = pix
    return pix


# ── Widget Tuile ──────────────────────────────────────────────────────────────
class TileButton(QFrame):
    def __init__(self, data: dict, tile_w: int, tile_h: int,
                 font_fam: str, parent=None):
        super().__init__(parent)
        self.data      = data
        self._hovered  = False
        self._pressed  = False

        self.setFixedSize(tile_w, tile_h)
        self.setCursor(Qt.PointingHandCursor)
        self.setStyleSheet(_STYLE_NORMAL)      # style précalculé

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Icône
        icon_h = tile_h - LABEL_H
        self.icon_lbl = QLabel()
        self.icon_lbl.setAlignment(Qt.AlignCenter)
        self.icon_lbl.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.icon_lbl.setPixmap(
            _get_pixmap(data["icon"], tile_w, icon_h, data["label"], font_fam)
        )
        layout.addWidget(self.icon_lbl, stretch=1)

        # Label
        self.text_lbl = QLabel(data["label"])
        self.text_lbl.setFixedHeight(LABEL_H)
        self.text_lbl.setAlignment(Qt.AlignCenter)
        self.text_lbl.setFont(QFont(font_fam, 9, QFont.DemiBold))
        self.text_lbl.setStyleSheet(_STYLE_LABEL)   # style précalculé
        layout.addWidget(self.text_lbl)

    def enterEvent(self, e):
        self._hovered = True
        self.setStyleSheet(_STYLE_HOVER)

    def leaveEvent(self, e):
        self._hovered = False
        self._pressed = False
        self.setStyleSheet(_STYLE_NORMAL)

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton:
            self._pressed = True
            self.setStyleSheet(_STYLE_PRESS)

    def mouseReleaseEvent(self, e):
        if e.button() == Qt.LeftButton and self._pressed:
            self._pressed = False
            self.setStyleSheet(_STYLE_NORMAL)
            launch(self.data["command"], self.data["args"])
            win = self.window()
            if win and hasattr(win, '_send_to_bottom'):
                win._send_to_bottom()


# ── Fenêtre principale ────────────────────────────────────────────────────────
class TileLauncher(QWidget):
    def __init__(self):
        super().__init__()
        self._drag_pos = None

        # Chargements légers avant l'UI
        self._font_fam = _init_font()
        self._app_cfg, self._tiles = load_config()

        self._setup_window()
        self._build_ui()
        self._position_window()

    def _setup_window(self):
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.Tool)
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setWindowTitle(self._app_cfg["title"])
        scr = QApplication.primaryScreen().availableGeometry()
        self.setFixedSize(scr.width() // 2, scr.height() // 2)

    def _position_window(self):
        scr = QApplication.primaryScreen().availableGeometry()
        self.move(scr.right() - self.width(), scr.top())
        self._send_to_bottom()

    def _send_to_bottom(self):
        try:
            ctypes.windll.user32.SetWindowPos(
                int(self.winId()), 1, 0, 0, 0, 0, 0x0013
            )
        except Exception:
            pass

    def _build_ui(self):
        W, H = self.width(), self.height()

        container = QFrame(self)
        container.setObjectName("main")
        container.setGeometry(0, 0, W, H)
        container.setStyleSheet(
            f"QFrame#main{{background:{BG_COLOR};"
            f"border:1.5px solid {BORDER_COLOR};border-radius:14px;}}"
        )

        root = QVBoxLayout(container)
        root.setContentsMargins(0, 0, 0, MARGIN)
        root.setSpacing(MARGIN)

        # Barre de titre
        tbar = QFrame()
        tbar.setObjectName("tb")
        tbar.setFixedHeight(TITLEBAR_H)
        tbar.setStyleSheet(
            f"QFrame#tb{{background:{TITLEBAR_BG};"
            f"border-top-left-radius:13px;border-top-right-radius:13px;"
            f"border-bottom:1px solid rgba(0,0,0,0.2);}}"
        )
        tb = QHBoxLayout(tbar)
        tb.setContentsMargins(10, 0, 6, 0)
        tb.setSpacing(4)

        self.title_lbl = QLabel(self._app_cfg["title"])
        self.title_lbl.setFont(QFont(self._font_fam, 11, QFont.Bold))
        self.title_lbl.setStyleSheet(
            f"color:{TITLEBAR_TEXT};letter-spacing:0.5px;"
        )

        btn_style = (
            f"QPushButton{{background:transparent;color:{TITLEBAR_TEXT};"
            f"border:none;font-size:15px;font-weight:bold;}}"
            f"QPushButton:hover{{background:rgba(255,255,255,0.22);border-radius:5px;}}"
        )
        reload_btn = QPushButton("↺")
        reload_btn.setFixedSize(26, 26)
        reload_btn.setCursor(Qt.PointingHandCursor)
        reload_btn.setToolTip("Recharger la configuration")
        reload_btn.setStyleSheet(btn_style)
        reload_btn.clicked.connect(self._reload)

        close_btn = QPushButton("✕")
        close_btn.setFixedSize(26, 26)
        close_btn.setCursor(Qt.PointingHandCursor)
        close_btn.setStyleSheet(
            f"QPushButton{{background:transparent;color:{TITLEBAR_TEXT};"
            f"border:none;font-size:11px;font-weight:bold;}}"
            f"QPushButton:hover{{background:rgba(220,60,60,0.75);border-radius:5px;}}"
        )
        close_btn.clicked.connect(self.close)

        tb.addWidget(self.title_lbl)
        tb.addStretch()
        tb.addWidget(reload_btn)
        tb.addWidget(close_btn)
        root.addWidget(tbar)

        # Grille
        self.grid_widget = QWidget()
        self.grid_layout = QGridLayout(self.grid_widget)
        self.grid_layout.setSpacing(SPACING)
        self.grid_layout.setContentsMargins(MARGIN, 0, MARGIN, 0)
        self._populate_grid(self._tiles)
        root.addWidget(self.grid_widget, stretch=1)

    def _tile_size(self):
        uw = self.width()  - 2 * MARGIN - (TILE_COLS - 1) * SPACING
        uh = self.height() - TITLEBAR_H - MARGIN * 2 - (TILE_ROWS - 1) * SPACING
        return uw // TILE_COLS, uh // TILE_ROWS

    def _populate_grid(self, tiles):
        while self.grid_layout.count():
            item = self.grid_layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        tw, th = self._tile_size()
        ff = self._font_fam
        for idx, t in enumerate(tiles):
            self.grid_layout.addWidget(
                TileButton(t, tw, th, ff),
                idx // TILE_COLS, idx % TILE_COLS
            )

    def _reload(self):
        _PIX_CACHE.clear()          # vider le cache pour relire les icônes
        self._app_cfg, tiles = load_config()
        self.title_lbl.setText(self._app_cfg["title"])
        self._populate_grid(tiles)

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton and e.y() < TITLEBAR_H:
            self._drag_pos = e.globalPos() - self.frameGeometry().topLeft()

    def mouseMoveEvent(self, e):
        if self._drag_pos and e.buttons() == Qt.LeftButton:
            self.move(e.globalPos() - self._drag_pos)

    def mouseReleaseEvent(self, e):
        self._drag_pos = None


# ── Point d'entrée ────────────────────────────────────────────────────────────
if __name__ == "__main__":
    import time
    _t = {}

    _t["start"] = time.perf_counter()
    app = QApplication(sys.argv)
    _t["QApplication"] = time.perf_counter()

    app.setStyle("Fusion")
    _t["setStyle"] = time.perf_counter()

    # Profiling injecté dans __init__
    _orig_init = TileLauncher.__init__
    def _profiled_init(self):
        self._drag_pos = None
        t0 = time.perf_counter()
        self._font_fam = _init_font()
        _t["_init_font"] = time.perf_counter()
        self._app_cfg, self._tiles = load_config()
        _t["load_config"] = time.perf_counter()
        self._setup_window()
        _t["_setup_window"] = time.perf_counter()
        self._build_ui()
        _t["_build_ui"] = time.perf_counter()
        self._position_window()
        _t["_position_window"] = time.perf_counter()
    TileLauncher.__init__ = _profiled_init

    window = TileLauncher()
    _t["TileLauncher()"] = time.perf_counter()

    window.show()
    _t["show"] = time.perf_counter()

    # Afficher le rapport après 500ms (fenêtre visible)
    def _print_profile():
        keys = ["start","QApplication","setStyle","_init_font","load_config",
                "_setup_window","_build_ui","_position_window","TileLauncher()","show"]
        prev_key = "start"
        print("\n=== PROFILING DÉMARRAGE ===")
        total = 0
        for k in keys[1:]:
            if k in _t and prev_key in _t:
                ms = (_t[k] - _t[prev_key]) * 1000
                total += ms
                print(f"  {k:25s} : {ms:7.1f} ms")
            prev_key = k
        print(f"  {'TOTAL':25s} : {total:7.1f} ms")
        print("===========================\n")

    from PyQt5.QtCore import QTimer

    def _print_and_quit():
        _print_profile()
        # Quitter après le rapport si on est en mode profiling (QT_QPA_PLATFORM=offscreen)
        if os.environ.get("QT_QPA_PLATFORM") == "offscreen":
            app.quit()

    QTimer.singleShot(300, _print_and_quit)

    sys.exit(app.exec_())
