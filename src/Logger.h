#pragma once

#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>

// ── Logger singleton ──────────────────────────────────────────────────────────
// Écrit dans TileLauncher.log (même dossier que l'exe)
// Format : [2025-05-14 10:23:45.123] [INFO] Message
//
// Compteur permanent de temps d'utilisation stocké dans TileLauncher.stats
// (fichier texte, clé = total_seconds)

class Logger
{
public:
    enum Level { INFO, ACTION, WARNING, ERROR_L };

    static Logger& instance();

    // Ouvrir le log (appel unique au démarrage)
    void open(const QString& logPath, const QString& statsPath);

    // Écriture
    void log(Level level, const QString& message);
    void logLaunch();                               // Démarrage application
    void logClose();                                // Fermeture + durée session
    void logTileAction(const QString& label,
                       const QString& command);     // Clic tuile

    // Compteur permanent
    qint64 totalUsageSeconds() const;               // Temps cumulé toutes sessions
    QString formattedTotalUsage() const;            // "12h 34m 56s"

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeStats();
    void readStats();
    QString levelStr(Level l) const;
    QString timestamp() const;

    QFile           m_logFile;
    QTextStream     m_stream;
    QElapsedTimer   m_sessionTimer;     // Chrono de la session courante
    qint64          m_totalSeconds = 0; // Cumulé depuis la première utilisation
    QString         m_statsPath;
    bool            m_open = false;
};
