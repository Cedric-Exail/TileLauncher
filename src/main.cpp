#include <QApplication>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QLocalServer>
#include <QLocalSocket>
#include "TileLauncher.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// ── Nom unique de l application (mutex + socket locale) ───────────────────────
static const QString APP_KEY = "TileLauncher_SingleInstance_2025";

// ── Tentative de connexion a une instance existante ───────────────────────────
static bool notifyExistingInstance()
{
    QLocalSocket socket;
    socket.connectToServer(APP_KEY);
    if (socket.waitForConnected(500)) {
        // Envoyer le signal "bring to front"
        socket.write("RAISE");
        socket.flush();
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return true;   // instance existante trouvee et notifiee
    }
    return false;      // pas d instance existante
}

int main(int argc, char* argv[])
{
    // ── Priorité maximale au démarrage ────────────────────────────────────────
#ifdef Q_OS_WIN
    // Priorité du processus : HIGH_PRIORITY_CLASS
    // (REALTIME_PRIORITY_CLASS serait trop agressif et bloquerait le système)
    ::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    // Priorité du thread principal : THREAD_PRIORITY_HIGHEST
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif

    qputenv("QT_OPENGL", "software");

    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setQuitOnLastWindowClosed(true);
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());

    // ── Detection instance unique ─────────────────────────────────────────────
    if (notifyExistingInstance()) {
        // Une instance tourne deja : elle va se mettre au premier plan
        return 0;
    }

    // ── Serveur local : ecoute les demandes "RAISE" des instances suivantes ───
    QLocalServer server;
    QLocalServer::removeServer(APP_KEY);   // nettoyer si crash precedent
    server.listen(APP_KEY);

    // ── CLI parser ────────────────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption screenshotOpt("screenshot",
        "Take a screenshot after N ms and quit (CI mode)", "file");
    QCommandLineOption delayOpt("delay",
        "Delay before screenshot in ms (default: 1000)", "ms", "1000");
    parser.addOption(screenshotOpt);
    parser.addOption(delayOpt);
    parser.process(app);

    TileLauncher window;
    window.show();

    // ── Revenir en priorité normale après affichage ───────────────────────────
    // Évite de monopoliser le CPU pendant l'utilisation normale
#ifdef Q_OS_WIN
    ::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif

    // ── Reaction aux connexions entrantes (autre instance lancee) ─────────────
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket* client = server.nextPendingConnection();
        QObject::connect(client, &QLocalSocket::readyRead, [&window, client]() {
            QByteArray data = client->readAll();
            if (data.contains("RAISE")) {
                // Remettre la fenetre au premier plan
                window.setWindowState(
                    window.windowState() & ~Qt::WindowMinimized);
                window.show();
                window.raise();
                window.activateWindow();
#ifdef Q_OS_WIN
                // Forcer le focus via WinAPI (Qt seul ne suffit pas toujours)
                HWND hwnd = reinterpret_cast<HWND>(window.winId());
                ::SetForegroundWindow(hwnd);
                ::BringWindowToTop(hwnd);
#endif
            }
            client->deleteLater();
        });
        // Lire les donnees si deja disponibles
        if (client->bytesAvailable() > 0)
            client->readyRead();
    });

    // ── Mode screenshot CI ────────────────────────────────────────────────────
    if (parser.isSet(screenshotOpt)) {
        QString outFile = parser.value(screenshotOpt);
        int     delay   = parser.value(delayOpt).toInt();
        QTimer::singleShot(delay, [&]() {
            app.processEvents();
            window.repaint();
            app.processEvents();
            QPixmap pix = window.grab();
            if (pix.save(outFile, "PNG"))
                qInfo("Screenshot saved: %s (%dx%d)",
                      qPrintable(outFile), pix.width(), pix.height());
            else
                qWarning("Failed to save screenshot: %s", qPrintable(outFile));
            app.quit();
        });
    }

    return app.exec();
}
