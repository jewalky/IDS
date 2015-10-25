#include "filtersettingswindow.h"
#include "ui_filtersettingswindow.h"

#include <QListWidget>
#include <QListWidgetItem>

#include "serverlist.h"

static void addWidgetFlag(QListWidget* widget, QString name, quint32 flag)
{
    QListWidgetItem* item = new QListWidgetItem(widget);
    item->setText(name);
    item->setData(Qt::UserRole, flag);
    widget->addItem(item);
}

static quint32 getWidgetFlags(QListWidget* widget)
{
    quint32 flags = 0;
    for (int i = 0; i < widget->count(); i++)
    {
        QListWidgetItem* item = widget->item(i);
        quint32 item_flag = item->data(Qt::UserRole).toUInt();
        if (item->isSelected())
            flags |= item_flag;
    }

    return flags;
}

static bool checkWidgetFlag(QListWidget* widget, quint32 flag)
{
    return ((getWidgetFlags(widget) & flag) == flag);
}

static void setWidgetFlags(QListWidget* widget, quint32 flags)
{
    for (int i = 0; i < widget->count(); i++)
    {
        QListWidgetItem* item = widget->item(i);
        quint32 item_flag = item->data(Qt::UserRole).toUInt();
        item->setSelected(!!(item_flag & flags));
    }
}

FilterSettingsWindow::FilterSettingsWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FilterSettingsWindow)
{
    ui->setupUi(this);

    addWidgetFlag(ui->filterFlags, "Empty servers", ServerFilterPart::Flag_Empty);
    addWidgetFlag(ui->filterFlags, "Full servers", ServerFilterPart::Flag_Full);
    addWidgetFlag(ui->filterFlags, "N/A (banned)", ServerFilterPart::Flag_Banned);
    addWidgetFlag(ui->filterFlags, "N/A (flood protection hit)", ServerFilterPart::Flag_Flood);
    addWidgetFlag(ui->filterFlags, "N/A (timeout)", ServerFilterPart::Flag_NA);
    addWidgetFlag(ui->filterFlags, "Connect password", ServerFilterPart::Flag_ConnectPassword);
    addWidgetFlag(ui->filterFlags, "Join password", ServerFilterPart::Flag_JoinPassword);
    addWidgetFlag(ui->filterFlags, "Has bots", ServerFilterPart::Flag_HasBots);
    addWidgetFlag(ui->filterFlags, "Only bots", ServerFilterPart::Flag_OnlyBots);
    addWidgetFlag(ui->filterFlags, "No bots", ServerFilterPart::Flag_NoBots);
    addWidgetFlag(ui->filterFlags, "Not N/A", ServerFilterPart::Flag_NotNA);
    addWidgetFlag(ui->filterFlags, "Has spectators", ServerFilterPart::Flag_HasSpectators);
    addWidgetFlag(ui->filterFlags, "Only spectators", ServerFilterPart::Flag_OnlySpectators);
    addWidgetFlag(ui->filterFlags, "No spectators", ServerFilterPart::Flag_NoSpectators);

    static QString GameModeStrings[] =
    {
        "Cooperative",
        "Survival",
        "Invasion",
        "Deathmatch",
        "TeamDM",
        "Duel",
        "Terminator",
        "LMS",
        "TeamLMS",
        "Possession",
        "TeamPossession",
        "TeamGame",
        "CTF",
        "OneFlagCTF",
        "Skulltag",
        "Domination"
    };

    for (int i = 0; i < 16; i++)
        addWidgetFlag(ui->filterModes, GameModeStrings[i], 1<<i);

    checkboxLink(ui->serverNameCheck, ui->serverNameInput, true);
    checkboxLink(ui->serverAddressCheck, ui->serverAddressInput, true);
    checkboxLink(ui->serverIWADCheck, ui->serverIWADInput, true);
    checkboxLink(ui->dmflagsCheck, ui->dmflagsInput, false);
    checkboxLink(ui->dmflags2Check, ui->dmflags2Input, false);
    checkboxLink(ui->zadmflagsCheck, ui->zadmflagsInput, false);
    checkboxLink(ui->compatflagsCheck, ui->compatflagsInput, false);
    checkboxLink(ui->zacompatflagsCheck, ui->zacompatflagsInput, false);
}

FilterSettingsWindow::~FilterSettingsWindow()
{
    delete ui;
}

