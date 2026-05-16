#pragma once

#include "Logger.h"

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QSettings>
#include <QList>
#include <QString>
#include <QPixmap>
#include <QTimer>
#include <QPoint>

// ── Données d'une tuile ───────────────────────────────────────────────────────
struct TileData {
    QString label;
    QString icon;
    QString command;
    QString args;
};

// ── Constantes layout ─────────────────────────────────────────────────────────
static constexpr int TILE_ROWS  = 2;
static constexpr int TILE_COLS  = 4;
static constexpr int NUM_TILES  = TILE_ROWS * TILE_COLS;
static constexpr int LABEL_H    = 22;
static constexpr int TITLEBAR_H = 36;
static constexpr int MARGIN     = 6;
static constexpr int SPACING    = 5;

// ── Palette ───────────────────────────────────────────────────────────────────
static const QString BG_COLOR      = "#0f1117";
static const QString TITLEBAR_BG   = "#00FFDA";
static const QString TITLEBAR_TEXT = "#000000";
static const QString TILE_BG       = "#1a1d27";
static const QString TILE_HOVER    = "#252840";
static const QString TILE_PRESS    = "#1e2235";
static const QString ACCENT        = "#00FFDA";
static const QString ACCENT2       = "#00ccb0";
static const QString TEXT_COLOR    = "#e8eaf6";
static const QString BORDER_COLOR  = "#007d6b";

// ── Widget Tuile ──────────────────────────────────────────────────────────────
class TileButton : public QFrame {
    Q_OBJECT
public:
    explicit TileButton(const TileData& data, int w, int h,
                        const QString& fontFamily, QWidget* parent = nullptr);

protected:
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    void updateStyle();
    QPixmap buildPixmap(int w, int h);
    QPixmap extractExeIcon(const QString& exePath, int w, int h);
    QPixmap scaleAndCrop(const QPixmap& src, int w, int h);

    TileData    m_data;
    QString     m_fontFamily;
    QLabel*     m_iconLabel  = nullptr;
    QLabel*     m_textLabel  = nullptr;
    bool        m_hovered    = false;
    bool        m_pressed    = false;

    static const QString STYLE_NORMAL;
    static const QString STYLE_HOVER;
    static const QString STYLE_PRESS;
    static const QString STYLE_LABEL;
};

// ── Fenêtre principale ────────────────────────────────────────────────────────
class TileLauncher : public QWidget {
    Q_OBJECT
public:
    explicit TileLauncher(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

public slots:
    void sendToBottom();

private:
    void setupWindow();
    void buildUi();
    void populateGrid(const QList<TileData>& tiles);
    void positionWindow();
    QSize tileSize() const;

    struct AppConfig {
        QString title;
    };

    AppConfig           m_appCfg;
    QList<TileData>     m_tiles;
    QString             m_fontFamily;
    QLabel*             m_titleLabel  = nullptr;
    QGridLayout*        m_gridLayout  = nullptr;
    QWidget*            m_gridWidget  = nullptr;
    QPoint              m_dragPos;
    bool                m_dragging    = false;

public:
    // Chemins fichiers — accessibles depuis TileButton
    static QString        iniPath();
    static QString        logPath();
    static QString        statsPath();

private:

    static AppConfig       loadAppConfig(const QString& iniPath);
    static QList<TileData> loadTiles(const QString& iniPath);
    static QString         loadFont();
};
