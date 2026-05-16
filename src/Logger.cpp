#include "Logger.h"

#include <QDir>
#include <QCoreApplication>
#include <QSettings>

// ── Singleton ─────────────────────────────────────────────────────────────────
Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

// ── Ouverture ─────────────────────────────────────────────────────────────────
void Logger::open(const QString& logPath, const QString& statsPath)
{
    m_statsPath = statsPath;

    // Lire le compteur permanent existant
    readStats();

    // Ouvrir le fichier log en mode append
    m_logFile.setFileName(logPath);
    m_logFile.open(QIODevice::Append | QIODevice::Text);
    m_stream.setDevice(&m_logFile);
    m_stream.setEncoding(QStringConverter::Utf8);

    m_open = true;
}

// ── Helpers ───────────────────────────────────────────────────────────────────
QString Logger::timestamp() const
{
    return QDateTime::currentDateTime()
               .toString("yyyy-MM-dd HH:mm:ss.zzz");
}

QString Logger::levelStr(Level l) const
{
    switch (l) {
        case INFO:    return "INFO   ";
        case ACTION:  return "ACTION ";
        case WARNING: return "WARNING";
        case ERROR_L: return "ERROR  ";
    }
    return "INFO   ";
}

// ── Écriture générique ────────────────────────────────────────────────────────
void Logger::log(Level level, const QString& message)
{
    if (!m_open) return;
    m_stream << QString("[%1] [%2] %3\n")
                    .arg(timestamp())
                    .arg(levelStr(level))
                    .arg(message);
    m_stream.flush();
}

// ── Événements applicatifs ────────────────────────────────────────────────────
void Logger::logLaunch()
{
    if (!m_open) return;

    m_sessionTimer.start();

    // Séparateur visuel entre sessions
    m_stream << "\n";
    m_stream << "╔══════════════════════════════════════════════════════╗\n";
    m_stream << QString("║  DÉMARRAGE  %1  ║\n")
                    .arg(timestamp(), -40);
    m_stream << QString("║  Temps total d'utilisation cumulé : %1  ║\n")
                    .arg(formattedTotalUsage(), -18);
    m_stream << "╚══════════════════════════════════════════════════════╝\n";
    m_stream.flush();

    log(INFO, "Application démarrée");
}

void Logger::logClose()
{
    if (!m_open) return;

    // Durée de cette session
    qint64 sessionSec = m_sessionTimer.elapsed() / 1000;
    m_totalSeconds   += sessionSec;

    // Sauvegarder le nouveau total
    writeStats();

    int h  =  sessionSec / 3600;
    int m  = (sessionSec % 3600) / 60;
    int s  =  sessionSec % 60;
    QString sessionStr = QString("%1h %2m %3s").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));

    log(INFO, QString("Session terminée — durée : %1").arg(sessionStr));

    m_stream << "╔══════════════════════════════════════════════════════╗\n";
    m_stream << QString("║  FERMETURE  %1  ║\n")
                    .arg(timestamp(), -40);
    m_stream << QString("║  Durée session          : %1  ║\n")
                    .arg(sessionStr, -26);
    m_stream << QString("║  Temps total cumulé     : %1  ║\n")
                    .arg(formattedTotalUsage(), -26);
    m_stream << "╚══════════════════════════════════════════════════════╝\n";
    m_stream.flush();

    m_logFile.close();
}

void Logger::logTileAction(const QString& label, const QString& command)
{
    log(ACTION, QString("Lancement tuile [%1] → %2").arg(label, command));
}

// ── Compteur permanent ────────────────────────────────────────────────────────
void Logger::readStats()
{
    QSettings stats(m_statsPath, QSettings::IniFormat);
    m_totalSeconds = stats.value("usage/total_seconds", 0).toLongLong();
}

void Logger::writeStats()
{
    QSettings stats(m_statsPath, QSettings::IniFormat);
    stats.setValue("usage/total_seconds",  m_totalSeconds);
    stats.setValue("usage/formatted",      formattedTotalUsage());
    stats.setValue("usage/last_updated",
                   QDateTime::currentDateTime().toString(Qt::ISODate));
    stats.sync();
}

qint64 Logger::totalUsageSeconds() const
{
    return m_totalSeconds;
}

QString Logger::formattedTotalUsage() const
{
    qint64 total = m_totalSeconds;
    int days  =  total / 86400;
    int hours = (total % 86400) / 3600;
    int mins  = (total % 3600)  / 60;
    int secs  =  total % 60;

    if (days > 0)
        return QString("%1j %2h %3m %4s").arg(days).arg(hours).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    if (hours > 0)
        return QString("%1h %2m %3s").arg(hours).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    return QString("%1m %2s").arg(mins).arg(secs, 2, 10, QChar('0'));
}
