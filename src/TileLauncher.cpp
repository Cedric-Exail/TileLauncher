#include "TileLauncher.h"
#include "Logger.h"
#include "AppData.h"

#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QEnterEvent>
#include <QCloseEvent>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

// ── Styles statiques ──────────────────────────────────────────────────────────
const QString TileButton::STYLE_NORMAL = QString(
    "TileButton{background:%1;border:1px solid %2;border-radius:8px;}"
).arg(TILE_BG, BORDER_COLOR);
const QString TileButton::STYLE_HOVER = QString(
    "TileButton{background:%1;border:1px solid %2;border-radius:8px;}"
).arg(TILE_HOVER, ACCENT);
const QString TileButton::STYLE_PRESS = QString(
    "TileButton{background:%1;border:1px solid %2;border-radius:8px;}"
).arg(TILE_PRESS, ACCENT);
const QString TileButton::STYLE_LABEL = QString(
    "QLabel{color:%1;background:rgba(10,12,20,0.72);"
    "border-bottom-left-radius:7px;border-bottom-right-radius:7px;padding:0 4px;}"
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
    m_iconLabel->setPixmap(buildPixmap(w, h - LABEL_H));
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
    if (w > 0 && h > 0)
        m_iconLabel->setPixmap(buildPixmap(w, h - LABEL_H));
}

void TileButton::updateStyle()
{
    if (m_pressed)       setStyleSheet(STYLE_PRESS);
    else if (m_hovered)  setStyleSheet(STYLE_HOVER);
    else                 setStyleSheet(STYLE_NORMAL);
}

QPixmap TileButton::scaleAndCrop(const QPixmap& src, int w, int h)
{
    return src.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap TileButton::extractExeIcon(const QString& exePath, int w, int h)
{
#ifdef Q_OS_WIN
    // Résoudre les variables d'environnement dans le chemin
    QString resolved = exePath;
    resolved = QDir::fromNativeSeparators(resolved);

    // Remplacer %VARNAME% par les valeurs système
    QRegularExpression envVar("%([^%]+)%");
    auto it = envVar.globalMatch(resolved);
    while (it.hasNext()) {
        auto match = it.next();
        QString varName = match.captured(1);
        QString varVal  = qEnvironmentVariable(qPrintable(varName));
        if (!varVal.isEmpty())
            resolved.replace(match.captured(0), varVal);
    }
    resolved = QDir::toNativeSeparators(resolved);

    if (resolved.isEmpty() || !QFileInfo::exists(resolved)) return {};

    const int iconSize = qMax(w, h) >= 48 ? 48 : 32;
    HICON hLarge[1]={}, hSmall[1]={};
    UINT n = ExtractIconExW(
        reinterpret_cast<const wchar_t*>(resolved.utf16()),
        0, hLarge, hSmall, 1);
    if (n == 0) return {};
    HICON hIcon = hLarge[0] ? hLarge[0] : hSmall[0];
    if (!hIcon) return {};

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem    = CreateCompatibleDC(hdcScreen);

    BITMAPINFOHEADER bmi{};
    bmi.biSize    = sizeof(bmi);
    bmi.biWidth   = iconSize;
    bmi.biHeight  = -iconSize;
    bmi.biPlanes  = 1;
    bmi.biBitCount = 32;

    void* pvBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem,
        reinterpret_cast<BITMAPINFO*>(&bmi),
        DIB_RGB_COLORS, &pvBits, nullptr, 0);
    SelectObject(hdcMem, hBmp);
    DrawIconEx(hdcMem, 0, 0, hIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);

    const int bufSize = iconSize * iconSize * 4;
    QByteArray buf(bufSize, Qt::Uninitialized);
    GetBitmapBits(hBmp, bufSize, buf.data());

    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    if (hLarge[0]) DestroyIcon(hLarge[0]);
    if (hSmall[0]) DestroyIcon(hSmall[0]);

    // BGRA -> RGBA
    QByteArray rgba(bufSize, Qt::Uninitialized);
    for (int i = 0; i < bufSize; i += 4) {
        rgba[i]   = buf[i+2];
        rgba[i+1] = buf[i+1];
        rgba[i+2] = buf[i];
        rgba[i+3] = buf[i+3];
    }
    QImage img(reinterpret_cast<const uchar*>(rgba.constData()),
               iconSize, iconSize, iconSize * 4, QImage::Format_RGBA8888);
    QPixmap pix = QPixmap::fromImage(img.copy());
    return pix.isNull() ? QPixmap{} : scaleAndCrop(pix, w, h);
#else
    Q_UNUSED(exePath); Q_UNUSED(w); Q_UNUSED(h);
    return {};
#endif
}

