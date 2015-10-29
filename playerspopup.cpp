#include "playerspopup.h"
#include "ui_playerspopup.h"

static PlayersPopup* SystemPopup = 0;

PlayersPopup::PlayersPopup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayersPopup)
{
    ui->setupUi(this);
    this->setFocusPolicy(Qt::NoFocus);
    this->setAttribute(Qt::WA_ShowWithoutActivating, true);
    this->setWindowFlags(Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus|Qt::WindowTransparentForInput|Qt::ToolTip);
}

void PlayersPopup::updateFromServer(const Server& server)
{
    setUpdatesEnabled(false);

    ui->svlabel_GameMode->setText(server.GameModeString);
    // time
    if (server.TimeLimit > 0)
    {
        ui->svlabel_Time->setText(QString("%1/%2").arg(QString::number(server.TimeLimit-server.TimeLeft),
                                                       QString::number(server.TimeLimit)));
    }
    else
    {
        ui->svlabel_Time->setText("unlimited");
    }
    // frags
    QString frgType = server.getPointsName();
    if (frgType == "Frags" || frgType == "Points" || frgType == "Wins")
    {
        ui->svlabel_PointsName->setText(frgType+":");
        int limit = 0;
        if (frgType == "Frags") limit = server.FragLimit;
        else if (frgType == "Points") limit = server.PointLimit;
        else if (frgType == "Wins") limit = server.WinLimit;
        if (limit > 0)
        {
            // find top point team or player
            int topPts = 0;
            if (!server.Teams.isEmpty())
            {
                topPts = -32768;
                for (int i = 0; i < server.Teams.size(); i++)
                {
                    //qDebug("%s = %d", server.Teams[i].NameClean.toUtf8().data(), server.Teams[i].Points);
                    if (server.Teams[i].Points > topPts)
                        topPts = server.Teams[i].Points;
                }
            }
            else
            {
                topPts = -32768;
                for (int i = 0; i < server.Players.size(); i++)
                {
                    if (server.Players[i].Points > topPts)
                        topPts = server.Players[i].Points;
                }
            }
            ui->svlabel_Points->setText(QString("%1/%2").arg(QString::number(topPts),
                                                             QString::number(limit)));
        }
        else
        {
            ui->svlabel_Points->setText("unlimited");
        }
    }
    else // simulating IDE behavior. although IMO in this case we should just hide the points counter.
    {
        ui->svlabel_PointsName->setText("Frags:");
        ui->svlabel_Points->setText("unlimited");
    }
    // players
    int canjoin = server.MaxPlayers-server.PlayerCount;
    // sometimes canjoin can be negative.  for example when an admin forces their join into a nospec server.
    // let's keep it negative to amuse users  ;)
    QString ptext = QString("%1/%2").arg(QString::number(server.PlayerCount+server.BotCount),
                                         QString::number(server.MaxClients));
    if (canjoin != 0)
        ptext += QString(" (")+QString::number(canjoin)+QString(" can join)");
    ui->svlabel_Players->setText(ptext);
    // teams
    // first, remove all old labels
    for (int i = 0; i < teams.size(); i++)
        delete teams[i];
    teams.clear();

    for (int i = 0; i < server.Teams.size(); i++)
    {
        QLabel* teamlabel = new QLabel(ui->frame_2);
        teamlabel->setText(QString("%1: %2").arg(server.Teams[i].NameClean, QString::number(server.Teams[i].Points)));
        QFont nf = teamlabel->font();
        nf.setBold(true);
        teamlabel->setFont(nf);
        // now adjust label background
        QPalette np = teamlabel->palette();
        QColor lbg = server.Teams[i].Color;
        np.setColor(QPalette::Background, lbg);
        float luminance = 0.299*lbg.redF() + 0.587*lbg.greenF() + 0.114*lbg.blueF();
        if (luminance > 0.5)
            np.setColor(QPalette::WindowText, QColor(0, 0, 0, 255));
        else np.setColor(QPalette::WindowText, QColor(255, 255, 255, 255));
        teamlabel->setPalette(np);
        teamlabel->setAutoFillBackground(true);
        teamlabel->setMargin(2);
        ui->svlayout_teams->addWidget(teamlabel);
        teams.append(teamlabel);
    }

    ui->svlayout_teams->activate();

    setUpdatesEnabled(true);
}

// static
void PlayersPopup::showFromServer(const Server& server, int absx, int absy)
{
    if (!SystemPopup) SystemPopup = new PlayersPopup(0);
    else SystemPopup->hide();
    SystemPopup->setGeometry(absx, absy, SystemPopup->width(), SystemPopup->height());
    SystemPopup->updateFromServer(server);
    if (!server.isNA())
        SystemPopup->show();
}

void PlayersPopup::hideStatic()
{
    if (SystemPopup)
        SystemPopup->hide();
}

void PlayersPopup::freeStatic()
{
    if (SystemPopup)
        delete SystemPopup;
    SystemPopup = 0;
}

PlayersPopup::~PlayersPopup()
{
    delete ui;
}
