#include "TileLauncher.h"
#include "Logger.h"

#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QCloseEvent>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QImageReader>
#include <QImage>

// Windows API pour ExtractIconEx et SetWindowPos
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

// ── Styles précalculés ────────────────────────────────────────────────────────
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

    // Icône
    m_iconLabel = new QLabel(this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_iconLabel->setPixmap(buildPixmap(w, h - LABEL_H));
    layout->addWidget(m_iconLabel, 1);

    // Label
    m_textLabel = new QLabel(data.label, this);
    m_textLabel->setFixedHeight(LABEL_H);
    m_textLabel->setAlignment(Qt::AlignCenter);
    QFont f(fontFamily, 9, QFont::DemiBold);
    m_textLabel->setFont(f);
    m_textLabel->setStyleSheet(STYLE_LABEL);
    layout->addWidget(m_textLabel);
}

void TileButton::updateStyle()
{
    if (m_pressed)       setStyleSheet(STYLE_PRESS);
    else if (m_hovered)  setStyleSheet(STYLE_HOVER);
    else                 setStyleSheet(STYLE_NORMAL);
}

QPixmap TileButton::scaleAndCrop(const QPixmap& src, int w, int h)
{
    QPixmap pix = src.scaled(w, h, Qt::KeepAspectRatioByExpanding,
                              Qt::SmoothTransformation);
    if (pix.width() > w || pix.height() > h) {
        int x = qMax(0, (pix.width()  - w) / 2);
        int y = qMax(0, (pix.height() - h) / 2);
        pix = pix.copy(x, y, qMin(w, pix.width()), qMin(h, pix.height()));
    }
    return pix;
}

QPixmap TileButton::extractExeIcon(const QString& exePath, int w, int h)
{
#ifdef Q_OS_WIN
    if (exePath.isEmpty() || !QFileInfo::exists(exePath))
        return {};

    const int iconSize = qMax(w, h) >= 48 ? 48 : 32;

    HICON hLarge[1] = {}, hSmall[1] = {};
    UINT n = ExtractIconExW(
        reinterpret_cast<const wchar_t*>(exePath.utf16()),
        0, hLarge, hSmall, 1);

    if (n == 0) return {};

    HICON hIcon = hLarge[0] ? hLarge[0] : hSmall[0];
    if (!hIcon) return {};

    // Rendre l'icône dans un DIBSection BGRA
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem    = CreateCompatibleDC(hdcScreen);

    BITMAPINFOHEADER bmi{};
    bmi.biSize     = sizeof(BITMAPINFOHEADER);
    bmi.biWidth    = iconSize;
    bmi.biHeight   = -iconSize;   // top-down
    bmi.biPlanes   = 1;
    bmi.biBitCount = 32;

    void* pvBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem,
        reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS, &pvBits, nullptr, 0);
    SelectObject(hdcMem, hBmp);
    DrawIconEx(hdcMem, 0, 0, hIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);

    // Lire les pixels BGRA
    const int bufSize = iconSize * iconSize * 4;
    QByteArray buf(bufSize, Qt::Uninitialized);
    GetBitmapBits(hBmp, bufSize, buf.data());

    // Nettoyage GDI
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    if (hLarge[0]) DestroyIcon(hLarge[0]);
    if (hSmall[0]) DestroyIcon(hSmall[0]);

    // BGRA → RGBA
    QByteArray rgba(bufSize, Qt::Uninitialized);
    for (int i = 0; i < bufSize; i += 4) {
        rgba[i]   = buf[i + 2]; // R
        rgba[i+1] = buf[i + 1]; // G
        rgba[i+2] = buf[i];     // B
        rgba[i+3] = buf[i + 3]; // A
    }

    QImage img(reinterpret_cast<const uchar*>(rgba.constData()),
               iconSize, iconSize, iconSize * 4,
               QImage::Format_RGBA8888);
    QPixmap pix = QPixmap::fromImage(img.copy()); // .copy() pour détacher du buffer
    return pix.isNull() ? QPixmap{} : scaleAndCrop(pix, w, h);
#else
    Q_UNUSED(exePath); Q_UNUSED(w); Q_UNUSED(h);
    return {};
#endif
}