QPixmap TileButton::buildPixmap(int w, int h)
{
    // 1. Image définie dans le .ini
    if (!m_data.icon.isEmpty()) {
        QString path = m_data.icon;
        // Résoudre chemin relatif par rapport au dossier de l'ini
        if (!QFileInfo(path).isAbsolute())
            path = QDir(TileLauncher::iniDir()).absoluteFilePath(path);
        path = QDir::toNativeSeparators(path);
        if (QFileInfo::exists(path)) {
            QPixmap src(path);
            if (!src.isNull()) return scaleAndCrop(src, w, h);
        }
    }

    // 2. Icône native de l'exe
    if (!m_data.command.isEmpty()) {
        // Nettoyer le chemin : supprimer guillemets, espaces superflus
        QString exe = m_data.command.trimmed();
        if (exe.startsWith('"') && exe.endsWith('"'))
            exe = exe.mid(1, exe.length() - 2);
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
    p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, initial);
    return pix;
}

void TileButton::enterEvent(QEnterEvent* e)
{ Q_UNUSED(e); m_hovered = true;  updateStyle(); }

void TileButton::leaveEvent(QEvent* e)
{ Q_UNUSED(e); m_hovered = false; m_pressed = false; updateStyle(); }

void TileButton::mousePressEvent(QMouseEvent* e)
{ if (e->button() == Qt::LeftButton) { m_pressed = true; updateStyle(); } }

void TileButton::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        updateStyle();

        if (!m_data.command.isEmpty()) {
            Logger::instance().logTileAction(m_data.label, m_data.command);

            // Nettoyer le chemin exe
            QString cmd = m_data.command.trimmed();
            if (cmd.length() >= 2 && cmd.front() == QChar('"') && cmd.back() == QChar('"'))
                cmd = cmd.mid(1, cmd.length() - 2);

            // Resoudre variables environnement Windows (%WINDIR%, etc.)
            QRegularExpression envRx("%([^%]+)%");
            auto it = envRx.globalMatch(cmd);
            while (it.hasNext()) {
                auto match = it.next();
                QString val = qEnvironmentVariable(
                    match.captured(1).toUtf8().constData());
                if (!val.isEmpty()) cmd.replace(match.captured(0), val);
            }
            cmd = QDir::toNativeSeparators(cmd);

            QStringList argList;
            if (!m_data.args.isEmpty())
                argList = QProcess::splitCommand(m_data.args);

            QFileInfo fi(cmd);
            QString workDir = fi.isFile() ? fi.absolutePath() : QString();

            qint64 pid = 0;
            bool ok = QProcess::startDetached(cmd, argList, workDir, &pid);
            if (!ok) {
                Logger::instance().log(Logger::WARNING,
                    QString("Failed to launch: %1").arg(cmd));
            } else if (pid > 0) {
                // Surveiller le processus pour logger sa fermeture
                QString lbl = m_data.label;
                QString ccmd = m_data.command;
                auto* watcher = new ProcessWatcher(pid, lbl, ccmd, qApp);
                QObject::connect(watcher, &ProcessWatcher::processFinished,
                    qApp, [](const QString& label,
                             const QString& command,
                             qint64 durationMs) {
                        Logger::instance().logTileClose(label, command, durationMs);
                    });
                QObject::connect(watcher, &ProcessWatcher::finished,
                    watcher, &QObject::deleteLater);
                watcher->start(QThread::LowPriority);
            }
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// TileLauncher — statics
// ════════════════════════════════════════════════════════════════════════════

QString TileLauncher::iniDir() {
    return QCoreApplication::applicationDirPath();
}
QString TileLauncher::iniPath() {
    return QDir(iniDir()).absoluteFilePath("tiles.ini");
}
QString TileLauncher::logPath() {
    return QDir(iniDir()).absoluteFilePath("TileLauncher.log");
}
QString TileLauncher::datPath() {
    QString internal = QDir(iniDir()).absoluteFilePath("_internal");
    QDir().mkpath(internal);
    return QDir(internal).absoluteFilePath("TileLauncher.dat");
}

static QString readIniValue(const QString& filePath,
                             const QString& section,
                             const QString& key,
                             const QString& defaultVal = QString())
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return defaultVal;

    bool inSection = false;
    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#'))
            continue;
        if (line.startsWith('[')) {
            QString sec = line.mid(1, line.indexOf(']') - 1).trimmed();
            inSection = (sec.compare(section, Qt::CaseInsensitive) == 0);
            continue;
        }
        if (inSection) {
            int eq = line.indexOf('=');
            if (eq < 0) continue;
            QString k = line.left(eq).trimmed();
            if (k.compare(key, Qt::CaseInsensitive) == 0) {
                // Supprimer commentaire inline apres ;
                QString v = line.mid(eq + 1);
                int sc = v.indexOf(" ;");
                if (sc >= 0) v = v.left(sc);
                return v.trimmed();
            }
        }
    }
    return defaultVal;
}