void FilterSettingsWindow::checkboxLink(QCheckBox* cb, QLineEdit* le, bool hasRegex)
{
    cb->setProperty("_oldTitle", cb->text());
    cb->setProperty("_hasRegex", hasRegex);
    cb->setProperty("_regex", false);
    cb->setProperty("_widget", (quint64)le);
    connect(cb, SIGNAL(clicked()), this, SLOT(onCheckboxChanged()));
    checkboxSet(cb, false, false);
}

void FilterSettingsWindow::checkboxSet(QCheckBox* cb, bool checked, bool regex)
{
    QString title = cb->property("_oldTitle").toString();

    if (cb->property("_hasRegex").toBool() && regex)
    {
        title += " (wildcards)";
        cb->setProperty("_regex", true);
    }
    else
    {
        cb->setProperty("_regex", false);
    }

    cb->setChecked(checked);
    cb->setText(title);

    QLineEdit* le = (QLineEdit*)(cb->property("_widget").toULongLong());
    le->setEnabled(checked);
}

bool FilterSettingsWindow::checkboxCheck(QCheckBox* cb)
{
    return cb->isChecked();
}

bool FilterSettingsWindow::checkboxCheckRegex(QCheckBox* cb)
{
    return (cb->property("_hasRegex").toBool() &&
            cb->property("_regex").toBool());
}

void FilterSettingsWindow::onCheckboxChanged()
{
    QCheckBox* cb = (QCheckBox*)sender();
    bool checked = !cb->isChecked();
    if (cb->property("_hasRegex").toBool())
    {
        if (!checked)
            checkboxSet(cb, true, false);
        else if (!checkboxCheckRegex(cb))
            checkboxSet(cb, true, true);
        else checkboxSet(cb, false, false);
    }
    else checkboxSet(cb, !checked, false);
}

void FilterSettingsWindow::on_pwadAddButton_clicked()
{
    QString pwadText = ui->pwadAddInput->text().trimmed();
    ui->pwadAddInput->setText("");

    if (pwadText.isEmpty())
        return;

    for (int i = 0; i < ui->pwadListInput->count(); i++)
    {
        QString exists = ui->pwadListInput->item(i)->text().trimmed().toLower();
        if (exists == pwadText.toLower())
            return;
    }

    ui->pwadListInput->insertItem(0, pwadText);
}

void FilterSettingsWindow::on_pwadDelButton_clicked()
{
    QList<QListWidgetItem*> items = ui->pwadListInput->selectedItems();
    if (items.size() <= 0)
        return;
    for (int i = 0; i < items.size(); i++)
        delete items[i];
}

void FilterSettingsWindow::setKind(bool filter)
{
    if (filter)
        ui->groupBox->setTitle("Filter");
    else ui->groupBox->setTitle("Exceptions");
}

bool FilterSettingsWindow::isFilterEnabled()
{
    return ui->groupBox->isChecked();
}

void FilterSettingsWindow::setFilterEnabled(bool enabled)
{
    ui->groupBox->setChecked(enabled);
}

