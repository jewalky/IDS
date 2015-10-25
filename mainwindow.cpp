#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serverlist.h"

#include <QCoreApplication>
#include <QScrollBar>
#include <QScreen>
#include <QSettings>

#include "settings.h"
#include "filterlist.h"

float MainWindow::DPIScale = 1.0;

static void SetToolButtonSize(QToolButton* tb, int size)
{
    tb->setFixedSize(size, size);
    tb->setIconSize(QSize(size, size));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->serverList->setScrollData(ui->serverScroller, ui->serverScroller->viewport());
    ui->serverScroller->setServerList(ui->serverList);

    // scale the toolbar on hidpi
    ui->serverToolbar->setFixedHeight(dpiScale(32)+8);
    SetToolButtonSize(ui->servers_update, dpiScale(32)+4);
    SetToolButtonSize(ui->servers_updateShown, dpiScale(32)+4);
    SetToolButtonSize(ui->servers_updateCurrent, dpiScale(32)+4);
    SetToolButtonSize(ui->servers_setupFilters, dpiScale(32)+4);

    svupdater = new ServerUpdater(0, ui->serverList);
    connect(svupdater, SIGNAL(started()), this, SLOT(sUpdateStarted()));
    connect(svupdater, SIGNAL(succeeded()), this, SLOT(sUpdateSucceeded()));
    connect(svupdater, SIGNAL(masterSucceeded()), this, SLOT(sUpdateSucceededMaster()));
    connect(svupdater, SIGNAL(failed(int)), this, SLOT(sUpdateFailed(int)));

    connect(ui->serverList, SIGNAL(updateRequested(quint32,quint16)), this, SLOT(sUpdateRequested(quint32,quint16)));

    initOptionsTab();

    setMinimumWidth(1024);
    setMinimumHeight(480);
}

float MainWindow::dpiScale(float value)
{
    //int dpi = ((QGuiApplication*)QCoreApplication::instance())->primaryScreen()->physicalDotsPerInch();
    int dpi = ((QGuiApplication*)QCoreApplication::instance())->primaryScreen()->logicalDotsPerInch();
    DPIScale = float(dpi) / 96.0;
    return value*DPIScale;
}

MainWindow::~MainWindow()
{
    delete svupdater;
    delete ui;
}

void MainWindow::on_servers_update_clicked()
{
    if (!svupdater->scheduleUpdateAll("master.zandronum.com:15300"))
        qDebug("Failed to schedule update with the default master address.");
}

void MainWindow::on_servers_updateShown_clicked()
{
    svupdater->scheduleUpdateVisible();
}

void MainWindow::on_servers_updateCurrent_clicked()
{
    Server* srv = ui->serverList->getSelectedServer();
    if (!srv) return;
    sUpdateRequested(srv->IP, srv->Port);
}

void MainWindow::on_servers_setupFilters_clicked()
{
    FilterList* list = new FilterList(this);
    list->setFilterList(ui->serverList->getFilterList());
    list->setModal(true);
    list->show();
    connect(list, SIGNAL(closedSave()), this, SLOT(sFiltersOkay()));
}

void MainWindow::sFiltersOkay()
{
    FilterList* list = qobject_cast<FilterList*>(sender());
    if (!list) return;
    ui->serverList->setFilterList(list->getFilterList());
    list->deleteLater();
}

void MainWindow::sUpdateSucceeded()
{
    ui->servers_update->setEnabled(true);
    ui->servers_updateShown->setEnabled(true);
    ui->servers_updateCurrent->setEnabled(true);
    ui->serverList->setFlag(ServerList::Flag_UpdateOften, false);

    QString ssFL = QString("(Servers total: %1").arg(QString::number(ui->serverList->getServerCount(-1)));
    QVector<ServerFilter> filters = ui->serverList->getFilterList();
    for (int i = 1; i < filters.size(); i++)
    {
        if (!filters[i].isEnabled())
            break;
        ssFL += ", ";
        ssFL += QString("%1: %2").arg(filters[i].getName(), QString::number(ui->serverList->getServerCount(i)));
    }

    int sv_players = 0;
    QVector<Server>& servers = ui->serverList->getServers();
    for (int i = 0; i < servers.size(); i++)
    {
        sv_players += servers[i].PlayerCount;
    }

    ssFL += QString("; Players: %1)").arg(QString::number(sv_players));

    setWindowTitle("Zandronum Launcher  " + ssFL);
}

void MainWindow::sUpdateSucceededMaster()
{

}

void MainWindow::sUpdateFailed(int reason)
{
    ui->servers_update->setEnabled(true);
    ui->servers_updateShown->setEnabled(true);
    ui->servers_updateCurrent->setEnabled(true);
    ui->serverList->setFlag(ServerList::Flag_UpdateOften, false);
    if (reason == ServerUpdater::Failed_Timeout)
        sUpdateSucceeded();
}

void MainWindow::sUpdateStarted()
{
    ui->servers_update->setEnabled(false);
    ui->servers_updateShown->setEnabled(false);
    ui->servers_updateCurrent->setEnabled(false);
    ui->serverList->setFlag(ServerList::Flag_UpdateOften, true);
}

