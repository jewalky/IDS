#ifndef FILTERLIST_H
#define FILTERLIST_H

#include <QDialog>
#include <QList>
#include <QListWidgetItem>
#include "serverlist.h"

namespace Ui {
class FilterList;
}

class FilterList : public QDialog
{
    Q_OBJECT

public:
    explicit FilterList(QWidget *parent = 0);
    ~FilterList();

    QVector<ServerFilter> getFilterList();
    void setFilterList(const QVector<ServerFilter>& list);

private slots:
    void on_button_create_clicked();
    void on_button_delete_clicked();
    void on_button_moveUp_clicked();
    void on_button_moveDown_clicked();
    void on_button_ok_clicked();
    void on_button_cancel_clicked();
    void on_button_edit_clicked();

    void sFilterSaved();

    void on_filterList_doubleClicked(const QModelIndex &index);

signals:
    void closedSave();
    void closedCancel();

private:
    Ui::FilterList *ui;

    void refreshList();
};

#endif // FILTERLIST_H