void FilterSettingsWindow::setupFromPart(const ServerFilterPart& part)
{
    setWidgetFlags(ui->filterFlags, part.Flags & 0xFFFF);
    setWidgetFlags(ui->filterModes, part.GameModes);
    ui->filterModesCheck->setChecked(!!(part.Flags & ServerFilterPart::Flag_FilterGameModes));
    ui->filterModes->setEnabled(!!(part.Flags & ServerFilterPart::Flag_FilterGameModes));

    checkboxSet(ui->serverNameCheck, !!(part.Flags & ServerFilterPart::Flag_FilterServerName), !!(part.Flags & ServerFilterPart::Flag_FilterServerNameRegex));
    if (ui->serverNameCheck->isChecked())
        ui->serverNameInput->setText(part.ServerName);

    checkboxSet(ui->serverAddressCheck, !!(part.Flags & ServerFilterPart::Flag_FilterServerIP), !!(part.Flags & ServerFilterPart::Flag_FilterServerIPRegex));
    if (ui->serverAddressCheck->isChecked())
        ui->serverAddressInput->setText(part.ServerIP);

    checkboxSet(ui->serverIWADCheck, !!(part.Flags & ServerFilterPart::Flag_FilterIWAD), !!(part.Flags & ServerFilterPart::Flag_FilterIWADRegex));
    if (ui->serverIWADCheck->isChecked())
        ui->serverIWADInput->setText(part.IWAD);

    if (!!(part.Flags & ServerFilterPart::Flag_FilterPWADs))
    {
        ui->pwadListInput->setUpdatesEnabled(false);
        ui->pwadListInput->clear();
        for (int i = 0; i < part.PWADs.size(); i++)
            ui->pwadListInput->addItem(part.PWADs[i]);
        ui->pwadListInput->setUpdatesEnabled(true);

        ui->pwadWildcards->setChecked(!!(part.Flags & ServerFilterPart::Flag_FilterPWADsRegex));
    }
    else
    {
        ui->pwadWildcards->setChecked(false);
        ui->pwadListInput->clear();
    }

    ui->dmflagsCheck->setChecked(!!(part.Flags & ServerFilterPart::Flag_FilterDMFlags));
    if (ui->dmflagsCheck->isChecked())
        ui->dmflagsInput->setText(QString::number(part.DMFlags));

    ui->dmflags2Check->setChecked(!!(part.Flags & ServerFilterPart::Flag_FilterDMFlags2));
    if (ui->dmflags2Check->isChecked())
        ui->dmflags2Input->setText(QString::number(part.DMFlags2));

    ui->zadmflagsCheck->setChecked(!!(part.Flags & ServerFilterPart::Flag_FilterZADMFlags));
    if (ui->zadmflagsCheck->isChecked())
        ui->zadmflagsInput->setText(QString::number(part.ZADMFlags));

    ui->compatflagsCheck->setChecked(!!(part.Flags & ServerFilterPart::Flag_FilterCompatFlags));
    if (ui->compatflagsCheck->isChecked())
        ui->compatflagsInput->setText(QString::number(part.CompatFlags));

    ui->zacompatflagsCheck->setChecked(!!(part.Flags & ServerFilterPart::Flag_FilterZACompatFlags));
    if (ui->zacompatflagsCheck->isChecked())
        ui->zacompatflagsInput->setText(QString::number(part.ZACompatFlags));

}

ServerFilterPart FilterSettingsWindow::getPart()
{
    ServerFilterPart part;

    part.Flags = getWidgetFlags(ui->filterFlags);

    if (ui->filterModesCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterGameModes;
        part.GameModes = getWidgetFlags(ui->filterModes);
    }

    if (ui->serverNameCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterServerName;
        if (checkboxCheckRegex(ui->serverNameCheck))
            part.Flags |= ServerFilterPart::Flag_FilterServerNameRegex;
        part.ServerName = ui->serverNameInput->text();
    }

    if (ui->serverAddressCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterServerIP;
        if (checkboxCheckRegex(ui->serverAddressCheck))
            part.Flags |= ServerFilterPart::Flag_FilterServerIPRegex;
        part.ServerIP = ui->serverAddressInput->text();
    }

    if (ui->serverIWADCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterIWAD;
        if (checkboxCheckRegex(ui->serverIWADCheck))
            part.Flags |= ServerFilterPart::Flag_FilterIWADRegex;
        part.IWAD = ui->serverIWADInput->text();
    }

    if (ui->pwadListInput->count())
    {
        part.Flags |= ServerFilterPart::Flag_FilterPWADs;
        if (ui->pwadWildcards->isChecked())
            part.Flags |= ServerFilterPart::Flag_FilterPWADsRegex;
        for (int i = 0; i < ui->pwadListInput->count(); i++)
            part.PWADs.append(ui->pwadListInput->item(i)->text());
    }

    if (ui->dmflagsCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterDMFlags;
        part.DMFlags = ui->dmflagsInput->text().toUInt();
    }

    if (ui->dmflags2Check->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterDMFlags2;
        part.DMFlags2 = ui->dmflags2Input->text().toUInt();
    }

    if (ui->zadmflagsCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterZADMFlags;
        part.ZADMFlags = ui->zadmflagsInput->text().toUInt();
    }

    if (ui->compatflagsCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterCompatFlags;
        part.CompatFlags = ui->compatflagsInput->text().toUInt();
    }

    if (ui->zacompatflagsCheck->isChecked())
    {
        part.Flags |= ServerFilterPart::Flag_FilterZACompatFlags;
        part.ZACompatFlags = ui->zacompatflagsInput->text().toUInt();
    }

    return part;
}

void FilterSettingsWindow::on_filterModesCheck_toggled(bool checked)
{
    ui->filterModes->setEnabled(checked);
}
