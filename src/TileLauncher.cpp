#include "TileLauncher.h"
#include "Logger.h"
#include "AppData.h"

#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEnterEvent>
#include <QCloseEvent>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QImage>
#include <QCursor>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

// ── Styles statiques ──────────────────────────────────────────────────────────
const QString TileButton::STYLE_NORMAL = QString(
    "TileButton{background:%1;border:1.5px solid %2;border-radius:10px;}"
).arg(TILE_BG, BORDER_COLOR);
const QString TileButton::STYLE_HOVER = QString(
    "TileButton{background:%1;border:1.5px solid %2;border-radius:10px;}"
).arg(TILE_HOVER, ACCENT);
const QString TileButton::STYLE_PRESS = QString(
    "TileButton{background:%1;border:1.5px solid %2;border-radius:10px;}"
).arg(TILE_PRESS, ACCENT);
const QString TileButton::STYLE_LABEL = QString(
    "QLabel{color:%1;background:rgba(10,12,20,0.72);"
    "border-bottom-left-radius:9px;border-bottom-right-radius:9px;padding:0 4px;}"
).arg(TEXT_COLOR);

// ════════════════════════════════════════════════════════════════════════════
// TileButton
// ════════════════════════════════════════════════════════════════════════════

TileButton::TileButton(const TileData& data, int w, int h,
                       const QString& fontFamily, QWidget* parent)
    : QFrame(parent), m_data(data), m_fontFamily(fontFamily)
{
    setFixedSize(w, h);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(STYLE_NORMAL);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    updateIcon(w, h - LABEL_H);
    layout->addWidget(m_iconLabel, 1);

    m_textLabel = new QLabel(data.label, this);
    m_textLabel->setFixedHeight(LABEL_H);
    m_textLabel->setAlignment(Qt::AlignCenter);
    m_textLabel->setFont(QFont(fontFamily, 9, QFont::DemiBold));
    m_textLabel->setStyleSheet(STYLE_LABEL);
    layout->addWidget(m_textLabel);
}

void TileButton::resize(int w, int h)
{
    setFixedSize(w, h);
    updateIcon(w, h - LABEL_H);
}

void TileButton::updateIcon(int w, int h)
{
    if (w > 0 && h > 0)
        m_iconLabel->setPixmap(buildPixmap(w, h));
}

void TileButton::updateStyle()
{
    if (m_pressed)       setStyleSheet(STYLE_PRESS);
    else if (m_hovered)  setStyleSheet(STYLE_HOVER);
    else                 setStyleSheet(STYLE_NORMAL);
}

QPixmap TileButton::scaleAndCrop(const QPixmap& src, int w, int h)
{
    // Conserver le ratio initial : KeepAspectRatio (pas expanding)
    QPixmap pix = src.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return pix;
}

QPixmap TileButton::extractExeIcon(const QString& exePath, int w, int h)
{
#ifdef Q_OS_WIN
    if (exePath.isEmpty() || !QFileInfo::exists(exePath)) return {};
    const int iconSize = qMax(w, h) >= 48 ? 48 : 32;
    HICON hLarge[1]={}, hSmall[1]={};
    UINT n = ExtractIconExW(
        reinterpret_cast<const wchar_t*>(exePath.utf16()),
        0, hLarge, hSmall, 1);
    if (n == 0) return {};
    HICON hIcon = hLarge[0] ? hLarge[0] : hSmall[0];
    if (!hIcon) return {};
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem    = CreateCompatibleDC(hdcScreen);
    BITMAPINFOHEADER bmi{};
    bmi.biSize=sizeof(bmi); bmi.biWidth=iconSize;
    bmi.biHeight=-iconSize; bmi.biPlanes=1; bmi.biBitCount=32;
    void* pvBits=nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem,
        reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS, &pvBits, nullptr, 0);
    SelectObject(hdcMem, hBmp);
    DrawIconEx(hdcMem,0,0,hIcon,iconSize,iconSize,0,nullptr,DI_NORMAL);
    const int bufSize = iconSize*iconSize*4;
    QByteArray buf(bufSize, Qt::Uninitialized);
    GetBitmapBits(hBmp, bufSize, buf.data());
    DeleteObject(hBmp); DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    if (hLarge[0]) DestroyIcon(hLarge[0]);
    if (hSmall[0]) DestroyIcon(hSmall[0]);
    QByteArray rgba(bufSize, Qt::Uninitialized);
    for (int i=0;i<bufSize;i+=4){
        rgba[i]=buf[i+2]; rgba[i+1]=buf[i+1]; rgba[i+2]=buf[i]; rgba[i+3]=buf[i+3];
    }
    QImage img(reinterpret_cast<const uchar*>(rgba.constData()),
               iconSize,iconSize,iconSize*4,QImage::Format_RGBA8888);
    QPixmap pix = QPixmap::fromImage(img.copy());
    return pix.isNull() ? QPixmap{} : scaleAndCrop(pix, w, h);
