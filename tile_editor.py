# identical editor minimal
import sys, configparser
from PyQt5.QtWidgets import *
INI="tiles.ini"
class E(QWidget):
 def __init__(self):
  super().__init__()
  l=QVBoxLayout(self)
  self.cfg=configparser.ConfigParser(); self.cfg.read(INI)
  self.f=[]
  for i in range(1,9):
   g=QGroupBox(f"Tile {i}"); fml=QFormLayout()
   a=QLineEdit(self.cfg.get(f"Tile{i}","label",fallback=""))
   b=QLineEdit(self.cfg.get(f"Tile{i}","command",fallback=""))
   c=QLineEdit(self.cfg.get(f"Tile{i}","args",fallback=""))
   fml.addRow("Label",a);fml.addRow("Cmd",b);fml.addRow("Args",c)
   g.setLayout(fml); l.addWidget(g)
   self.f.append((a,b,c))
  btn=QPushButton("Save");btn.clicked.connect(self.s);l.addWidget(btn)
 def s(self):
  for i,(a,b,c) in enumerate(self.f,1):
   sec=f"Tile{i}";self.cfg[sec]={"label":a.text(),"command":b.text(),"args":c.text()}
  open(INI,'w').write('
'.join(['[%s]
'+"
".join(f"{k} = {v}" for k,v in self.cfg[s].items()) for s in self.cfg]))
if __name__=='__main__':
 app=QApplication(sys.argv);w=E();w.show();sys.exit(app.exec_())
