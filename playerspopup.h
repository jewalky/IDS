#ifndef PLAYERSPOPUP_H
#define PLAYERSPOPUP_H

#include <QWidget>
#include "serverupdater.h"
#include <QLabel>
#include <QVector>

namespace Ui {
class PlayersPopup;
}

class PlayersPopup : public QWidget
{
    Q_OBJECT

public:
    explicit PlayersPopup(QWidget *parent = 0);
    ~PlayersPopup();

    static void showFromServer(const Server& server, int absx, int absy);
    static void hideStatic();
    static void freeStatic();

private:
    void updateFromServer(const Server& server);

    QVector<QLabel*> teams;

    Ui::PlayersPopup *ui;
};

#endif // PLAYERSPOPUP_H