#else
    Q_UNUSED(exePath); Q_UNUSED(w); Q_UNUSED(h); return {};
#endif
}

QPixmap TileButton::buildPixmap(int w, int h)
{
    // 1. Image définie dans le .ini
    if (!m_data.icon.isEmpty()) {
        QString path = m_data.icon;
        if (!QFileInfo(path).isAbsolute())
            path = QDir(TileLauncher::iniPath()).absoluteFilePath(path);
        if (QFileInfo::exists(path)) {
            QPixmap src(path);
            if (!src.isNull()) return scaleAndCrop(src, w, h);
        }
    }
    // 2. Icône native de l'exe
    if (!m_data.command.isEmpty()) {
        QString exe = m_data.command.trimmed().remove('"');
        QPixmap pix = extractExeIcon(exe, w, h);
        if (!pix.isNull()) return pix;
    }
    // 3. Initiale sur fond noir
    QPixmap pix(w, h);
    pix.fill(Qt::black);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(ACCENT));
    p.setFont(QFont(m_fontFamily, qMax(18, qMin(w,h)/3), QFont::Bold));
    QString initial = m_data.label.isEmpty()
        ? QStringLiteral("?")
        : QString(m_data.label.at(0).toUpper());
    p.drawText(QRect(0,0,w,h), Qt::AlignCenter, initial);
    return pix;
}

