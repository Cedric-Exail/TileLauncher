"""
TileLauncher - Lanceur d'applications en tuiles configurable
Dépendances : PyQt5
"""

import sys
import os
import subprocess
import configparser
import ctypes
import ctypes.wintypes
from PyQt5.QtWidgets import (
    QApplication, QWidget, QGridLayout, QPushButton,
    QLabel, QVBoxLayout, QHBoxLayout, QFrame, QSizePolicy
)
from PyQt5.QtGui import (
    QIcon, QPixmap, QFont, QColor, QPalette,
    QFontDatabase, QPainter, QLinearGradient, QBrush
)
from PyQt5.QtCore import Qt, QSize, QPoint, QRect, QTimer

# ── Chemin du fichier .ini ───────────────────────────────────────────────────
BASE_DIR = os.path.dirname(os.path.abspath(sys.executable if getattr(sys, 'frozen', False) else __file__))
INI_PATH = os.path.join(BASE_DIR, "tiles.ini")

# ── Constantes visuelles ─────────────────────────────────────────────────────
TILE_ROWS    = 2
TILE_COLS    = 4
NUM_TILES    = TILE_ROWS * TILE_COLS  # 8

BG_COLOR      = "#0f1117"
TITLEBAR_BG   = "#5f9ea0"          # Céladon
TITLEBAR_TEXT = "#ffffff"
TILE_BG       = "#1a1d27"
TILE_HOVER    = "#252840"
TILE_PRESS    = "#1e2235"
ACCENT        = "#5c6ef8"
ACCENT2       = "#8b5cf6"
TEXT_COLOR    = "#e8eaf6"
SUB_COLOR     = "#cfe8e8"
BORDER_COLOR  = "#2a2d3e"

LABEL_HEIGHT  = 22
TITLEBAR_H    = 36
MARGIN        = 6
SPACING       = 5


def _load_gilroy(db):
    """
    Charge Gilroy depuis fonts/ à côté de l'exe.
    Retourne le nom de famille ou 'Segoe UI'.
    """
    fonts_dir = os.path.join(BASE_DIR, "fonts")
    family = None
    if os.path.isdir(fonts_dir):
        for fname in sorted(os.listdir(fonts_dir)):
            if fname.lower().startswith("gilroy") and fname.lower().endswith(".ttf"):
                fid = db.addApplicationFont(os.path.join(fonts_dir, fname))
                if fid >= 0:
                    families = db.applicationFontFamilies(fid)
                    if families:
                        family = families[0]
    return family if family else "Segoe UI"


def load_config():
    """Retourne (app_config dict, liste des tuiles)."""
    config = configparser.ConfigParser()
    config.read(INI_PATH, encoding="utf-8")

    app_cfg = {
        "title": config.get("App", "title", fallback="TileLauncher"),
    }

    tiles = []
    for i in range(1, NUM_TILES + 1):
        section = f"Tile{i}"
        if config.has_section(section):
            tiles.append({
                "label":   config.get(section, "label",   fallback=f"Tuile {i}"),
                "icon":    config.get(section, "icon",    fallback=""),
                "command": config.get(section, "command", fallback=""),
                "args":    config.get(section, "args",    fallback=""),
            })
        else:
            tiles.append({"label": f"Tuile {i}", "icon": "", "command": "", "args": ""})
    return app_cfg, tiles


def launch(command, args):
    if not command:
        return
    try:
        full_cmd = f'"{command}"'
        if args:
            full_cmd += f" {args}"
        subprocess.Popen(full_cmd, shell=True)
    except Exception as e:
        print(f"Erreur lancement : {e}")


