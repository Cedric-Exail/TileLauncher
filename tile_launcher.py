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

# ── Chemin du fichier .ini (même dossier que l'exécutable) ──────────────────
BASE_DIR = os.path.dirname(os.path.abspath(sys.executable if getattr(sys, 'frozen', False) else __file__))
INI_PATH = os.path.join(BASE_DIR, "tiles.ini")

# ── Constantes visuelles ────────────────────────────────────────────────────
TILE_ROWS    = 2
TILE_COLS    = 4
NUM_TILES    = TILE_ROWS * TILE_COLS   # 8

BG_COLOR     = "#0f1117"
TILE_BG      = "#1a1d27"
TILE_HOVER   = "#252840"
TILE_PRESS   = "#1e2235"
ACCENT       = "#5c6ef8"
ACCENT2      = "#8b5cf6"
TEXT_COLOR   = "#e8eaf6"
SUB_COLOR    = "#6b7280"
BORDER_COLOR = "#2a2d3e"


def load_config():
    """Charge le fichier .ini et retourne la liste des tuiles."""
    config = configparser.ConfigParser()
    config.read(INI_PATH, encoding="utf-8")

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
            tiles.append({
                "label":   f"Tuile {i}",
                "icon":    "",
                "command": "",
                "args":    "",
            })
    return tiles


def launch(command, args):
    """Lance la commande configurée."""
    if not command:
        return
    try:
        full_cmd = f'"{command}"'
        if args:
            full_cmd += f" {args}"
        subprocess.Popen(full_cmd, shell=True)
    except Exception as e:
        print(f"Erreur lancement : {e}")


# ── Widget Tuile ────────────────────────────────────────────────────────────
class TileButton(QFrame):
    def __init__(self, data: dict, parent=None):
        super().__init__(parent)
        self.data = data
        self._hovered  = False
        self._pressed  = False

        self.setFixedSize(130, 110)
        self.setCursor(Qt.PointingHandCursor)
        self.setStyleSheet(self._style())

        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        layout.setSpacing(8)
        layout.setContentsMargins(8, 12, 8, 12)

        # Icône
        self.icon_label = QLabel()
        self.icon_label.setAlignment(Qt.AlignCenter)
        self.icon_label.setFixedSize(40, 40)
        self._set_icon()
        layout.addWidget(self.icon_label)

        # Texte
        self.text_label = QLabel(data["label"])
        self.text_label.setAlignment(Qt.AlignCenter)
        self.text_label.setWordWrap(True)
        self.text_label.setStyleSheet(f"""
            color: {TEXT_COLOR};
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 0.3px;
        """)
        layout.addWidget(self.text_label)

    def _set_icon(self):
        icon_path = self.data.get("icon", "")
        if icon_path and os.path.isfile(icon_path):
            pix = QPixmap(icon_path).scaled(
                36, 36, Qt.KeepAspectRatio, Qt.SmoothTransformation
            )
            self.icon_label.setPixmap(pix)
        else:
            # Icône par défaut : carré coloré avec initiale
            pix = QPixmap(36, 36)
            pix.fill(Qt.transparent)
            p = QPainter(pix)
            p.setRenderHint(QPainter.Antialiasing)
            grad = QLinearGradient(0, 0, 36, 36)
            grad.setColorAt(0, QColor(ACCENT))
            grad.setColorAt(1, QColor(ACCENT2))
            p.setBrush(QBrush(grad))
            p.setPen(Qt.NoPen)
            p.drawRoundedRect(0, 0, 36, 36, 8, 8)
            p.setPen(QColor("white"))
            font = QFont()
            font.setPointSize(14)
            font.setBold(True)
            p.setFont(font)
            initial = self.data["label"][0].upper() if self.data["label"] else "?"
            p.drawText(QRect(0, 0, 36, 36), Qt.AlignCenter, initial)
            p.end()
            self.icon_label.setPixmap(pix)

    def _style(self):
        if self._pressed:
            bg = TILE_PRESS
        elif self._hovered:
            bg = TILE_HOVER
        else:
            bg = TILE_BG

        border = ACCENT if self._hovered else BORDER_COLOR
        shadow = f"0 0 12px {ACCENT}55" if self._hovered else "none"
        return f"""
            TileButton {{
                background: {bg};
                border: 1.5px solid {border};
                border-radius: 12px;
            }}
        """

    def enterEvent(self, e):
        self._hovered = True
        self.setStyleSheet(self._style())

    def leaveEvent(self, e):
        self._hovered = False
        self._pressed = False
        self.setStyleSheet(self._style())

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton:
            self._pressed = True
            self.setStyleSheet(self._style())

    def mouseReleaseEvent(self, e):
        if e.button() == Qt.LeftButton and self._pressed:
            self._pressed = False
            self.setStyleSheet(self._style())
            launch(self.data["command"], self.data["args"])
            # Repasser en arrière-plan après lancement
            if self.window() and hasattr(self.window(), '_send_to_bottom'):
                self.window()._send_to_bottom()


