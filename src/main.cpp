#include <QApplication>
#include <QTimer>
#include <QScreen>
#include <QPixmap>
#include <QCommandLineParser>
#include "TileLauncher.h"

int main(int argc, char* argv[])
{
    // Forcer le rendu logiciel pour headless (CI, runners sans GPU)
    qputenv("QT_OPENGL", "software");

    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setQuitOnLastWindowClosed(true);

    // ── Parser arguments ──────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption screenshotOpt(
        "screenshot",
        "Prendre un screenshot apres N ms et quitter (mode CI)",
        "fichier"
    );
    QCommandLineOption delayOpt(
        "delay",
        "Delai avant screenshot en ms (defaut: 1000)",
        "ms", "1000"
    );
    parser.addOption(screenshotOpt);
    parser.addOption(delayOpt);
    parser.process(app);

    TileLauncher window;
    window.show();

    // ── Mode screenshot (CI headless) ─────────────────────────────────────
    if (parser.isSet(screenshotOpt)) {
        QString outFile = parser.value(screenshotOpt);
        int     delay   = parser.value(delayOpt).toInt();

        QTimer::singleShot(delay, [&]() {
            // Forcer le rendu complet
            app.processEvents();
            window.repaint();
            app.processEvents();

            // Capture via Qt directement (rendu logiciel garanti)
            QPixmap pix = window.grab();
            if (pix.save(outFile, "PNG")) {
                qInfo("Screenshot sauvegarde : %s (%dx%d)",
                      qPrintable(outFile), pix.width(), pix.height());
            } else {
                qWarning("Echec sauvegarde screenshot : %s", qPrintable(outFile));
            }
            app.quit();
        });
    }

    return app.exec();
}