void TileButton::enterEvent(QEnterEvent* e)
{ Q_UNUSED(e); m_hovered=true;  updateStyle(); }
void TileButton::leaveEvent(QEvent* e)
{ Q_UNUSED(e); m_hovered=false; m_pressed=false; updateStyle(); }
void TileButton::mousePressEvent(QMouseEvent* e)
{ if(e->button()==Qt::LeftButton){ m_pressed=true; updateStyle(); } }
void TileButton::mouseReleaseEvent(QMouseEvent* e)
{
    if(e->button()==Qt::LeftButton && m_pressed){
        m_pressed=false; updateStyle();
        if(!m_data.command.isEmpty()){
            Logger::instance().logTileAction(m_data.label, m_data.command);
            QStringList args;
            if(!m_data.args.isEmpty()) args=QProcess::splitCommand(m_data.args);
            QProcess::startDetached(m_data.command, args);
        }
        if(auto* win=qobject_cast<TileLauncher*>(window()))
            QMetaObject::invokeMethod(win,"sendToBottom",Qt::QueuedConnection);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// TileLauncher — statics
// ════════════════════════════════════════════════════════════════════════════

QString TileLauncher::iniPath() {
    // tiles.ini dans _internal/ (a cote de l exe)
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("tiles.ini");
}
QString TileLauncher::logPath() {
    // TileLauncher.log dans _internal/ (a cote de l exe)
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("TileLauncher.log");
}
QString TileLauncher::datPath() {
    // TileLauncher.dat : dans _internal/ (a cote de l exe)
    QString appDir   = QCoreApplication::applicationDirPath();
    QString internal = QDir(appDir).absoluteFilePath("_internal");
    // Creer _internal/ si absent (premier lancement depuis un autre endroit)
    QDir().mkpath(internal);
    return QDir(internal).absoluteFilePath("TileLauncher.dat");
}

TileLauncher::AppConfig TileLauncher::loadAppConfig(const QString& path)
{
    QSettings s(path, QSettings::IniFormat);
    AppConfig cfg;
    cfg.title = s.value("App/title","TileLauncher").toString();
    return cfg;
}

QList<TileData> TileLauncher::loadTiles(const QString& path)
{
    QSettings s(path, QSettings::IniFormat);
    QList<TileData> tiles;
    for(int i=1;i<=NUM_TILES;++i){
        s.beginGroup(QString("Tile%1").arg(i));
        TileData t;
        t.label   = s.value("label",   QString("Tile %1").arg(i)).toString();
        t.icon    = s.value("icon",    "").toString();
        t.command = s.value("command", "").toString();
        t.args    = s.value("args",    "").toString();
        s.endGroup();
        tiles.append(t);
    }
    return tiles;
}

QString TileLauncher::loadFont()
{
    // Utiliser la police systeme Windows 11 : Segoe UI Variable
    // Disponible nativement sur Windows 11, fallback Segoe UI sur Win10
    QFontDatabase db;
    if(db.families().contains("Segoe UI Variable"))
        return "Segoe UI Variable";
    return "Segoe UI";
}

// ════════════════════════════════════════════════════════════════════════════
// TileLauncher — constructor & UI
// ════════════════════════════════════════════════════════════════════════════

TileLauncher::TileLauncher(QWidget* parent) : QWidget(parent)
{
    Logger::instance().open(logPath(), datPath());
    Logger::instance().logLaunch();

    m_fontFamily = loadFont();
    const QString ini = iniPath();
    m_appCfg = loadAppConfig(ini);
    m_tiles  = loadTiles(ini);

    setupWindow();
    buildUi();
    positionWindow();
}

void TileLauncher::setupWindow()
{
    // ── Style Windows 11 : fenêtre native avec coins arrondis ────────────
    // On garde FramelessWindowHint pour la barre custom mais on active
    // l'effet Mica/acrylique via WA_TranslucentBackground + DWM
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(m_appCfg.title);
    setMinimumSize(MIN_W, MIN_H);

    // Taille initiale : restaurée ou 1/4 d'écran
    const AppData& d = Logger::instance().data();
    QScreen* screen  = QGuiApplication::primaryScreen();
    QRect    geo     = screen->availableGeometry();
    int w = d.hasSize() ? d.windowW : geo.width()  / 2;
    int h = d.hasSize() ? d.windowH : geo.height() / 2;
    resize(w, h);
}

void TileLauncher::buildUi()
{
    m_container = new QFrame(this);
    m_container->setObjectName("main");
    m_container->setGeometry(0, 0, width(), height());
    m_container->setStyleSheet(QString(
        "QFrame#main{background:%1;border:1.5px solid %2;border-radius:12px;}"
    ).arg(BG_COLOR, BORDER_COLOR));

    auto* root = new QVBoxLayout(m_container);
    root->setContentsMargins(0, 0, 0, MARGIN);
    root->setSpacing(MARGIN);

    // ── Barre de titre ────────────────────────────────────────────────────
    auto* tbar = new QFrame;
    tbar->setObjectName("tb");
    tbar->setFixedHeight(TITLEBAR_H);
    tbar->setStyleSheet(QString(
        "QFrame#tb{background:%1;"
        "border-top-left-radius:11px;border-top-right-radius:11px;"
        "border-bottom:1px solid rgba(0,0,0,0.15);}"
    ).arg(TITLEBAR_BG));

    auto* tb = new QHBoxLayout(tbar);
    tb->setContentsMargins(10,0,6,0); tb->setSpacing(4);

    m_titleLabel = new QLabel(m_appCfg.title);
    m_titleLabel->setFont(QFont(m_fontFamily,11,QFont::Bold));
    m_titleLabel->setStyleSheet(
        QString("color:%1;letter-spacing:0.5px;").arg(TITLEBAR_TEXT));

    const QString btnBase = QString(
        "QPushButton{background:transparent;color:%1;border:none;"
        "font-size:15px;font-weight:bold;border-radius:4px;}"
        "QPushButton:hover{background:rgba(0,0,0,0.12);}"
    ).arg(TITLEBAR_TEXT);

    auto* reloadBtn = new QPushButton("↺");
    reloadBtn->setFixedSize(26,26);
    reloadBtn->setCursor(Qt::PointingHandCursor);
    reloadBtn->setToolTip("Reload configuration");
    reloadBtn->setStyleSheet(btnBase);
    connect(reloadBtn, &QPushButton::clicked, this, [this](){
        const QString ini = iniPath();
        m_appCfg = loadAppConfig(ini);
        m_tiles  = loadTiles(ini);
        m_titleLabel->setText(m_appCfg.title);
        populateGrid(m_tiles);
    });

    auto* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(26,26);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QString(
        "QPushButton{background:transparent;color:%1;border:none;"
        "font-size:11px;font-weight:bold;border-radius:4px;}"
        "QPushButton:hover{background:rgba(220,60,60,0.85);color:#fff;}"
    ).arg(TITLEBAR_TEXT));
    connect(closeBtn, &QPushButton::clicked, this, &TileLauncher::close);

    tb->addWidget(m_titleLabel);
    tb->addStretch();
    tb->addWidget(reloadBtn);
    tb->addWidget(closeBtn);
    root->addWidget(tbar);

    // ── Grille ────────────────────────────────────────────────────────────
    m_gridWidget = new QWidget;
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setSpacing(SPACING);
    m_gridLayout->setContentsMargins(MARGIN,0,MARGIN,0);
    populateGrid(m_tiles);
    root->addWidget(m_gridWidget, 1);
}

QSize TileLauncher::tileSize() const
{
    int uw = width()  - 2*MARGIN - (TILE_COLS-1)*SPACING;
    int uh = height() - TITLEBAR_H - MARGIN*2 - (TILE_ROWS-1)*SPACING;
    return {uw/TILE_COLS, uh/TILE_ROWS};
}

void TileLauncher::populateGrid(const QList<TileData>& tiles)
{
    while(m_gridLayout->count()){
        auto* item = m_gridLayout->takeAt(0);
        if(item->widget()) item->widget()->deleteLater();
        delete item;
    }
    QSize sz = tileSize();
    for(int i=0;i<tiles.size();++i){
        auto* btn = new TileButton(tiles[i],sz.width(),sz.height(),
                                   m_fontFamily, m_gridWidget);
        m_gridLayout->addWidget(btn, i/TILE_COLS, i%TILE_COLS);
    }
}

void TileLauncher::resizeTiles()
{
    QSize sz = tileSize();
    for(int i=0;i<m_gridLayout->count();++i){
        if(auto* btn = qobject_cast<TileButton*>(m_gridLayout->itemAt(i)->widget()))
            btn->resize(sz.width(), sz.height());
    }
}

void TileLauncher::positionWindow()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect    geo    = screen->availableGeometry();
    const AppData& d = Logger::instance().data();

    if(d.hasPosition()){
        QPoint saved = d.position();
        bool onScreen = false;
        for(QScreen* s : QGuiApplication::screens())
            if(s->availableGeometry().contains(saved)){ onScreen=true; break; }
        if(onScreen){
            move(saved);
            sendToBottom();
            Logger::instance().log(Logger::INFO,
                QString("Position restored: X=%1 Y=%2 W=%3 H=%4")
                    .arg(saved.x()).arg(saved.y()).arg(width()).arg(height()));
            return;
        }
    }
    move(geo.right()-width(), geo.top());
    sendToBottom();
}

