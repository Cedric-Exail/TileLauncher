#include "Logger.h"
#include <QDateTime>
#include <QSize>

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
    m_data    = AppData::load(datPath);

    m_logFile.setFileName(logPath);
    // WriteOnly | Append | Text — sans BOM (QFile ne met pas de BOM par défaut)
    m_logFile.open(QIODevice::Append | QIODevice::Text);
    m_stream.setDevice(&m_logFile);
    // UTF-8 sans BOM pour compatibilité maximale avec les éditeurs Windows
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

// ── Écriture générique ────────────────────────────────────────────────────────
void Logger::log(Level level, const QString& message)
{
    if (!m_open) return;
    // ASCII-safe : éviter les caractères spéciaux dans le flux
    m_stream << QString("[%1] [%2] %3\n")
                    .arg(timestamp(), levelStr(level), message);
    m_stream.flush();
}

// ── Démarrage ─────────────────────────────────────────────────────────────────
void Logger::logLaunch()
{
    if (!m_open) return;

    m_data.launchCount++;
    m_data.lastLaunch = QDateTime::currentSecsSinceEpoch();
    m_sessionTimer.start();
    m_data.save(m_datPath);

    QString sep(54, '=');
    m_stream << "\n";
    m_stream << "+" << sep << "+\n";
    m_stream << QString("| STARTUP    %-42s|\n").arg(timestamp());
    m_stream << QString("| Launch #%-8lld  Total: %-25s|\n")
                    .arg(m_data.launchCount)
                    .arg(m_data.formattedUsage());
    if (m_data.hasPosition())
        m_stream << QString("| Last position: X=%-5d Y=%-5d W=%-5d H=%-5d   |\n")
                        .arg(m_data.windowX).arg(m_data.windowY)
                        .arg(m_data.windowW).arg(m_data.windowH);
    m_stream << "+" << sep << "+\n";
    m_stream.flush();

    log(INFO, "Application started");
}

// ── Fermeture ─────────────────────────────────────────────────────────────────
void Logger::logClose(const QPoint& windowPos, const QSize& windowSize)
{
    if (!m_open) return;

    // Duration of this session
    qint64 sessionSec = m_sessionTimer.elapsed() / 1000;
    m_data.totalSecs += sessionSec;
    m_data.setPosition(windowPos);
    m_data.setSize(windowSize);
    m_data.save(m_datPath);

    int h = static_cast<int>(sessionSec / 3600);
    int m = static_cast<int>((sessionSec % 3600) / 60);
    int s = static_cast<int>(sessionSec % 60);
    QString sessionStr = QString("%1h %2m %3s")
        .arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));

    log(INFO, QString("Session ended - duration: %1").arg(sessionStr));

    QString sep(54, '=');
    m_stream << "+" << sep << "+\n";
    m_stream << QString("| SHUTDOWN   %-42s|\n").arg(timestamp());
    m_stream << QString("| Session duration   : %-32s|\n").arg(sessionStr);
    m_stream << QString("| Total usage        : %-32s|\n").arg(m_data.formattedUsage());
    m_stream << QString("| Position saved     : X=%-5d Y=%-5d              |\n")
                    .arg(windowPos.x()).arg(windowPos.y());
    m_stream << QString("| Size saved         : W=%-5d H=%-5d              |\n")
                    .arg(windowSize.width()).arg(windowSize.height());
    m_stream << "+" << sep << "+\n";
    m_stream.flush();
    m_logFile.close();
}

// ── Action tuile ──────────────────────────────────────────────────────────────
void Logger::logTileAction(const QString& label, const QString& command)
{
    // Utiliser des caractères ASCII purs pour éviter la corruption
    log(ACTION, QString("Tile [%1] -> %2").arg(label, command));
}