# ── Fenêtre principale ──────────────────────────────────────────────────────
class TileLauncher(QWidget):
    def __init__(self):
        super().__init__()
        self._drag_pos = None
        self._setup_window()
        self._build_ui()
        self._position_window()

    def _setup_window(self):
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.Tool)
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setWindowTitle("TileLauncher")

        # Taille = 1/4 d'écran environ
        screen = QApplication.primaryScreen().availableGeometry()
        w = screen.width() // 2
        h = screen.height() // 2
        self.setFixedSize(w, h)

    def _position_window(self):
        screen = QApplication.primaryScreen().availableGeometry()
        # Coin supérieur droit par défaut
        x = screen.right() - self.width()
        y = screen.top()
        self.move(x, y)
        # Placer en arrière-plan via l'API Windows
        self._send_to_bottom()

    def _send_to_bottom(self):
        """Utilise SetWindowPos (WinAPI) pour placer la fenêtre en dernier plan."""
        try:
            HWND_BOTTOM = 1          # Derrière toutes les fenêtres
            SWP_NOMOVE      = 0x0002
            SWP_NOSIZE      = 0x0001
            SWP_NOACTIVATE  = 0x0010
            hwnd = int(self.winId())
            ctypes.windll.user32.SetWindowPos(
                hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
            )
        except Exception as e:
            print(f"SetWindowPos non disponible : {e}")

    def _build_ui(self):
        tiles_data = load_config()

        # Conteneur principal avec fond arrondi
        container = QFrame(self)
        container.setGeometry(0, 0, self.width(), self.height())
        container.setStyleSheet(f"""
            QFrame {{
                background: {BG_COLOR};
                border: 1.5px solid {BORDER_COLOR};
                border-radius: 16px;
            }}
        """)

        root = QVBoxLayout(container)
        root.setContentsMargins(16, 12, 16, 16)
        root.setSpacing(10)

        # ── Barre de titre ──────────────────────────────────────────────────
        title_bar = QHBoxLayout()
        title_bar.setContentsMargins(0, 0, 0, 0)

        dot_row = QHBoxLayout()
        dot_row.setSpacing(6)
        for color in ["#ff5f57", "#febc2e", "#28c840"]:
            d = QLabel()
            d.setFixedSize(12, 12)
            d.setStyleSheet(f"background:{color}; border-radius:6px;")
            dot_row.addWidget(d)
        dot_row.addStretch()

        title_lbl = QLabel("⚡ TileLauncher")
        title_lbl.setStyleSheet(f"""
            color: {SUB_COLOR};
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 1px;
        """)

        reload_btn = QPushButton("↺")
        reload_btn.setToolTip("Recharger la configuration")
        reload_btn.setFixedSize(24, 24)
        reload_btn.setCursor(Qt.PointingHandCursor)
        reload_btn.setStyleSheet(f"""
            QPushButton {{
                background: transparent;
                color: {SUB_COLOR};
                border: none;
                font-size: 14px;
                font-weight: bold;
            }}
            QPushButton:hover {{ color: {ACCENT}; }}
        """)
        reload_btn.clicked.connect(self._reload)

        close_btn = QPushButton("✕")
        close_btn.setFixedSize(24, 24)
        close_btn.setCursor(Qt.PointingHandCursor)
        close_btn.setStyleSheet(f"""
            QPushButton {{
                background: transparent;
                color: {SUB_COLOR};
                border: none;
                font-size: 12px;
                font-weight: bold;
            }}
            QPushButton:hover {{ color: #ff5f57; }}
        """)
        close_btn.clicked.connect(self.close)

        title_bar.addLayout(dot_row)
        title_bar.addStretch()
        title_bar.addWidget(title_lbl)
        title_bar.addStretch()
        title_bar.addWidget(reload_btn)
        title_bar.addWidget(close_btn)
        root.addLayout(title_bar)

        # Séparateur
        sep = QFrame()
        sep.setFrameShape(QFrame.HLine)
        sep.setStyleSheet(f"color: {BORDER_COLOR};")
        root.addWidget(sep)

        # ── Grille de tuiles ────────────────────────────────────────────────
        self.grid_widget = QWidget()
        self.grid_layout = QGridLayout(self.grid_widget)
        self.grid_layout.setSpacing(12)
        self.grid_layout.setContentsMargins(4, 4, 4, 4)

        self._populate_grid(tiles_data)
        root.addWidget(self.grid_widget, stretch=1)

    def _populate_grid(self, tiles_data):
        # Vider la grille existante
        while self.grid_layout.count():
            item = self.grid_layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        for idx, tdata in enumerate(tiles_data):
            row = idx // TILE_COLS
            col = idx % TILE_COLS
            tile = TileButton(tdata)
            # Centrer les tuiles
            tile.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
            tile.setFixedSize(
                max(110, (self.width() - 80) // TILE_COLS),
                max(90,  (self.height() - 110) // TILE_ROWS)
            )
            self.grid_layout.addWidget(tile, row, col, Qt.AlignCenter)

    def _reload(self):
        tiles_data = load_config()
        self._populate_grid(tiles_data)

    # ── Déplacement de la fenêtre ───────────────────────────────────────────
    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton:
            self._drag_pos = e.globalPos() - self.frameGeometry().topLeft()

    def mouseMoveEvent(self, e):
        if self._drag_pos and e.buttons() == Qt.LeftButton:
            self.move(e.globalPos() - self._drag_pos)

    def mouseReleaseEvent(self, e):
        self._drag_pos = None


# ── Point d'entrée ──────────────────────────────────────────────────────────
if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    window = TileLauncher()
    window.show()
    sys.exit(app.exec_())