QPixmap TileButton::buildPixmap(int w, int h)
{
    // 1. Icône explicite
    if (!m_data.icon.isEmpty()) {
        QString path = m_data.icon;
        if (!QFileInfo(path).isAbsolute())
            path = QDir(TileLauncher::iniPath()).absoluteFilePath(path);
        if (QFileInfo::exists(path)) {
            QPixmap src(path);
            if (!src.isNull()) return scaleAndCrop(src, w, h);
        }
    }

    // 2. Icône extraite de l'exe
    if (!m_data.command.isEmpty()) {
        QString exe = m_data.command.trimmed().remove('"');
        QPixmap pix = extractExeIcon(exe, w, h);
        if (!pix.isNull()) return pix;
    }

    // 3. Fallback : dégradé + initiale
    QPixmap pix(w, h);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient grad(0, 0, w, h);
    grad.setColorAt(0, QColor(ACCENT));
    grad.setColorAt(1, QColor(ACCENT2));
    p.fillRect(0, 0, w, h, grad);
    p.setPen(Qt::white);
    QFont f(m_fontFamily, qMax(18, qMin(w, h) / 3), QFont::Bold);
    p.setFont(f);
    QString initial = m_data.label.isEmpty() ? QStringLiteral("?") : QString(m_data.label.at(0).toUpper());
    p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, initial);
    return pix;
}

void TileButton::enterEvent(QEnterEvent* e)
{
    Q_UNUSED(e);
    m_hovered = true;
    updateStyle();
}

void TileButton::leaveEvent(QEvent* e)
{
    Q_UNUSED(e);
    m_hovered = false;
    m_pressed = false;
    updateStyle();
}

void TileButton::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_pressed = true;
        updateStyle();
    }
}

void TileButton::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        updateStyle();

        if (!m_data.command.isEmpty()) {
            Logger::instance().logTileAction(m_data.label, m_data.command);
            QString cmd = m_data.command;
            QStringList args;
            if (!m_data.args.isEmpty())
                args = QProcess::splitCommand(m_data.args);
            QProcess::startDetached(cmd, args);
        }

        // Repasser en arrière-plan
        if (auto* win = qobject_cast<TileLauncher*>(window()))
            QMetaObject::invokeMethod(win, "sendToBottom", Qt::QueuedConnection);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// TileLauncher — statics
// ════════════════════════════════════════════════════════════════════════════

QString TileLauncher::iniPath()
{
    return QDir(QCoreApplication::applicationDirPath())
               .absoluteFilePath("tiles.ini");
}

QString TileLauncher::logPath()
{
    return QDir(QCoreApplication::applicationDirPath())
               .absoluteFilePath("TileLauncher.log");
}

QString TileLauncher::statsPath()
{
    return QDir(QCoreApplication::applicationDirPath())
               .absoluteFilePath("TileLauncher.stats");
}

TileLauncher::AppConfig TileLauncher::loadAppConfig(const QString& path)
{
    QSettings s(path, QSettings::IniFormat);
    AppConfig cfg;
    cfg.title = s.value("App/title", "TileLauncher").toString();
    return cfg;
}

