#include "filtersettingsmaster.h"
#include "ui_filtersettingsmaster.h"

FilterSettingsMaster::FilterSettingsMaster(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterSettingsMaster)
{
    ui->setupUi(this);
    ui->lFilter->setKind(true);
    ui->lExceptions->setKind(false);
    ui->lFilter->setFilterEnabled(false);
    ui->lExceptions->setFilterEnabled(false);
}

void FilterSettingsMaster::setupFromFilter(const ServerFilter& filter)
{
    ui->lFilter->setFilterEnabled(filter.checkFlag(ServerFilter::Flag_HasFilter));
    ui->lExceptions->setFilterEnabled(filter.checkFlag(ServerFilter::Flag_HasExceptions));
    if (ui->lFilter->isFilterEnabled())
        ui->lFilter->setupFromPart(filter.getFilter());
    if (ui->lExceptions->isFilterEnabled())
        ui->lExceptions->setupFromPart(filter.getExceptions());
    setWindowTitle(QString("Filter settings: %1").arg(filter.getName()));
    Name = filter.getName();
}

ServerFilter FilterSettingsMaster::getFilter()
{
    ServerFilter filter;

    filter.setName(Name);

    if (ui->lFilter->isFilterEnabled())
    {
        filter.setFlag(ServerFilter::Flag_HasFilter, true);
        filter.setFilter(ui->lFilter->getPart());
    }

    if (ui->lExceptions->isFilterEnabled())
    {
        filter.setFlag(ServerFilter::Flag_HasExceptions, true);
        filter.setExceptions(ui->lExceptions->getPart());
    }

    return filter;
}

FilterSettingsMaster::~FilterSettingsMaster()
{
    delete ui;
}

void FilterSettingsMaster::on_saveButton_clicked()
{

    emit closedSave();
    close();
}

void FilterSettingsMaster::on_cancelButton_clicked()
{
    emit closedCancel();
    close();
}