# ── Widget Tuile ─────────────────────────────────────────────────────────────
class TileButton(QFrame):
    def __init__(self, data, tile_w, tile_h, font_family, parent=None):
        super().__init__(parent)
        self.data       = data
        self._hovered   = False
        self._pressed   = False
        self._tile_w    = tile_w
        self._tile_h    = tile_h
        self._font_fam  = font_family

        self.setFixedSize(tile_w, tile_h)
        self.setCursor(Qt.PointingHandCursor)
        self.setStyleSheet(self._style())

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Icône : remplit tout l'espace au-dessus du label
        self.icon_label = QLabel()
        self.icon_label.setAlignment(Qt.AlignCenter)
        self.icon_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.icon_label.setScaledContents(False)
        self._set_icon(tile_w, tile_h - LABEL_HEIGHT)
        layout.addWidget(self.icon_label, stretch=1)

        # Label fixe en bas
        self.text_label = QLabel(data["label"])
        self.text_label.setFixedHeight(LABEL_HEIGHT)
        self.text_label.setAlignment(Qt.AlignCenter)
        self.text_label.setWordWrap(False)
        lbl_font = QFont(font_family, 9, QFont.DemiBold)
        self.text_label.setFont(lbl_font)
        self.text_label.setStyleSheet(f"""
            QLabel {{
                color: {TEXT_COLOR};
                background: rgba(10, 12, 20, 0.72);
                border-bottom-left-radius: 9px;
                border-bottom-right-radius: 9px;
                padding: 0 4px;
            }}
        """)
        layout.addWidget(self.text_label)

    def _set_icon(self, w, h):
        icon_path = self.data.get("icon", "")
        if icon_path and not os.path.isabs(icon_path):
            icon_path = os.path.join(BASE_DIR, icon_path)

        if icon_path and os.path.isfile(icon_path):
            pix = QPixmap(icon_path).scaled(
                w, h, Qt.KeepAspectRatioByExpanding, Qt.SmoothTransformation
            )
            # Recadrer si débordement
            if pix.width() > w or pix.height() > h:
                x = max(0, (pix.width()  - w) // 2)
                y = max(0, (pix.height() - h) // 2)
                pix = pix.copy(x, y, min(w, pix.width()), min(h, pix.height()))
            self.icon_label.setPixmap(pix)
        else:
            # Icône par défaut : dégradé + initiale Gilroy
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
            sz = max(18, min(w, h) // 3)
            p.setFont(QFont(self._font_fam, sz, QFont.Bold))
            initial = self.data["label"][0].upper() if self.data["label"] else "?"
            p.drawText(QRect(0, 0, w, h), Qt.AlignCenter, initial)
            p.end()
            self.icon_label.setPixmap(pix)

    def _style(self):
        bg     = TILE_PRESS if self._pressed else (TILE_HOVER if self._hovered else TILE_BG)
        border = ACCENT if self._hovered else BORDER_COLOR
        return f"""
            TileButton {{
                background: {bg};
                border: 1.5px solid {border};
                border-radius: 10px;
            }}
        """

    def enterEvent(self, e):
        self._hovered = True;  self.setStyleSheet(self._style())

    def leaveEvent(self, e):
        self._hovered = False; self._pressed = False; self.setStyleSheet(self._style())

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton:
            self._pressed = True;  self.setStyleSheet(self._style())

    def mouseReleaseEvent(self, e):
        if e.button() == Qt.LeftButton and self._pressed:
            self._pressed = False; self.setStyleSheet(self._style())
            launch(self.data["command"], self.data["args"])
            if self.window() and hasattr(self.window(), '_send_to_bottom'):
                self.window()._send_to_bottom()


# ── Fenêtre principale ────────────────────────────────────────────────────────
class TileLauncher(QWidget):
    def __init__(self):
        super().__init__()
        self._drag_pos = None
        self._font_fam = _load_gilroy(QFontDatabase())
        self._app_cfg, self._tiles_data = load_config()
        self._setup_window()
        self._build_ui()
        self._position_window()

    def _setup_window(self):
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.Tool)
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setWindowTitle(self._app_cfg["title"])
        screen = QApplication.primaryScreen().availableGeometry()
        self.setFixedSize(screen.width() // 2, screen.height() // 2)

    def _position_window(self):
        screen = QApplication.primaryScreen().availableGeometry()
        self.move(screen.right() - self.width(), screen.top())
        self._send_to_bottom()

    def _send_to_bottom(self):
        try:
            hwnd = int(self.winId())
            ctypes.windll.user32.SetWindowPos(
                hwnd, 1, 0, 0, 0, 0, 0x0002 | 0x0001 | 0x0010
            )
        except Exception as e:
            print(f"SetWindowPos : {e}")

    def _build_ui(self):
        container = QFrame(self)
        container.setObjectName("main")
        container.setGeometry(0, 0, self.width(), self.height())
        container.setStyleSheet("""
            QFrame#main {
                background: """ + BG_COLOR + """;
                border: 1.5px solid """ + BORDER_COLOR + """;
                border-radius: 14px;
            }
        """)

        root = QVBoxLayout(container)
        root.setContentsMargins(0, 0, 0, MARGIN)
        root.setSpacing(MARGIN)

        # ── Barre de titre céladon ────────────────────────────────────────────
        title_bar = QFrame()
        title_bar.setObjectName("titlebar")
        title_bar.setFixedHeight(TITLEBAR_H)
        title_bar.setStyleSheet("""
            QFrame#titlebar {
                background: """ + TITLEBAR_BG + """;
                border-top-left-radius: 13px;
                border-top-right-radius: 13px;
                border-bottom: 1px solid rgba(0,0,0,0.2);
            }
        """)

        tb = QHBoxLayout(title_bar)
        tb.setContentsMargins(10, 0, 6, 0)
        tb.setSpacing(4)

        self.title_lbl = QLabel(self._app_cfg["title"])
        self.title_lbl.setFont(QFont(self._font_fam, 11, QFont.Bold))
        self.title_lbl.setStyleSheet(f"color: {TITLEBAR_TEXT}; letter-spacing: 0.5px;")

        reload_btn = QPushButton("↺")
        reload_btn.setFixedSize(26, 26)
        reload_btn.setCursor(Qt.PointingHandCursor)
        reload_btn.setToolTip("Recharger la configuration")
        reload_btn.setStyleSheet(f"""
            QPushButton {{
                background: transparent; color: {TITLEBAR_TEXT};
                border: none; font-size: 16px; font-weight: bold;
            }}
            QPushButton:hover {{
                background: rgba(255,255,255,0.22); border-radius: 5px;
            }}
        """)
        reload_btn.clicked.connect(self._reload)

        close_btn = QPushButton("✕")
        close_btn.setFixedSize(26, 26)
        close_btn.setCursor(Qt.PointingHandCursor)
        close_btn.setStyleSheet(f"""
            QPushButton {{
                background: transparent; color: {TITLEBAR_TEXT};
                border: none; font-size: 11px; font-weight: bold;
            }}
            QPushButton:hover {{
                background: rgba(220,60,60,0.75); border-radius: 5px;
            }}
        """)
        close_btn.clicked.connect(self.close)

        tb.addWidget(self.title_lbl)
        tb.addStretch()
        tb.addWidget(reload_btn)
        tb.addWidget(close_btn)
        root.addWidget(title_bar)

        # ── Grille ────────────────────────────────────────────────────────────
        self.grid_widget = QWidget()
        self.grid_layout = QGridLayout(self.grid_widget)
        self.grid_layout.setSpacing(SPACING)
        self.grid_layout.setContentsMargins(MARGIN, 0, MARGIN, 0)
        self._populate_grid(self._tiles_data)
        root.addWidget(self.grid_widget, stretch=1)

    def _tile_size(self):
        usable_w = self.width()  - 2 * MARGIN - (TILE_COLS - 1) * SPACING
        usable_h = self.height() - TITLEBAR_H - MARGIN * 2 - (TILE_ROWS - 1) * SPACING
        return usable_w // TILE_COLS, usable_h // TILE_ROWS

    def _populate_grid(self, tiles_data):
        while self.grid_layout.count():
            item = self.grid_layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        tile_w, tile_h = self._tile_size()
        for idx, tdata in enumerate(tiles_data):
            tile = TileButton(tdata, tile_w, tile_h, self._font_fam)
            self.grid_layout.addWidget(tile, idx // TILE_COLS, idx % TILE_COLS)

    def _reload(self):
        self._app_cfg, tiles_data = load_config()
        self.title_lbl.setText(self._app_cfg["title"])
        self._populate_grid(tiles_data)

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
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    window = TileLauncher()
    window.show()
    sys.exit(app.exec_())
