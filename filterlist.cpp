#include "filterlist.h"
#include "ui_filterlist.h"

#include "filtersettingsmaster.h"

FilterList::FilterList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterList)
{
    ui->setupUi(this);
}

FilterList::~FilterList()
{
    delete ui;
}

QVector<ServerFilter> FilterList::getFilterList()
{
    QVector<ServerFilter> list;
    for (int i = 0; i < ui->filterList->count(); i++)
    {
        ServerFilter filter = ui->filterList->item(i)->data(Qt::UserRole).value<ServerFilter>();
        list.append(filter);
    }

    return list;
}

void FilterList::setFilterList(const QVector<ServerFilter>& list)
{
    ui->filterList->clear();
    ui->filterList->clearSelection();
    for (int i = 0; i < list.size(); i++)
    {
        const ServerFilter& filter = list[i];
        QListWidgetItem* item = new QListWidgetItem(filter.getName());
        item->setData(Qt::UserRole, QVariant::fromValue(filter));
        ui->filterList->addItem(item);
    }

    refreshList();
}

void FilterList::on_button_create_clicked()
{
    ServerFilter filter;
    filter.setName("New filter");

    for (int i = 1; i < 65536; i++)
    {
        bool exists = false;
        for (int j = 0; j < ui->filterList->count(); j++)
        {
            QListWidgetItem* item = ui->filterList->item(j);
            const ServerFilter& filter2 = item->data(Qt::UserRole).value<ServerFilter>();
            if (filter2.getName().toLower() == filter.getName().toLower())
            {
                exists = true;
                break;
            }
        }

        if (exists)
        {
            filter.setName(QString("New filter #%1").arg(QString::number(i)));
            continue;
        }

        break;
    }

    QListWidgetItem* item = new QListWidgetItem(filter.getName());
    item->setData(Qt::UserRole, QVariant::fromValue(filter));
    ui->filterList->addItem(item);
}

void FilterList::on_button_delete_clicked()
{
    QList<QListWidgetItem*> items = ui->filterList->selectedItems();

    bool deleted0 = false;
    for (int i = 0; i < items.size(); i++)
    {
        if (ui->filterList->row(items[i]) == 0)
            deleted0 = true;
        items[i]->setSelected(false);
        delete items[i];
    }

    if (deleted0)
    {
        ServerFilter filter;
        filter.setName("Default global filter");

        for (int i = 1; i < 65536; i++)
        {
            bool exists = false;
            for (int j = 0; j < ui->filterList->count(); j++)
            {
                QListWidgetItem* item = ui->filterList->item(j);
                const ServerFilter& filter2 = item->data(Qt::UserRole).value<ServerFilter>();
                if (filter2.getName().toLower() == filter.getName().toLower())
                {
                    exists = true;
                    break;
                }
            }

            if (exists)
            {
                filter.setName(QString("Default global filter #%1").arg(QString::number(i)));
                continue;
            }

            break;
        }

        QListWidgetItem* item = new QListWidgetItem(filter.getName());
        item->setData(Qt::UserRole, QVariant::fromValue(filter));
        ui->filterList->insertItem(0, item);
        item->setSelected(true);
    }

    refreshList();
}

void FilterList::on_button_moveUp_clicked()
{
    QList<QListWidgetItem*> items = ui->filterList->selectedItems();
    int topRow = 0;

    for (int i = 0; i < items.size(); i++)
    {
        int itemRow = ui->filterList->row(items[i]);
        if (itemRow > topRow)
        {
            ui->filterList->insertItem(itemRow-1, ui->filterList->takeItem(itemRow));
            itemRow--;
            topRow = itemRow+1;
        }
        else topRow++;
    }

    for (int i = 0; i < ui->filterList->count(); i++)
    {
        QListWidgetItem* item = ui->filterList->item(i);
        item->setSelected(items.contains(item));
    }

    refreshList();
}

void FilterList::on_button_moveDown_clicked()
{
    QList<QListWidgetItem*> items = ui->filterList->selectedItems();
    int bottomRow = ui->filterList->count()-1;

    for (int i = 0 ; i < items.size(); i++)
    {
        int itemRow = ui->filterList->row(items[i]);
        if (itemRow < bottomRow)
        {
            ui->filterList->insertItem(itemRow+1, ui->filterList->takeItem(itemRow));
            itemRow++;
            bottomRow = itemRow-1;
        }
        else bottomRow--;
    }

    for (int i = 0; i < ui->filterList->count(); i++)
    {
        QListWidgetItem* item = ui->filterList->item(i);
        item->setSelected(items.contains(item));
    }

    refreshList();
}

void FilterList::on_button_ok_clicked()
{
    emit closedSave();
    close();
}


void FilterList::on_button_cancel_clicked()
{
    emit closedCancel();
    close();
}

void FilterList::on_button_edit_clicked()
{
    QList<QListWidgetItem*> items = ui->filterList->selectedItems();
    if (!items.size()) return;

    FilterSettingsMaster* master = new FilterSettingsMaster(this);
    master->setModal(true);

    ServerFilter filter = items[0]->data(Qt::UserRole).value<ServerFilter>();
    master->setupFromFilter(filter);

    connect(master, SIGNAL(closedSave()), this, SLOT(sFilterSaved()));
    master->show();
}

void FilterList::sFilterSaved()
{
    FilterSettingsMaster* master = qobject_cast<FilterSettingsMaster*>(sender());
    if (!master) return;

    ServerFilter filter = master->getFilter();

    for (int i = 0; i < ui->filterList->count(); i++)
    {
        QListWidgetItem* item = ui->filterList->item(i);
        QString name = item->data(Qt::UserRole).value<ServerFilter>().getName();

        if (name.toLower() == filter.getName().toLower())
            item->setData(Qt::UserRole, QVariant::fromValue(filter));
    }

    master->deleteLater();
}

void FilterList::refreshList()
{
    ui->filterList->setUpdatesEnabled(false);

    for (int i = 0; i < ui->filterList->count(); i++)
    {
        QListWidgetItem* item = ui->filterList->item(i);
        if (i == 0)
        {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            const ServerFilter& filter = item->data(Qt::UserRole).value<ServerFilter>();
            item->setText("Global: "+filter.getName());
        }
        else
        {
            QFont f = item->font();
            f.setBold(false);
            item->setFont(f);
            const ServerFilter& filter = item->data(Qt::UserRole).value<ServerFilter>();
            item->setText(filter.getName());
        }
    }

    ui->filterList->setUpdatesEnabled(true);
}

void FilterList::on_filterList_doubleClicked(const QModelIndex &index)
{
    QListWidgetItem* item = ui->filterList->item(index.row());
    item->setSelected(true);
    on_button_edit_clicked();
}
