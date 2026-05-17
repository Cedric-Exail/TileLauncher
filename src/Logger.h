#pragma once

#include "AppData.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>

// ── Logger singleton ──────────────────────────────────────────────────────────
// Journal texte  → TileLauncher.log  (append)
// Données binaires → TileLauncher.dat (AppData, 64 octets, CRC32)

class Logger
{
public:
    enum Level { INFO, ACTION, WARNING, ERROR_L };

    static Logger& instance();

    // Initialisation (une seule fois au démarrage)
    void open(const QString& logPath, const QString& datPath);

    // Journal
    void log(Level level, const QString& message);
    void logLaunch();
    void logClose(const QPoint& windowPos, const QSize& windowSize);
    void logTileAction(const QString& label, const QString& command);

    // Accès aux données persistées
    const AppData& data() const { return m_data; }
    AppData&       data()       { return m_data; }
    QString        formattedTotalUsage() const { return m_data.formattedUsage(); }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QString levelStr(Level l) const;
    QString timestamp() const;

    QFile         m_logFile;
    QTextStream   m_stream;
    QElapsedTimer m_sessionTimer;
    AppData       m_data;
    QString       m_datPath;
    bool          m_open = false;
};
