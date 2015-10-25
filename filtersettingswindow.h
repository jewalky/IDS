#ifndef FILTERSETTINGSWINDOW_H
#define FILTERSETTINGSWINDOW_H

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>

#include "serverlist.h"

namespace Ui {
class FilterSettingsWindow;
}

class FilterSettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FilterSettingsWindow(QWidget *parent = 0);
    ~FilterSettingsWindow();

    void setKind(bool filter);
    bool isFilterEnabled();
    void setFilterEnabled(bool enabled);

    void setupFromPart(const ServerFilterPart& part);
    ServerFilterPart getPart();

private:
    Ui::FilterSettingsWindow *ui;

    void checkboxLink(QCheckBox* cb, QLineEdit* le, bool hasRegex);
    void checkboxSet(QCheckBox* cb, bool checked, bool regex);
    bool checkboxCheck(QCheckBox* cb);
    bool checkboxCheckRegex(QCheckBox* cb);

private slots:
    void onCheckboxChanged();
    void on_pwadAddButton_clicked();
    void on_pwadDelButton_clicked();
    void on_filterModesCheck_toggled(bool checked);
};

#endif // FILTERSETTINGSWINDOW_H
