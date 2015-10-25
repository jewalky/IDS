#include "mainwindow.h"
#include <QCoreApplication>
#include <QApplication>
#include <QStyleFactory>

#include "settings.h"

int main(int argc, char *argv[])
{
    //Settings::setPortable(false);
    Settings::setPortable(true);

    for (int i = 1; i < argc; i++)
    {
        QString arg = QString::fromUtf8(QByteArray(argv[i]));
        if (arg == "--portable")
            Settings::setPortable(true);
    }

    QApplication::addLibraryPath("./plugins/");
    QApplication a(argc, argv);
    //a.setStyle(QStyleFactory::create("windows"));
    //a.setStyle(QStyleFactory::create("Fusion"));

    QFont fnt = a.font();
    //fnt.setPixelSize(24);
    a.setFont(fnt);

    MainWindow w;
    w.show();

    return a.exec();
}
