#include <QApplication>
#include "TileLauncher.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setQuitOnLastWindowClosed(true);

    TileLauncher window;
    window.show();

    return app.exec();
}
