#include <QApplication>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QCommandLineParser>
#include "TileLauncher.h"

int main(int argc, char* argv[])
{
    // Software rendering for headless / CI runners without GPU
    qputenv("QT_OPENGL", "software");

    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setQuitOnLastWindowClosed(true);

    // Qt plugins are in the same directory as the exe (_internal/)
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());

    // ── CLI parser ────────────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption screenshotOpt(
        "screenshot",
        "Take a screenshot after N ms and quit (CI mode)",
        "file"
    );
    QCommandLineOption delayOpt(
        "delay",
        "Delay before screenshot in ms (default: 1000)",
        "ms", "1000"
    );
    parser.addOption(screenshotOpt);
    parser.addOption(delayOpt);
    parser.process(app);

    TileLauncher window;
    window.show();

    // ── CI screenshot mode ────────────────────────────────────────────────
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
