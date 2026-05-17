#include "Logger.h"
#include <QDateTime>

// ── Singleton ─────────────────────────────────────────────────────────────────
Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

// ── Ouverture ─────────────────────────────────────────────────────────────────
void Logger::open(const QString& logPath, const QString& datPath)
{
    m_datPath = datPath;

    // Charger le fichier .dat binaire (ou créer par défaut si absent/corrompu)
    m_data = AppData::load(datPath);

    // Ouvrir le log texte en mode append
    m_logFile.setFileName(logPath);
    m_logFile.open(QIODevice::Append | QIODevice::Text);
    m_stream.setDevice(&m_logFile);
    m_stream.setEncoding(QStringConverter::Utf8);

    m_open = true;
}

// ── Helpers ───────────────────────────────────────────────────────────────────
QString Logger::timestamp() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
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

// ── Écriture ──────────────────────────────────────────────────────────────────
void Logger::log(Level level, const QString& message)
{
    if (!m_open) return;
    m_stream << QString("[%1] [%2] %3\n")
                    .arg(timestamp())
                    .arg(levelStr(level))
                    .arg(message);
    m_stream.flush();
}

// ── Démarrage ─────────────────────────────────────────────────────────────────
void Logger::logLaunch()
{
    if (!m_open) return;

    // Incrémenter le compteur de lancements et horodatage
    m_data.launchCount++;
    m_data.lastLaunch = QDateTime::currentSecsSinceEpoch();

    // Démarrer le chrono de session
    m_sessionTimer.start();

    // Sauvegarder immédiatement (même si crash, on a le lancement enregistré)
    m_data.save(m_datPath);

    m_stream << "\n";
    m_stream << "╔══════════════════════════════════════════════════════╗\n";
    m_stream << QString("║  DÉMARRAGE  %1  ║\n").arg(timestamp(), -40);
    m_stream << QString("║  Lancement n°%-6lld  Temps total : %-18s║\n")
                    .arg(m_data.launchCount)
                    .arg(m_data.formattedUsage());
    if (m_data.hasPosition())
        m_stream << QString("║  Dernière position : X=%-5d Y=%-5d                ║\n")
                        .arg(m_data.windowX).arg(m_data.windowY);
    m_stream << "╚══════════════════════════════════════════════════════╝\n";
    m_stream.flush();

    log(INFO, "Application démarrée");
}

// ── Fermeture ─────────────────────────────────────────────────────────────────
void Logger::logClose(const QPoint& windowPos)
{
    if (!m_open) return;

    // Durée de cette session
    qint64 sessionSec = m_sessionTimer.elapsed() / 1000;
    m_data.totalSecs += sessionSec;

    // Sauvegarder la position finale de la fenêtre
    m_data.setPosition(windowPos);

    // Persister le .dat
    m_data.save(m_datPath);

    int h =  sessionSec / 3600;
    int m = (sessionSec % 3600) / 60;
    int s =  sessionSec % 60;
    QString sessionStr = QString("%1h %2m %3s")
        .arg(h)
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));

    log(INFO, QString("Session terminée — durée : %1").arg(sessionStr));

    m_stream << "╔══════════════════════════════════════════════════════╗\n";
    m_stream << QString("║  FERMETURE  %1  ║\n").arg(timestamp(), -40);
    m_stream << QString("║  Durée session      : %-30s║\n").arg(sessionStr);
    m_stream << QString("║  Temps total cumulé : %-30s║\n").arg(m_data.formattedUsage());
    m_stream << QString("║  Position sauvée    : X=%-5d Y=%-20d║\n")
                    .arg(windowPos.x()).arg(windowPos.y());
    m_stream << "╚══════════════════════════════════════════════════════╝\n";
    m_stream.flush();

    m_logFile.close();
}

// ── Action tuile ──────────────────────────────────────────────────────────────
void Logger::logTileAction(const QString& label, const QString& command)
{
    log(ACTION, QString("Lancement tuile [%1] → %2").arg(label, command));
}