TileLauncher::AppConfig TileLauncher::loadAppConfig(const QString& path)
{
    AppConfig cfg;
    cfg.title = readIniValue(path, "App", "title", "TileLauncher");
    return cfg;
}

// Lecture manuelle du .ini pour eviter que QSettings interprete
// les backslashes Windows comme sequences d'echappement (\W -> W, 	 -> TAB etc.)
QList<TileData> TileLauncher::loadTiles(const QString& path)
{
    QList<TileData> tiles;
    for (int i = 1; i <= NUM_TILES; ++i) {
        QString sec = QString("Tile%1").arg(i);
        TileData t;
        t.label   = readIniValue(path, sec, "label",   QString("Tile %1").arg(i));
        t.icon    = readIniValue(path, sec, "icon",    "");
        t.command = readIniValue(path, sec, "command", "");
        t.args    = readIniValue(path, sec, "args",    "");
        tiles.append(t);
    }
    return tiles;
}

QString TileLauncher::loadFont()
{
    QFontDatabase db;
    if (db.families().contains("Segoe UI Variable"))
        return "Segoe UI Variable";
    return "Segoe UI";
}

// ════════════════════════════════════════════════════════════════════════════
// TileLauncher — UI
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
    setupTray();
    positionWindow();
}

void TileLauncher::setupWindow()
{
    // Icone embarquee dans les ressources Qt
    QIcon appIcon;
    appIcon.addFile(":/icons/icon_16.png",  QSize(16,16));
    appIcon.addFile(":/icons/icon_32.png",  QSize(32,32));
    appIcon.addFile(":/icons/icon_48.png",  QSize(48,48));
    appIcon.addFile(":/icons/icon_256.png", QSize(256,256));
    QApplication::setWindowIcon(appIcon);

    // Style natif Windows 11 — barre titre native avec boutons min/max/close
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(m_appCfg.title);
    setMinimumSize(MIN_W, MIN_H);

    // Restaurer taille sauvegardée ou utiliser 1/4 ecran par defaut
    const AppData& d = Logger::instance().data();
    QScreen* screen  = QGuiApplication::primaryScreen();
    QRect    geo     = screen->availableGeometry();
    int w = d.hasSize() ? d.windowW : geo.width()  / 2;
    int h = d.hasSize() ? d.windowH : geo.height() / 2;
    QWidget::resize(w, h);
}