void TileLauncher::sendToBottom()
{
#ifdef Q_OS_WIN
    SetWindowPos(reinterpret_cast<HWND>(winId()),
                 HWND_BOTTOM,0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif
}

// ── Redimensionnement ─────────────────────────────────────────────────────────
ResizeEdge TileLauncher::edgeAt(const QPoint& p) const
{
    bool L = p.x() < RESIZE_BORDER;
    bool R = p.x() > width()  - RESIZE_BORDER;
    bool T = p.y() < RESIZE_BORDER;
    bool B = p.y() > height() - RESIZE_BORDER;
    if (L && T) return ResizeEdge::TopLeft;
    if (R && T) return ResizeEdge::TopRight;
    if (L && B) return ResizeEdge::BottomLeft;
    if (R && B) return ResizeEdge::BottomRight;
    if (L)      return ResizeEdge::Left;
    if (R)      return ResizeEdge::Right;
    if (T)      return ResizeEdge::Top;
    if (B)      return ResizeEdge::Bottom;
    return ResizeEdge::None;
}

void TileLauncher::updateCursor(ResizeEdge edge)
{
    switch(edge){
        case ResizeEdge::Left:
        case ResizeEdge::Right:        setCursor(Qt::SizeHorCursor);  break;
        case ResizeEdge::Top:
        case ResizeEdge::Bottom:       setCursor(Qt::SizeVerCursor);  break;
        case ResizeEdge::TopLeft:
        case ResizeEdge::BottomRight:  setCursor(Qt::SizeFDiagCursor); break;
        case ResizeEdge::TopRight:
        case ResizeEdge::BottomLeft:   setCursor(Qt::SizeBDiagCursor); break;
        default:                       unsetCursor();                  break;
    }
}

void TileLauncher::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if(m_container) m_container->setGeometry(0,0,width(),height());
    resizeTiles();
}

