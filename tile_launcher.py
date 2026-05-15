import sys, os, subprocess, configparser, ctypes
from PyQt5.QtWidgets import QApplication, QWidget, QGridLayout, QLabel, QVBoxLayout, QFrame, QMessageBox, QLineEdit
from PyQt5.QtGui import QPixmap, QFont, QPainter, QColor, QLinearGradient
from PyQt5.QtCore import Qt, QRect, QPropertyAnimation

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
INI_PATH = os.path.join(BASE_DIR, "tiles.ini")
ROWS, COLS = 2, 4

# ─── Windows 11 Blur Effect ──────────────────────────────
def enable_blur(hwnd):
    class ACCENT(ctypes.Structure):
        _fields_ = [('state', ctypes.c_int), ('flags', ctypes.c_int), ('color', ctypes.c_int), ('animation', ctypes.c_int)]

    class DATA(ctypes.Structure):
        _fields_ = [('attr', ctypes.c_int), ('pAccent', ctypes.POINTER(ACCENT)), ('size', ctypes.c_int)]

    accent = ACCENT(3, 0, 0x990000, 0)
    data = DATA(19, ctypes.pointer(accent), ctypes.sizeof(accent))

    ctypes.windll.user32.SetWindowCompositionAttribute(hwnd, ctypes.byref(data))

# ───────────────────────────────────────────────────────-

def load_config():
    cfg = configparser.ConfigParser()
    cfg.read(INI_PATH, encoding="utf-8")
    tiles = []
    for i in range(1,9):
        sec=f"Tile{i}"
        tiles.append({
            "label": cfg.get(sec,"label",fallback=f"Tile {i}"),
            "command": cfg.get(sec,"command",fallback=""),
            "args": cfg.get(sec,"args",fallback="")
        })
    return tiles

# ───────────────────────────────────────────────────────-

def launch(command,args):
    if not command: return
    try:
        cmd=[command]
        if args: cmd+=args.split()
        subprocess.Popen(cmd,shell=False)
    except Exception as e:
        QMessageBox.critical(None,"Erreur",str(e))

# ───────────────────────────────────────────────────────-

def gen_pix(w,h,label):
    pix=QPixmap(w,h)
    pix.fill(Qt.transparent)
    p=QPainter(pix)
    grad=QLinearGradient(0,0,w,h)
    grad.setColorAt(0,QColor("#00FFDA"))
    grad.setColorAt(1,QColor("#007d6b"))
    p.setBrush(grad)
    p.setPen(Qt.NoPen)
    p.drawRect(0,0,w,h)
    p.setPen(Qt.white)
    p.setFont(QFont("Segoe UI",26,QFont.Bold))
    p.drawText(QRect(0,0,w,h),Qt.AlignCenter,label[0].upper())
    p.end()
    return pix

# ───────────────────────────────────────────────────────-

class Tile(QFrame):
    def __init__(self,data):
        super().__init__()
        self.data=data
        self.setMinimumSize(160,120)
        self.setStyleSheet("border-radius:12px;background:#1a1d27;")
        l=QVBoxLayout(self)
        icon=QLabel()
        icon.setPixmap(gen_pix(160,120,data["label"]))
        txt=QLabel(data["label"])
        txt.setAlignment(Qt.AlignCenter)
        l.addWidget(icon)
        l.addWidget(txt)
        self.anim=QPropertyAnimation(self,b"geometry")
        self.anim.setDuration(120)

    def mousePressEvent(self,e): launch(self.data["command"],self.data["args"])

    def enterEvent(self,e):
        r=self.geometry()
        self.anim.stop()
        self.anim.setStartValue(r)
        self.anim.setEndValue(r.adjusted(-2,-2,2,2))
        self.anim.start()

# ───────────────────────────────────────────────────────-

class App(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("TileLauncher")
        self.resize(720,400)
        self.setAttribute(Qt.WA_TranslucentBackground)

        layout=QVBoxLayout(self)
        self.search=QLineEdit()
        self.search.setPlaceholderText("Rechercher...")
        layout.addWidget(self.search)

        self.grid=QGridLayout()
        layout.addLayout(self.grid)

        self.tiles=load_config()
        self.draw()

        self.search.textChanged.connect(self.filter)

        # Blur activation
        hwnd=int(self.winId())
        enable_blur(hwnd)

    def draw(self):
        for i in reversed(range(self.grid.count())):
            self.grid.itemAt(i).widget().deleteLater()
        for i,t in enumerate(self.tiles):
            self.grid.addWidget(Tile(t),i//4,i%4)

    def filter(self,t):
        t=t.lower()
        self.tiles=[x for x in load_config() if t in x["label"].lower()]
        self.draw()

if __name__=="__main__":
    app=QApplication(sys.argv)
    w=App()
    w.show()
    sys.exit(app.exec_())
