#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QVariant>
#include <QMap>
#include <QSettings>

class Settings
{
private:
    Settings() {}

public:
    // portable means the settings are saved directly into a ZanLauncher.cfg
    // instead of putting them into an abstract Qt storage.
    static void setPortable(bool portable);
    static bool isPortable();

    // get/set settings
    static QSettings* get();

    // this causes instant write to the disk/qsettings (and sync() on qsettings in case of non-portable setup)
    static void sync();

private:
    static bool Portable;
    static QSettings NativeSettings;
    static bool FirstQuery;
};

#endif // SETTINGS_H
