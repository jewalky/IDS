#ifndef FILTERSETTINGSMASTER_H
#define FILTERSETTINGSMASTER_H

#include <QDialog>

#include "serverlist.h"

namespace Ui {
class FilterSettingsMaster;
}

class FilterSettingsMaster : public QDialog
{
    Q_OBJECT

public:
    explicit FilterSettingsMaster(QWidget *parent = 0);
    ~FilterSettingsMaster();

    void setupFromFilter(const ServerFilter& filter);
    ServerFilter getFilter();

signals:
    void closedSave();
    void closedCancel();

private slots:
    void on_saveButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::FilterSettingsMaster *ui;
    QString Name;
};

#endif // FILTERSETTINGSMASTER_H
