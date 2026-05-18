#include "Logger.h"
#include <QDateTime>
#include <QSize>

Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

void Logger::open(const QString& logPath, const QString& datPath)
{
    m_datPath = datPath;
    m_data    = AppData::load(datPath);
    m_logFile.setFileName(logPath);
    m_logFile.open(QIODevice::Append | QIODevice::WriteOnly);
    m_open = true;
}

void Logger::writeLine(const QString& line)
{
    if (!m_open) return;
    QByteArray bytes = (line + "\r\n").toUtf8();
    m_logFile.write(bytes);
    m_logFile.flush();
}

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

void Logger::log(Level level, const QString& message)
{
    if (!m_open) return;
    writeLine(QString("[%1] [%2] %3")
        .arg(timestamp(), levelStr(level), message));
}

void Logger::logLaunch()
{
    if (!m_open) return;

    m_data.launchCount++;
    m_data.lastLaunch = QDateTime::currentSecsSinceEpoch();
    m_sessionTimer.start();
    m_data.save(m_datPath);

    // Utiliser QString::arg(val, width) au lieu de sprintf %-Ns
    QString ts   = timestamp();
    QString tot  = m_data.formattedUsage();
    QString sep(58, '=');

    writeLine("");
    writeLine("+" + sep + "+");
    writeLine("| STARTUP    " + ts.leftJustified(44) + "|");
    writeLine("| Launch #"
        + QString::number(m_data.launchCount).leftJustified(8)
        + "  Total: "
        + tot.leftJustified(27) + "|");
    if (m_data.hasPosition()) {
        writeLine("| Last pos: X="
            + QString::number(m_data.windowX).leftJustified(6)
            + " Y=" + QString::number(m_data.windowY).leftJustified(6)
            + " W=" + QString::number(m_data.windowW).leftJustified(6)
            + " H=" + QString::number(m_data.windowH).leftJustified(4)
            + "|");
    }
    writeLine("+" + sep + "+");
    log(INFO, "Application started");
}

void Logger::logClose(const QPoint& windowPos, const QSize& windowSize)
{
    if (!m_open) return;

    qint64 sessionSec = m_sessionTimer.elapsed() / 1000;
    m_data.totalSecs += sessionSec;
    m_data.setPosition(windowPos);
    m_data.setSize(windowSize);
    m_data.save(m_datPath);

    int h = static_cast<int>(sessionSec / 3600);
    int m = static_cast<int>((sessionSec % 3600) / 60);
    int s = static_cast<int>(sessionSec % 60);
    QString dur = QString("%1h %2m %3s")
        .arg(h)
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));

    log(INFO, "Session ended - duration: " + dur);

    QString sep(58, '=');
    writeLine("+" + sep + "+");
    writeLine("| SHUTDOWN   " + timestamp().leftJustified(44) + "|");
    writeLine("| Session : " + dur.leftJustified(47) + "|");
    writeLine("| Total   : " + m_data.formattedUsage().leftJustified(47) + "|");
    writeLine("| Pos     : X="
        + QString::number(windowPos.x()).leftJustified(6)
        + " Y=" + QString::number(windowPos.y()).leftJustified(6)
        + " W=" + QString::number(windowSize.width()).leftJustified(6)
        + " H=" + QString::number(windowSize.height()).leftJustified(4)
        + "|");
    writeLine("+" + sep + "+");
    m_logFile.close();
}

void Logger::logTileAction(const QString& label, const QString& command)
{
    log(ACTION, "Tile [" + label + "] -> " + command);
}
