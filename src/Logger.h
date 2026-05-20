#pragma once

#include "AppData.h"
#include <QString>
#include <QFile>
#include <QElapsedTimer>
#include <QProcess>
#include <QMap>
#include <QPoint>
#include <QSize>

// ── Logger singleton ──────────────────────────────────────────────────────────
// Journal texte  -> TileLauncher.log  (append, UTF-8, CRLF)
// Donnees binaires -> TileLauncher.dat (AppData, 80 octets, CRC32)

class Logger
{
public:
    enum Level { INFO, ACTION, WARNING, ERROR_L };

    static Logger& instance();

    void open(const QString& logPath, const QString& datPath);
    void log(Level level, const QString& message);
    void logLaunch();
    void logClose(const QPoint& windowPos, const QSize& windowSize);
    void logTileAction(const QString& label, const QString& command);
    void logTileClose(const QString& label, const QString& command,
                      qint64 durationMs);

    const AppData& data() const { return m_data; }
    AppData&       data()       { return m_data; }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void    writeLine(const QString& line);
    QString levelStr(Level l) const;
    QString timestamp() const;

    QFile         m_logFile;
    QElapsedTimer m_sessionTimer;
    AppData       m_data;
    QString       m_datPath;
    bool          m_open = false;
};
