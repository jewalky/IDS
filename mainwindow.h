#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "serverlist.h"
#include "serverupdater.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static float dpiScale(float value);

private slots:
    void on_servers_update_clicked();
    void on_servers_updateShown_clicked();
    void on_servers_updateCurrent_clicked();
    void on_servers_setupFilters_clicked();
    void sFiltersOkay();

    void sUpdateSucceeded();
    void sUpdateSucceededMaster();
    void sUpdateFailed(int reason);
    void sUpdateStarted();
    void sUpdateRequested(quint32 ip, quint16 port);

    void on_optionsAppearance_pingBad_valueChanged(int arg1);
    void on_optionsAppearance_pingGood_valueChanged(int arg1);
    void on_optionsAppearance_classicView_toggled(bool checked);
    void on_optionsAppearance_noHover_toggled(bool checked);
    void on_optionsAppearance_playersTop_toggled(bool checked);
    void on_optionsAppearance_playersSorting_toggled(bool checked);
    void on_optionsAppearance_noGrid_toggled(bool checked);
    void on_optionsAppearance_fullGrid_toggled(bool checked);
    void on_optionsAppearance_animateRefresh_toggled(bool checked);
    void on_optionsAppearance_smoothPing_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    ServerUpdater* svupdater;

    static float DPIScale;

    void initOptionsTab();
};

#endif // MAINWINDOW_H