QList<TileData> TileLauncher::loadTiles(const QString& path)
{
    QSettings s(path, QSettings::IniFormat);
    QList<TileData> tiles;
    for (int i = 1; i <= NUM_TILES; ++i) {
        s.beginGroup(QString("Tile%1").arg(i));
        TileData t;
        t.label   = s.value("label",   QString("Tuile %1").arg(i)).toString();
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
    QString fontsDir = QDir(QCoreApplication::applicationDirPath())
                           .absoluteFilePath("fonts");
    QDir dir(fontsDir);
    if (dir.exists()) {
        const auto files = dir.entryList({"Gilroy*.ttf"}, QDir::Files);
        for (const auto& f : files) {
            int id = QFontDatabase::addApplicationFont(dir.absoluteFilePath(f));
            if (id >= 0) {
                auto families = QFontDatabase::applicationFontFamilies(id);
                if (!families.isEmpty()) return families.first();
            }
        }
    }
    return "Segoe UI";
}

// ════════════════════════════════════════════════════════════════════════════
// TileLauncher — constructor & UI
// ════════════════════════════════════════════════════════════════════════════

TileLauncher::TileLauncher(QWidget* parent)
    : QWidget(parent)
{
    // Logger — ouverture avant toute action
    Logger::instance().open(logPath(), statsPath());
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
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(m_appCfg.title);

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect geo = screen->availableGeometry();
    setFixedSize(geo.width() / 2, geo.height() / 2);
}

void TileLauncher::buildUi()
{
    // Conteneur principal
    auto* container = new QFrame(this);
    container->setObjectName("main");
    container->setGeometry(0, 0, width(), height());
    container->setStyleSheet(QString(
        "QFrame#main{background:%1;border:1.5px solid %2;border-radius:14px;}"
    ).arg(BG_COLOR, BORDER_COLOR));

    auto* root = new QVBoxLayout(container);
    root->setContentsMargins(0, 0, 0, MARGIN);
    root->setSpacing(MARGIN);

    // ── Barre de titre ────────────────────────────────────────────────────
    auto* tbar = new QFrame;
    tbar->setObjectName("tb");
    tbar->setFixedHeight(TITLEBAR_H);
    tbar->setStyleSheet(QString(
        "QFrame#tb{background:%1;"
        "border-top-left-radius:13px;border-top-right-radius:13px;"
        "border-bottom:1px solid rgba(0,0,0,0.2);}"
    ).arg(TITLEBAR_BG));

    auto* tb = new QHBoxLayout(tbar);
    tb->setContentsMargins(10, 0, 6, 0);
    tb->setSpacing(4);

    m_titleLabel = new QLabel(m_appCfg.title);
    QFont tf(m_fontFamily, 11, QFont::Bold);
    m_titleLabel->setFont(tf);
    m_titleLabel->setStyleSheet(
        QString("color:%1;letter-spacing:0.5px;").arg(TITLEBAR_TEXT));

    const QString btnBase = QString(
        "QPushButton{background:transparent;color:%1;border:none;"
        "font-size:15px;font-weight:bold;}"
        "QPushButton:hover{background:rgba(0,0,0,0.12);border-radius:5px;}"
    ).arg(TITLEBAR_TEXT);

    auto* reloadBtn = new QPushButton("↺");
    reloadBtn->setFixedSize(26, 26);
    reloadBtn->setCursor(Qt::PointingHandCursor);
    reloadBtn->setToolTip("Recharger la configuration");
    reloadBtn->setStyleSheet(btnBase);
    connect(reloadBtn, &QPushButton::clicked, this, [this]() {
        const QString ini = iniPath();
        m_appCfg = loadAppConfig(ini);
        m_tiles  = loadTiles(ini);
        m_titleLabel->setText(m_appCfg.title);
        populateGrid(m_tiles);
    });

    auto* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(26, 26);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QString(
        "QPushButton{background:transparent;color:%1;border:none;"
        "font-size:11px;font-weight:bold;}"
        "QPushButton:hover{background:rgba(220,60,60,0.85);color:#ffffff;"
        "border-radius:5px;}"
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
    m_gridLayout->setContentsMargins(MARGIN, 0, MARGIN, 0);
    populateGrid(m_tiles);
    root->addWidget(m_gridWidget, 1);
}

QSize TileLauncher::tileSize() const
{
    int uw = width()  - 2 * MARGIN - (TILE_COLS - 1) * SPACING;
    int uh = height() - TITLEBAR_H - MARGIN * 2 - (TILE_ROWS - 1) * SPACING;
    return {uw / TILE_COLS, uh / TILE_ROWS};
}

void TileLauncher::populateGrid(const QList<TileData>& tiles)
{
    // Vider la grille
    while (m_gridLayout->count()) {
        auto* item = m_gridLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    QSize sz = tileSize();
    for (int i = 0; i < tiles.size(); ++i) {
        auto* btn = new TileButton(tiles[i], sz.width(), sz.height(),
                                   m_fontFamily, m_gridWidget);
        m_gridLayout->addWidget(btn, i / TILE_COLS, i % TILE_COLS);
    }
}

void TileLauncher::positionWindow()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect geo = screen->availableGeometry();
    move(geo.right() - width(), geo.top());
    sendToBottom();
}

void TileLauncher::sendToBottom()
{
#ifdef Q_OS_WIN
    SetWindowPos(reinterpret_cast<HWND>(winId()),
                 HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}

void TileLauncher::closeEvent(QCloseEvent* e)
{
    Logger::instance().logClose();
    e->accept();
    QApplication::instance()->quit();
}

void TileLauncher::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && e->pos().y() < TITLEBAR_H) {
        m_dragPos  = e->globalPosition().toPoint() - frameGeometry().topLeft();
        m_dragging = true;
    }
}

void TileLauncher::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging && e->buttons() & Qt::LeftButton)
        move(e->globalPosition().toPoint() - m_dragPos);
}

void TileLauncher::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    m_dragging = false;
}
