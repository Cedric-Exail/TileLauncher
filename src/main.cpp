#include <QApplication>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QCommandLineParser>
#include "TileLauncher.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

// ── Ajoute _internal/ au chemin de recherche des DLL (Windows) ───────────────
// Permet de garder l'exe seul a la racine et toutes les DLL dans _internal/
static void setupInternalPath()
{
#ifdef Q_OS_WIN
    // Construire le chemin absolu vers _internal/ a cote de l'exe
    QString exeDir  = QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath());
    QString internalDir = exeDir + "\\_internal";

    // AddDllDirectory : ajoute le dossier au chemin de recherche DLL
    // sans modifier PATH (plus propre et securise)
    if (QDir(internalDir).exists()) {
        SetDllDirectoryW(reinterpret_cast<LPCWSTR>(internalDir.utf16()));

        // Ajouter aussi les sous-dossiers plugins Qt
        for (const QString& sub : {QString("platforms"), QString("styles"),
                                    QString("imageformats")}) {
            QString plugPath = internalDir + "\\" + sub;
            AddDllDirectory(reinterpret_cast<PCWSTR>(plugPath.utf16()));
        }

        // Indiquer a Qt ou trouver ses plugins
        QCoreApplication::addLibraryPath(internalDir);
        QCoreApplication::addLibraryPath(exeDir);
    }
#endif
}

int main(int argc, char* argv[])
{
    // Forcer rendu logiciel pour headless / runners CI sans GPU
    qputenv("QT_OPENGL", "software");

    // Configurer le chemin _internal AVANT QApplication
    // (QApplication charge deja des DLL Qt a sa construction)
    // Note : QCoreApplication::applicationDirPath() necessite argc/argv
    // On utilise GetModuleFileNameW directement pour l'appel pre-QApplication
#ifdef Q_OS_WIN
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        QString dir = QString::fromWCharArray(exePath);
        dir = dir.left(dir.lastIndexOf('\\'));
        QString internal = dir + "\\_internal";
        if (QDir(internal).exists()) {
            SetDllDirectoryW(reinterpret_cast<LPCWSTR>(internal.utf16()));
        }
    }
#endif

    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setQuitOnLastWindowClosed(true);

    // Configurer les chemins apres QApplication (pour les plugins)
    setupInternalPath();

    // ── Parser CLI ────────────────────────────────────────────────────────
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

    // ── Mode screenshot CI ────────────────────────────────────────────────
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