void MainWindow::sUpdateRequested(quint32 ip, quint16 port)
{
    if (!svupdater->isActive())
        svupdater->scheduleUpdateServer(ip, port);
}

void MainWindow::initOptionsTab()
{
    QSettings* s = Settings::get();

    ui->optionsAppearance_pingBad->setValue(s->value("appearance.pingAcceptable", 200).toInt());
    ui->optionsAppearance_pingGood->setValue(s->value("appearance.pingGood", 100).toInt());

    ui->optionsAppearance_smoothPing->setChecked(s->value("appearance.smoothPing", true).toBool());
    ui->serverList->setFlag(ServerList::Flag_SmoothPing, ui->optionsAppearance_smoothPing->isChecked());
    ui->optionsAppearance_classicView->setChecked(s->value("appearance.classicView", false).toBool());
    ui->serverList->setFlag(ServerList::Flag_ClassicView, ui->optionsAppearance_classicView->isChecked());
    ui->optionsAppearance_noHover->setChecked(s->value("appearance.noHover", false).toBool());
    ui->serverList->setFlag(ServerList::Flag_NoHighlight, ui->optionsAppearance_noHover->isChecked());
    ui->optionsAppearance_animateRefresh->setChecked(s->value("appearance.animateRefresh", false).toBool());
    ui->serverList->setFlag(ServerList::Flag_AnimateRefresh, ui->optionsAppearance_animateRefresh->isChecked());


    ui->optionsAppearance_playersTop->setChecked(s->value("appearance.cvPlayersTop", true).toBool());
    ui->serverList->setFlag(ServerList::Flag_CVPlayersTop, ui->optionsAppearance_playersTop->isChecked());
    ui->optionsAppearance_playersSorting->setChecked(s->value("appearance.cvPlayersSorting", false).toBool());
    ui->serverList->setFlag(ServerList::Flag_CVSortPlayers, ui->optionsAppearance_playersSorting->isChecked());
    ui->optionsAppearance_noGrid->setChecked(s->value("appearance.cvNoGrid", false).toBool());
    ui->serverList->setFlag(ServerList::Flag_CVNoGrid, ui->optionsAppearance_noGrid->isChecked());
    ui->optionsAppearance_fullGrid->setChecked(s->value("appearance.cvFullGrid", true).toBool());
    ui->serverList->setFlag(ServerList::Flag_CVGrid, ui->optionsAppearance_fullGrid->isChecked());
}


void MainWindow::on_optionsAppearance_pingBad_valueChanged(int arg1)
{
    ui->optionsAppearance_pingGood->setMaximum(arg1-1);
    Settings::get()->setValue("appearance.pingAcceptable", arg1);
}


void MainWindow::on_optionsAppearance_pingGood_valueChanged(int arg1)
{
    ui->optionsAppearance_pingBad->setMinimum(arg1+1);
    Settings::get()->setValue("appearance.pingGood", arg1);
}


void MainWindow::on_optionsAppearance_classicView_toggled(bool checked)
{
    Settings::get()->setValue("appearance.classicView", checked);
    ui->serverList->setFlag(ServerList::Flag_ClassicView, checked);
}

void MainWindow::on_optionsAppearance_noHover_toggled(bool checked)
{
    Settings::get()->setValue("appearance.noHover", checked);
    ui->serverList->setFlag(ServerList::Flag_NoHighlight, checked);
}

void MainWindow::on_optionsAppearance_playersTop_toggled(bool checked)
{
    Settings::get()->setValue("appearance.cvPlayersTop", checked);
    ui->serverList->setFlag(ServerList::Flag_CVPlayersTop, checked);
}


void MainWindow::on_optionsAppearance_playersSorting_toggled(bool checked)
{
    Settings::get()->setValue("appearance.cvPlayersSorting", checked);
    ui->serverList->setFlag(ServerList::Flag_CVSortPlayers, checked);
}


void MainWindow::on_optionsAppearance_noGrid_toggled(bool checked)
{
    Settings::get()->setValue("appearance.cvNoGrid", checked);
    ui->serverList->setFlag(ServerList::Flag_CVNoGrid, checked);
    if (checked) ui->optionsAppearance_fullGrid->setChecked(false);
}


void MainWindow::on_optionsAppearance_fullGrid_toggled(bool checked)
{
    Settings::get()->setValue("appearance.cvFullGrid", checked);
    ui->serverList->setFlag(ServerList::Flag_CVGrid, checked);
    if (checked) ui->optionsAppearance_noGrid->setChecked(false);
}

void MainWindow::on_optionsAppearance_animateRefresh_toggled(bool checked)
{
    Settings::get()->setValue("appearance.animateRefresh", checked);
    ui->serverList->setFlag(ServerList::Flag_AnimateRefresh, checked);
}

void MainWindow::on_optionsAppearance_smoothPing_toggled(bool checked)
{
    Settings::get()->setValue("appearance.smoothPing", checked);
    ui->serverList->setFlag(ServerList::Flag_SmoothPing, checked);
}