// ── Mouse events ──────────────────────────────────────────────────────────────
void TileLauncher::mousePressEvent(QMouseEvent* e)
{
    if(e->button() != Qt::LeftButton) return;
    ResizeEdge edge = edgeAt(e->pos());
    if(edge != ResizeEdge::None){
        m_resizeEdge  = edge;
        m_resizeStart = e->globalPosition().toPoint();
        m_resizeGeom  = geometry();
    } else if(e->pos().y() < TITLEBAR_H){
        m_dragging = true;
        m_dragPos  = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void TileLauncher::mouseMoveEvent(QMouseEvent* e)
{
    if(m_resizeEdge != ResizeEdge::None){
        QPoint delta = e->globalPosition().toPoint() - m_resizeStart;
        QRect  r     = m_resizeGeom;
        switch(m_resizeEdge){
            case ResizeEdge::Right:       r.setRight(r.right()+delta.x());   break;
            case ResizeEdge::Left:        r.setLeft(r.left()+delta.x());     break;
            case ResizeEdge::Bottom:      r.setBottom(r.bottom()+delta.y()); break;
            case ResizeEdge::Top:         r.setTop(r.top()+delta.y());       break;
            case ResizeEdge::BottomRight: r.setRight(r.right()+delta.x());
                                          r.setBottom(r.bottom()+delta.y()); break;
            case ResizeEdge::BottomLeft:  r.setLeft(r.left()+delta.x());
                                          r.setBottom(r.bottom()+delta.y()); break;
            case ResizeEdge::TopRight:    r.setRight(r.right()+delta.x());
                                          r.setTop(r.top()+delta.y());       break;
            case ResizeEdge::TopLeft:     r.setLeft(r.left()+delta.x());
                                          r.setTop(r.top()+delta.y());       break;
            default: break;
        }
        // Appliquer la taille minimale
        if(r.width() >= MIN_W && r.height() >= MIN_H)
            setGeometry(r);
    } else if(m_dragging){
        move(e->globalPosition().toPoint() - m_dragPos);
    } else {
        updateCursor(edgeAt(e->pos()));
    }
}

void TileLauncher::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    m_resizeEdge = ResizeEdge::None;
    m_dragging   = false;
    updateCursor(ResizeEdge::None);
}

void TileLauncher::closeEvent(QCloseEvent* e)
{
    Logger::instance().logClose(pos(), size());
    e->accept();
    QApplication::instance()->quit();
}
