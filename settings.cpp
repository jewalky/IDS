#include "settings.h"
#include <QSettings>

bool Settings::Portable = true;
QSettings Settings::NativeSettings;
bool Settings::FirstQuery = true;

void Settings::setPortable(bool portable)
{
    if ((Portable != portable) || FirstQuery)
    {
        Portable = portable;

        // we're returning the pointer later.
        // the way it works right now, the NativeSettings object can randomly change at any time, but the pointer won't invalidate and will still work.
        if (Portable)
        {
            NativeSettings.QSettings::~QSettings();
            new (&NativeSettings) QSettings("ZanLauncher.ini", QSettings::IniFormat);
        }
        else
        {
            NativeSettings.QSettings::~QSettings();
            new (&NativeSettings) QSettings("AlienSoft", "ZanLauncher");
        }
    }
}

bool Settings::isPortable()
{
    return Portable;
}

QSettings* Settings::get()
{
    if (FirstQuery)
    {
        setPortable(Portable);
        FirstQuery = false;
    }

    return &NativeSettings;
}