void TileLauncher::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    root->setSpacing(MARGIN);

    // Grille de tuiles directement dans la fenêtre native
    m_gridWidget = new QWidget(this);
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setSpacing(SPACING);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    populateGrid(m_tiles);
    root->addWidget(m_gridWidget, 1);
}

void TileLauncher::setupTray()
{
    m_trayIcon = new QSystemTrayIcon(QApplication::windowIcon(), this);
    m_trayIcon->setToolTip(m_appCfg.title);

    auto* menu = new QMenu(this);

    QAction* restoreAct = menu->addAction("Show / Restore");
    QObject::connect(restoreAct, &QAction::triggered, this, [this]() {
        raiseWindow();
    });

    menu->addSeparator();

    QAction* quitAct = menu->addAction("Quit");
    QObject::connect(quitAct, &QAction::triggered, this, &TileLauncher::close);

    m_trayIcon->setContextMenu(menu);
    m_trayIcon->show();

    // Clic simple sur l icone systray -> afficher/masquer
    QObject::connect(m_trayIcon, &QSystemTrayIcon::activated,
        this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger ||
                reason == QSystemTrayIcon::DoubleClick) {
                raiseWindow();
            }
        });
}

QSize TileLauncher::tileSize() const
{
    int uw = width()  - 2*MARGIN - (TILE_COLS-1)*SPACING;
    int uh = height() - 2*MARGIN - (TILE_ROWS-1)*SPACING;
    return {qMax(1, uw/TILE_COLS), qMax(1, uh/TILE_ROWS)};
}

void TileLauncher::populateGrid(const QList<TileData>& tiles)
{
    while (m_gridLayout->count()) {
        auto* item = m_gridLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    QSize sz = tileSize();
    for (int i = 0; i < tiles.size(); ++i)
        m_gridLayout->addWidget(
            new TileButton(tiles[i], sz.width(), sz.height(),
                           m_fontFamily, m_gridWidget),
            i / TILE_COLS, i % TILE_COLS);
}

void TileLauncher::resizeTiles()
{
    QSize sz = tileSize();
    for (int i = 0; i < m_gridLayout->count(); ++i)
        if (auto* btn = qobject_cast<TileButton*>(m_gridLayout->itemAt(i)->widget()))
            btn->resize(sz.width(), sz.height());
}

void TileLauncher::positionWindow()
{
    const AppData& d = Logger::instance().data();
    QScreen* screen  = QGuiApplication::primaryScreen();
    QRect    geo     = screen->availableGeometry();

    if (d.hasPosition()) {
        QPoint saved = d.position();
        bool onScreen = false;
        for (QScreen* s : QGuiApplication::screens())
            if (s->availableGeometry().contains(saved)) { onScreen = true; break; }
        if (onScreen) {
            move(saved);
            Logger::instance().log(Logger::INFO,
                QString("Position restored: X=%1 Y=%2 W=%3 H=%4")
                    .arg(saved.x()).arg(saved.y()).arg(width()).arg(height()));
            return;
        }
    }
    // Par défaut : coin supérieur droit
    move(geo.right() - width(), geo.top());
}

void TileLauncher::raiseWindow()
{
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
#ifdef Q_OS_WIN
    ::SetForegroundWindow(reinterpret_cast<HWND>(winId()));
#endif
}

void TileLauncher::sendToBottom()
{
#ifdef Q_OS_WIN
    SetWindowPos(reinterpret_cast<HWND>(winId()),
                 HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}

void TileLauncher::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    resizeTiles();
}

void TileLauncher::closeEvent(QCloseEvent* e)
{
    // Sauvegarder position/taille a chaque fermeture/minimisation
    Logger::instance().logClose(pos(), size());
    e->accept();
    QApplication::instance()->quit();
}

// Drag et resize : délégués entièrement au chrome Windows natif
void TileLauncher::mousePressEvent(QMouseEvent* e)   { QWidget::mousePressEvent(e);   }
void TileLauncher::mouseMoveEvent(QMouseEvent* e)    { QWidget::mouseMoveEvent(e);    }
void TileLauncher::mouseReleaseEvent(QMouseEvent* e) { QWidget::mouseReleaseEvent(e); }
