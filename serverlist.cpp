#include "serverlist.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollBar>

#include <QPainter>
#include <QStylePainter>

#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>

#include <QStyleOption>
#include <QStyleOptionHeader>

#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>

#include <QRegExp>

#include <cmath>

#include "mainwindow.h"
#include "settings.h"
#include "playerspopup.h"

ServerList* ServerList::serverComparatorContext = 0;

ServerList::ServerList(QWidget* parent) :
    QFrame(parent)
{
    Flags = 0;

    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ScrollArea = 0;
    ScrollView = 0;

    recalcColumnWidths();

    //setFlag(Flag_ClassicView, true);
    //setFlag(Flag_NoHighlight, true);
    //setFlag(Flag_CVGrid, true);
    //setFlag(Flag_CVPlayersTop, true);
    //setFlag(Flag_AnimateRefresh, true);

    HoveredServerColumn = HoveredServer = ClickedServer = SelectedServer = -1;
    HoveredCategory = ClickedCategory = -1;
    HoveredHeader = ClickedHeader = -1;

    SortingBy = ServerList_GameMode;
    SortingOrder = 1;

    setMouseTracking(true);

    // load data
    ISrv_Good = QPixmap(":/resources/icons/srv_good.png");
    ISrv_Bad = QPixmap(":/resources/icons/srv_bad.png");
    ISrv_UtterShit = QPixmap(":/resources/icons/srv_uttershit.png");
    ISrv_Banned = QPixmap(":/resources/icons/srv_banned.png");
    ISrv_Flood = QPixmap(":/resources/icons/srv_flood.png");
    ISrv_Na = QPixmap(":/resources/icons/srv_na.png");
    for (int i = 0; i < 8; i++)
        ISrv_Refresh[i] = QPixmap(QString(":/resources/icons/srv_ref%1.png").arg(QString::number(i+1)));

    IPl_Player = QPixmap(":/resources/icons/pl_normal.png");
    IPl_Bot = QPixmap(":/resources/icons/pl_bot.png");
    IPl_Spectator = QPixmap(":/resources/icons/pl_spect.png");
    IPl_Free = QPixmap(":/resources/icons/pl_free.png");
    IPl_Empty = QPixmap(":/resources/icons/pl_empty.png");

    connect(&ISrvTimer, SIGNAL(timeout()), this, SLOT(ISrvTimeout()));
    ISrvTimer.setInterval(80);
    ISrvTimer.start();

    ServerCatCount = 0;

    if (!Settings::get()->value("filters.configured").toBool())
    {
        // the default filter list
        QVector<ServerFilter> filters;

        ServerFilter filterGlobal;
        filterGlobal.setName("Default global filter");
        filterGlobal.setFlag(ServerFilter::Flag_Enabled, true);
        filters.append(filterGlobal);

        ServerFilter filterActive;
        filterActive.setName("Active");
        ServerFilterPart filterActiveNegative;
        filterActiveNegative.Flags |= ServerFilterPart::Flag_NA;
        filterActiveNegative.Flags |= ServerFilterPart::Flag_Empty;
        filterActive.setExceptions(filterActiveNegative);
        filterActive.setFlag(ServerFilter::Flag_Enabled|ServerFilter::Flag_HasExceptions, true);
        filters.append(filterActive);

        ServerFilter filterInactive;
        filterInactive.setName("Inactive");
        ServerFilterPart filterInactivePositive;
        filterInactivePositive.Flags |= ServerFilterPart::Flag_Empty;
        ServerFilterPart filterInactiveNegative;
        filterInactiveNegative.Flags |= ServerFilterPart::Flag_NA;
        filterInactive.setFilter(filterInactivePositive);
        filterInactive.setExceptions(filterInactiveNegative);
        filterInactive.setFlag(ServerFilter::Flag_Enabled|ServerFilter::Flag_HasFilter|ServerFilter::Flag_HasExceptions, true);
        filters.append(filterInactive);

        ServerFilter filterInvalid;
        filterInvalid.setName("Invalid");
        ServerFilterPart filterInvalidPositive;
        filterInvalidPositive.Flags |= ServerFilterPart::Flag_NA;
        ServerFilterPart filterInvalidNegative;
        filterInvalidNegative.Flags |= ServerFilterPart::Flag_NotNA;
        filterInvalid.setFilter(filterInvalidPositive);
        filterInvalid.setExceptions(filterInvalidNegative);
        filterInvalid.setFlag(ServerFilter::Flag_Enabled|ServerFilter::Flag_HasFilter|ServerFilter::Flag_HasExceptions, true);
        filters.append(filterInvalid);

        setFilterList(filters);
    }
    else
    {
        // restore filter list from whatever there was before
        QVector<ServerFilter> filters;
        QVariantList vlf = Settings::get()->value("filters.list").value< QVariantList >();
        for (int i = 0; i < vlf.size(); i++)
        {
            ServerFilter f;
            f.fromMap(vlf[i].value< QMap<QString, QVariant> >());
            filters.append(f);
        }

        setFilterList(filters);
    }
}

void ServerList::setScrollData(QScrollArea *area, QWidget *viewport)
{
    ScrollArea = area;
    ScrollView = viewport;
    updateWidget();
}

static bool filterCheckString(QString what, QString againstwhat, bool casesensitive, bool regex)
{
    if (!regex)
    {
        if (casesensitive)
            return what.trimmed() == againstwhat.trimmed();
        else return what.toLower().trimmed() == againstwhat.toLower().trimmed();
    }
    else
    {
        QRegExp re(againstwhat, casesensitive?Qt::CaseSensitive:Qt::CaseInsensitive, QRegExp::WildcardUnix);
        return re.exactMatch(what);
    }
}

bool ServerFilterPart::checkServer(Server &server) const
{
    if (Flags & Flag_Empty)
    {
        if (server.PlayerCount <= 0)
            return true;
    }

    if (Flags & Flag_Full)
    {
        if (server.PlayerCount >= server.MaxPlayers)
            return true;
    }

    if (Flags & Flag_Banned)
    {
        if (server.LastState == Server::State_Banned)
            return true;
    }

    if (Flags & Flag_Flood)
    {
        if (server.LastState == Server::State_Flood)
            return true;
    }

    if (Flags & Flag_NA)
    {
        if (server.isNA())
            return true;
    }

    if (Flags & Flag_ConnectPassword)
    {
        if (server.ForcePassword)
            return true;
    }

    if (Flags & Flag_JoinPassword)
    {
        if (server.ForceJoinPassword)
            return true;
    }

    if (Flags & Flag_HasBots)
    {
        if (server.BotCount > 0)
            return true;
    }

    if (Flags & Flag_OnlyBots)
    {
        if ((server.PlayerCount-server.BotCount <= 0) && (server.PlayerCount))
            return true;
    }

    if (Flags & Flag_NotNA)
    {
        if (!server.isNA())
            return true;
    }

    if (Flags & Flag_NoBots)
    {
        if (server.BotCount <= 0)
            return true;
    }

    if (Flags & Flag_HasSpectators)
    {
        if (server.SpectatorCount > 0)
            return true;
    }

    if (Flags & Flag_NoSpectators)
    {
        if (server.SpectatorCount <= 0)
            return true;
    }

    if (Flags & Flag_OnlySpectators)
    {
        if (server.PlayerCount-server.SpectatorCount <= 0)
            return true;
    }

    if (Flags & Flag_FilterGameModes)
    {
        if ((1 << server.GameMode) & GameModes)
            return true;
    }

    if (Flags & Flag_FilterServerName)
    {
        if (filterCheckString(server.Title, ServerName, false, !!(Flags & Flag_FilterServerNameRegex)))
            return true;
    }

    if (Flags & Flag_FilterServerIP)
    {
        if (filterCheckString(server.IPString, ServerIP, false, !!(Flags & Flag_FilterServerIPRegex)))
            return true;
    }

    if (Flags & Flag_FilterDMFlags)
    {
        if ((server.DMFlags & DMFlags) == DMFlags)
            return true;
    }

    if (Flags & Flag_FilterDMFlags2)
    {
        if ((server.DMFlags2 & DMFlags2) == DMFlags2)
            return true;
    }

    if (Flags & Flag_FilterZADMFlags)
    {
        if ((server.ZADMFlags & ZADMFlags) == ZADMFlags)
            return true;
    }

    if (Flags & Flag_FilterCompatFlags)
    {
        if ((server.CompatFlags & CompatFlags) == CompatFlags)
            return true;
    }

    if (Flags & Flag_FilterZACompatFlags)
    {
        if ((server.ZACompatFlags & ZACompatFlags) == ZACompatFlags)
            return true;
    }

    if (Flags & Flag_FilterIWAD)
    {
        if (filterCheckString(server.IWAD, IWAD, false, !!(Flags & Flag_FilterIWADRegex)))
            return true;
    }

    if (Flags & Flag_FilterPWADs)
    {
        for (int i = 0; i < server.PWADs.size(); i++)
        {
            for (int j = 0; j < PWADs.size(); j++)
            {
                if (filterCheckString(server.PWADs[i], PWADs[j], false, !!(Flags & Flag_FilterPWADsRegex)))
                    return true;
            }
        }
    }

    return false;
}

void ServerList::updateWidget()
{
    for (int i = 0; i < Servers.size(); i++)
    {
        Servers[i].Visible = false;
        Servers[i].CategoryCount = 0;
    }

    ServersVisual.clear();
    if (!checkFlag(Flag_ClassicView))
    {
        for (int i = 1; i < ServerCatCount; i++)
        {
            CategoriesServers[i] = 0;
            for (int j = 0; j < Servers.size(); j++)
            {
                ServerVisual svis;

                // check global filter. Filters[0].
                if ((Filters[0].checkFlag(ServerFilter::Flag_HasFilter) &&
                     !Filters[0].getFilter().checkServer(Servers[j])) ||
                    (Filters[0].checkFlag(ServerFilter::Flag_HasExceptions) &&
                     Filters[0].getExceptions().checkServer(Servers[j]))) // GLOBAL filter check (to remove certain servers from all lists)
                {
                    continue;
                    // this server won't ever be visible in the lists
                }


                if ((!Filters[i].checkFlag(ServerFilter::Flag_HasFilter) ||
                     Filters[i].getFilter().checkServer(Servers[j])) &&
                    (!Filters[i].checkFlag(ServerFilter::Flag_HasExceptions) ||
                     !Filters[i].getExceptions().checkServer(Servers[j]))) // here be the filter check
                {
                    CategoriesServers[i]++;

                    if (Filters[i].isEnabled() && isCategoryActive(i))
                    {
                        svis.Server = &Servers[j];
                        svis.Category = i;
                        ServersVisual.append(svis);
                        Servers[j].Visible = true;
                        Servers[j].CategoryCount++;
                    }
                }
            }
        }
    }
    else
    {
        for (int j = 0; j < ServerCatCount; j++)
            CategoriesServers[j] = 0;
        for (int i = 0; i < Servers.size(); i++)
        {
            for (int j = 1; j < ServerCatCount; j++)
            {
                if ((!Filters[j].checkFlag(ServerFilter::Flag_HasFilter) ||
                     Filters[j].getFilter().checkServer(Servers[i])) &&
                    (!Filters[j].checkFlag(ServerFilter::Flag_HasExceptions) ||
                     !Filters[j].getExceptions().checkServer(Servers[i]))) // here be the filter check
                {
                    CategoriesServers[j]++; // still count servers in categories
                }
            }

            if ((Filters[0].checkFlag(ServerFilter::Flag_HasFilter) &&
                 !Filters[0].getFilter().checkServer(Servers[i])) ||
                (Filters[0].checkFlag(ServerFilter::Flag_HasExceptions) &&
                 Filters[0].getExceptions().checkServer(Servers[i]))) // global filter is used even in classic views
            {
                continue;
            }

            // there should be the main filter. it's not implemented yet.
            ServerVisual svis;
            svis.Server = &Servers[i];
            svis.Category = 0;
            ServersVisual.append(svis);
            Servers[i].Visible = true;
        }
    }

    updateSorting(false, false);

    if (!ScrollArea || !ScrollView)
        return;

    // calculate min size for proper scrolling
    QFontMetrics fm = fontMetrics();

    QRect gr = rect();
    QRect vr = ScrollView->rect(); //vr.moveTo(ScrollArea->horizontalScrollBar()->value(), ScrollArea->verticalScrollBar()->value());

    QRect rect_header = gr;
    rect_header.setHeight(fm.height()+MainWindow::dpiScale(10));
    rect_header.setY(vr.y());

    QRect rect_contents = gr;
    rect_contents.setTop(rect_header.bottom());

    int htoffs = rect_header.height()+(checkFlag(Flag_ClassicView) ? 1 : 2);

    int ServerCatMax = checkFlag(Flag_ClassicView) ? 2 : ServerCatCount;
    for (int i = 1; i < ServerCatMax; i++)
    {
        // if category is visible
        if (!checkFlag(Flag_ClassicView))
        {
            if (CategoriesServers[i] <= 0)
                continue;

            QRect cat_rect = rect_contents;
            cat_rect.setTop(htoffs);
            cat_rect.setHeight(fm.height()+6);
            cat_rect.setLeft(cat_rect.left()+1);
            cat_rect.setRight(cat_rect.right()-1);
            CategoriesRects[i] = cat_rect;

            htoffs += cat_rect.height();
        }

        if (checkFlag(Flag_ClassicView) || (isCategoryActive(i) && (CategoriesServers[i] > 0)))
        {
            for (int j = 0; j < ServersVisual.size(); j++)
            {
                Server& srv = *ServersVisual[j].Server;
                ServerVisual& svis = ServersVisual[j];

                if (!checkFlag(Flag_ClassicView) && (svis.Category != i))
                    continue;
                if (!checkFlag(Flag_ClassicView) && !isCategoryActive(svis.Category))
                    continue;

                QRect srv_rect = rect_contents;
                srv_rect.setTop(htoffs);
                srv_rect.setHeight(fm.height()+8+(checkFlag(Flag_ClassicView) ? 0 : 2));
                srv_rect.setLeft(srv_rect.left()+1);
                srv_rect.setRight(srv_rect.right()-1);

                svis.Rect = srv_rect;

                htoffs += srv_rect.height()+1;
            }

            if (!checkFlag(Flag_ClassicView))
                htoffs += 8;
        }
        else htoffs += 3;
    }

    //setFixedHeight(htoffs);
    setMinimumHeight(htoffs);
}

void ServerList::recalcColumnWidths()
{
    QFontMetrics fm = fontMetrics();

    ColumnWidths[ServerList_Icon] = fm.height()+8;
    float pw = float(fm.height()+4)*0.8;
    ColumnWidths[ServerList_Players] = (pw*8+6);
    ColumnWidths[ServerList_Ping] = fm.width("99999")+fm.height()+12;
    ColumnWidths[ServerList_Country] = fm.height()*1.333+4;
    ColumnWidths[ServerList_Title] = -1;
    ColumnWidths[ServerList_IP] = fm.width("999.999.999.999:99999")+4;
    ColumnWidths[ServerList_Map] = fm.width("PROCTF24")+4;
    ColumnWidths[ServerList_IWAD] = fm.width("MEGAGAME.WAD")+4;
    ColumnWidths[ServerList_PWADs] = fm.width("WWWWWWWWWWWWWWWWWWWWWW")+4;
    ColumnWidths[ServerList_GameMode] = fm.width("TeamPossessionWW")+4;

    int count = 0;
    int leftWd = rect().width();
    for (int i = 0; i < ServerList_WIDTH; i++)
    {
        if (ColumnWidths[i] < 0)
            count++;
        else leftWd -= ColumnWidths[i];
    }

    for (int i = 0; i < ServerList_WIDTH; i++)
    {
        if (ColumnWidths[i] < 0)
            ColumnWidths[i] = leftWd/count;
    }
}

static void SL_paintPingIndicator(QPainter& p, QColor c, QRect r, int depth = 0)
{
    p.setRenderHint(QPainter::Antialiasing, true);

    QRect outerRect = r;
    QRect innerRect(outerRect);
    float x = r.x()+r.width()/2;
    float y = r.y()+r.height()/2;
    float radius = r.width()/2-1;
    outerRect.moveTo(x-radius, y-radius);

    if (depth <= 0)
    {
        QColor shadowColor(0, 0, 0, 16);
        QColor shadowColor2(0, 0, 0, 128);
        p.setPen(shadowColor);
        p.setBrush(shadowColor2);
        p.drawEllipse(outerRect);
    }

    QColor outerColor = c;
    outerColor.setHsl(c.hslHue(), c.hslSaturation(), c.lightness()/3);
    QColor outerColor2 = c;
    outerColor2.setHsl(c.hslHue(), c.hslSaturation(), c.lightness()/6);

    QRadialGradient qrgM(x-radius/6, y-radius/6, radius*2);
    qrgM.setColorAt(0.0, c);
    qrgM.setColorAt(1.0, outerColor);

    p.setBrush(qrgM);
    p.setPen(outerColor2);
    p.drawEllipse(innerRect);

    p.setRenderHint(QPainter::Antialiasing, false);

    if (depth <= 0)
    {
        QPainterPath oldClip = p.clipPath();

        QPainterPath pp;
        pp.addEllipse(r);

        QRect halfRec = r;
        halfRec.setHeight(halfRec.height()/2);
        QPainterPath pp2;
        pp2.addEllipse(halfRec);
        pp = pp.subtracted(pp2);

        p.setClipPath(pp, Qt::IntersectClip);
        QColor subColor = c;
        subColor.setHsl(c.hslHue(), c.hslSaturation(), float(c.lightness()) * 0.75);
        SL_paintPingIndicator(p, subColor, r, depth+1);
        p.setClipPath(oldClip);
    }
}

void ServerList::paintEvent(QPaintEvent *e)
{
    // do not draw if the reason is Scroll (thats done separately)
    QRect gr = rect();
    QRect vrh = ScrollView->rect(); vrh.moveTo(ScrollArea->horizontalScrollBar()->value(), ScrollArea->verticalScrollBar()->value());
    QRegion vr = e->region();

    /*if ((e->region().rectCount() == 1) && (vrh != e->region().boundingRect()))
        return;*/

    // draw column titles
    QPainter p(this);
    QPalette pal = palette();

    QFontMetrics fm = fontMetrics();

    QRect rect_header = gr;
    rect_header.setHeight(fm.height()+MainWindow::dpiScale(10));
    rect_header.setY(0);
    //qDebug("paint at %d, %d, %d, %d", vr.x(), vr.y(), vr.width(), vr.height());

    QRect rect_contents = gr;
    rect_contents.setTop(rect_header.bottom());
    p.setClipRect(rect_contents);
    p.fillRect(rect_contents, pal.base().color());

    QColor liteSep = pal.mid().color();
    liteSep.setHsl(liteSep.hslHue(), liteSep.hslSaturation(), std::min(255, liteSep.lightness()+72));

    int ServerCatMax = checkFlag(Flag_ClassicView) ? 2 : ServerCatCount;
    for (int i = 1; i < ServerCatMax; i++)
    {
        if (!checkFlag(Flag_ClassicView))
        {
            // if category is visible
            if (CategoriesServers[i] <= 0)
                continue;

            QRect& cat_rect = CategoriesRects[i];

            if (vr.intersects(cat_rect))
            {
                QColor midDark = pal.dark().color();

                int lit;
                if (ClickedCategory == i)
                    lit = 32;
                else if ((HoveredCategory == i) && (ClickedCategory < 0))
                    lit = 72;
                else lit = 96;
                midDark.setHsl(midDark.hslHue(), midDark.hslSaturation(), std::min(255, midDark.lightness()+lit));

                p.fillRect(cat_rect, midDark);

                p.setPen(pal.mid().color());
                p.drawLine(cat_rect.topLeft(), cat_rect.topRight());
                p.drawLine(cat_rect.bottomLeft(), cat_rect.bottomRight());

                QFont ftmp = font();
                ftmp.setBold(true);
                p.setFont(ftmp);
                QRect cat_textrect = cat_rect;
                cat_textrect.setLeft(cat_textrect.left()+32);
                cat_textrect.setRight(cat_textrect.right()-4);
                cat_textrect.setBottom(cat_textrect.bottom()-2);
                QTextOption to;
                to.setAlignment(Qt::AlignVCenter);
                p.setPen(pal.text().color());
                p.drawText(cat_textrect, getCategoryName(i) + " (" + QString::number(CategoriesServers[i]) + ")", to);
                p.setFont(font());

                p.setPen(Qt::NoPen);
                p.setBrush(pal.text().color());
                QPolygon arrow;
                arrow.append(QPoint(0, -2));
                arrow.append(QPoint(-4, 2));
                arrow.append(QPoint(3, 2));
                QMatrix arrow_mat;

                int addC = 0;
                if (isCategoryActive(i) && (CategoriesServers[i] > 0))
                {
                    arrow_mat.rotate(180);
                    addC = 0;
                }
                else
                {
                    arrow_mat.rotate(90);
                    addC = 1;
                }

                arrow = arrow_mat.map(arrow);
                arrow.translate(cat_rect.left()+16, cat_rect.top()+cat_rect.height()/2+addC);

                p.drawPolygon(arrow);
            }
        }

        if (checkFlag(Flag_ClassicView) || (isCategoryActive(i) && (CategoriesServers[i] > 0)))
        {
            for (int j = 0; j < ServersVisual.size(); j++)
            {
                Server& srv = *ServersVisual[j].Server;

                if (!checkFlag(Flag_ClassicView) && (ServersVisual[j].Category != i))
                    continue;

                QRect& srv_rect = ServersVisual[j].Rect;

                QColor textColor = pal.text().color();

                if (vr.intersects(srv_rect))
                {
                    if ((SelectedServer == j) || (ClickedServer == j))
                    {
                        p.fillRect(srv_rect, pal.highlight().color());
                        textColor = pal.highlightedText().color();
                    }
                    else if (!checkFlag(Flag_NoHighlight) && (HoveredServer == j))
                    {
                        QColor hilitP = pal.highlight().color();
                        hilitP.setAlpha(16);
                        p.fillRect(srv_rect, hilitP);
                    }

                    // draw server columns
                    int wdoffs = 0;
                    for (int k = 0; k < ServerList_WIDTH; k++)
                    {
                        QRect srvc_rect = srv_rect;
                        srvc_rect.setLeft(std::max(srvc_rect.left(), srvc_rect.left()-1+wdoffs));
                        srvc_rect.setRight(std::min(srvc_rect.right(), srvc_rect.left()+ColumnWidths[k]));

                        QRect srvcc_rect = srvc_rect;
                        srvcc_rect.setTop(srvcc_rect.top()+2);
                        srvcc_rect.setBottom(srvcc_rect.bottom()-2);
                        srvcc_rect.setLeft(srvcc_rect.left()+6);
                        srvcc_rect.setRight(srvcc_rect.right()-6);

                        if (k == ServerList_Title)
                        {
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter);
                            p.setPen(textColor);
                            p.drawText(srvcc_rect, srv.Title, to);
                        }
                        else if (k == ServerList_IP)
                        {
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter);
                            p.setPen(textColor);
                            p.drawText(srvcc_rect, srv.IPString, to);
                        }
                        else if (k == ServerList_GameMode)
                        {
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter);
                            p.setPen(textColor);
                            p.drawText(srvcc_rect, srv.GameModeString, to);
                        }
                        else if (k == ServerList_Ping)
                        {
                            int pingSize = fm.height()+1;

                            QRect srvcc_texrect = srvcc_rect;
                            srvcc_texrect.setLeft(srvcc_texrect.left()+pingSize+6);
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter|Qt::AlignHCenter);
                            p.setPen(textColor);

                            QString pingText = QString::number(srv.Ping);
                            if (srv.Ping <= 0)
                                pingText = "n/a";
                            else if (srv.Ping >= 1000)
                                pingText = ">1k";

                            p.drawText(srvcc_texrect, pingText, to);

                            QRect srvcc_imgrect = srvcc_rect;
                            srvcc_imgrect.setY(srvcc_rect.y()+srvcc_rect.height()/2-pingSize/2);
                            srvcc_imgrect.setWidth(pingSize);
                            srvcc_imgrect.setHeight(pingSize);

                            //
                            // ok, now measure how shit the server is.
                            QColor pingc = QColor(192, 192, 192), pingc2 = pingc;
                            int pingBad = Settings::get()->value("appearance.pingGood").toInt();
                            int pingHorrid = Settings::get()->value("appearance.pingAcceptable").toInt();
                            int hueMin = 0; // red
                            int hueMid = 60;
                            int hueMax = 120; // green
                            if ((srv.LastState == Server::State_Good) ||
                                (srv.LastState == Server::State_Banned) ||
                                (srv.LastState == Server::State_Flood))
                            {
                                if (srv.Ping < pingBad)
                                {
                                    int lp = (checkFlag(Flag_SmoothPing) ? hueMax - (srv.Ping * (hueMid-hueMin) / pingBad) : hueMax);
                                    pingc.setHsl(lp, 255, 255);
                                }
                                else if (srv.Ping < pingHorrid)
                                {
                                    int lp = (checkFlag(Flag_SmoothPing) ? hueMid - ((srv.Ping-pingBad) * (hueMax-hueMid) / (pingHorrid-pingBad)) : hueMid);
                                    pingc.setHsl(lp, 255, 255);
                                }
                                else if (srv.Ping != 0)
                                {
                                    pingc.setHsl(hueMin, 255, 255);
                                }
                            }
                            else if (srv.LastState == Server::State_Refreshing)
                            {
                                if (checkFlag(Flag_AnimateRefresh))
                                {
                                    pingc.setRgb(0, 255, 255);
                                    pingc.setHsl(pingc.hslHue(), pingc.hslSaturation()/4, 164+float(pingc.lightness())*(sin(float(ISrvIcon/2))/3));
                                }
                                else
                                {
                                    pingc.setRgb(0, 255, 255);
                                    pingc.setHsl(pingc.hslHue(), pingc.hslSaturation()/4, 255);
                                }
                            }

                            //p.fillRect(srvcc_imgrect, Qt::black);

                            if ((srv.LastState == Server::State_Banned) ||
                                (srv.LastState == Server::State_Flood))
                            {
                                QRect leftRect = srvcc_imgrect;
                                leftRect.setLeft(leftRect.left()-2);
                                leftRect.setRight(leftRect.right()+2);
                                leftRect.setTop(leftRect.top()-2);
                                leftRect.setBottom(leftRect.bottom()+2);
                                QRect rightRect = leftRect;
                                leftRect.setRight(leftRect.left()+leftRect.width()/2);
                                rightRect.setLeft(rightRect.left()+rightRect.width()/2);
                                QPainterPath oldClip = p.clipPath();
                                p.setClipRect(leftRect);
                                SL_paintPingIndicator(p, pingc, srvcc_imgrect);
                                p.setClipRect(rightRect);
                                SL_paintPingIndicator(p, pingc2, srvcc_imgrect);
                                p.setClipPath(oldClip);
                            }
                            else
                            {
                                SL_paintPingIndicator(p, pingc, srvcc_imgrect);
                            }
                        }
                        else if (k == ServerList_Players)
                        {
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter);
                            p.setPen(textColor);
                            //p.drawText(srvcc_rect, QString::number(srv.PlayerCount), to);

                            int n_players = srv.PlayerCount-srv.SpectatorCount-srv.BotCount;
                            int n_bots = srv.PlayerCount-srv.SpectatorCount;
                            int n_spec = srv.PlayerCount;
                            int n_free = srv.MaxPlayers;
                            int n_empty = srv.MaxClients;

                            float ph = fm.height()+6;
                            float pw = float(fm.height()+4)*0.8;

                            int maxshow = 8;
                            if (n_spec > 8)
                                maxshow = std::min(16, n_spec);
                            if (maxshow > 8)
                                pw *= 8/float(maxshow);

                            for (int l = 0; l < std::min(srv.MaxClients, maxshow); l++)
                            {
                                QRectF srvcc_player = QRectF(srvcc_rect.right()-pw*(l+1)+4,
                                                             srvcc_rect.top()-1,
                                                             pw, ph+(checkFlag(Flag_ClassicView)?0:1));
                                p.setRenderHint(QPainter::SmoothPixmapTransform, true);

                                QPixmap* px = 0;

                                if (l < n_players)
                                    px = &IPl_Player;
                                else if (l < n_bots)
                                    px = &IPl_Bot;
                                else if (l < n_spec)
                                    px = &IPl_Spectator;
                                else if (l < n_free)
                                    px = &IPl_Free;
                                else if (l < n_empty)
                                    px = &IPl_Empty;

                                if (px)
                                    p.drawPixmap(srvcc_player, *px, QRectF(0, 0, px->width(), px->height()));
                            }
                        }
                        else if (k == ServerList_IWAD)
                        {
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter);
                            p.setPen(textColor);
                            p.drawText(srvcc_rect, srv.IWAD.toLower(), to);
                        }
                        else if (k == ServerList_PWADs)
                        {
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter);
                            p.setPen(textColor);
                            QStringList pwads_list;
                            for (int l = 0; l < srv.PWADs.size(); l++)
                            {
                                if (srv.PWADs[l].toLower().contains("skulltag_data") ||
                                    srv.PWADs[l].toLower().contains("skulltag_actors"))
                                        continue;
                                pwads_list.append(srv.PWADs[l]);
                            }
                            QString pwads_str = pwads_list.join(", ");
                            p.drawText(srvcc_rect, pwads_str, to);
                        }
                        else if (k == ServerList_Map)
                        {
                            QTextOption to;
                            to.setWrapMode(QTextOption::NoWrap);
                            to.setAlignment(Qt::AlignVCenter);
                            p.setPen(textColor);
                            p.drawText(srvcc_rect, srv.Map.toLower(), to);
                        }

                        if (checkFlag(Flag_ClassicView|Flag_CVGrid) && (k != ServerList_WIDTH-1))
                        {
                            p.setPen(liteSep);
                            p.drawLine(srvc_rect.right(), srvc_rect.top(), srvc_rect.right(), srvc_rect.bottom());
                        }

                        wdoffs += ColumnWidths[k];
                    }

                    p.setPen(liteSep);
                    if (!checkFlag(Flag_ClassicView) || ((j != Servers.size()-1) && !checkFlag(Flag_CVNoGrid)))
                        p.drawLine(srv_rect.left(), srv_rect.bottom()+1, srv_rect.right(), srv_rect.bottom()+1);
                }
            }
        }
    }

    if (vr.intersects(vrh))
        paintHeader(p, vrh);
}

void ServerList::paintHeader(QPainter& p, QRect vr)
{
    QFontMetrics fm = fontMetrics();
    QPalette pal = palette();
    QRect rect_header = vr;
    rect_header.setHeight(fm.height()+MainWindow::dpiScale(10));

    p.setClipRect(vr);
    rect_header.setY(vr.top());
    rect_header.setHeight(fm.height()+MainWindow::dpiScale(10));

    p.fillRect(rect_header, pal.background().color());

    p.setPen(pal.light().color());
    p.drawLine(rect_header.topLeft(), rect_header.topRight());
    p.drawLine(rect_header.topLeft(), rect_header.bottomLeft());

    p.setPen(pal.dark().color());
    p.drawLine(rect_header.left()+1, rect_header.bottom()-1, rect_header.right(), rect_header.bottom()-1);
    p.drawLine(rect_header.topRight(), rect_header.bottomRight());

    QColor veryDark = pal.dark().color();
    veryDark.setHsl(veryDark.hslHue(), veryDark.hslSaturation(), std::max(0, veryDark.lightness()-64));
    p.setPen(veryDark);
    p.drawLine(rect_header.left(), rect_header.bottom(), rect_header.right(), rect_header.bottom());

    QStylePainter sp(this);

    bool native = ((style()->metaObject()->className()) != QString("QWindowsStyle"));

    int wdoffs = 0;
    for (int i = 0; i < ServerList_WIDTH; i++)
    {
        // draw the header
        int wdc = ColumnWidths[i];
        QString title = getColumnName(i);

        QRect rect_headercur = rect_header;
        rect_headercur.setLeft(rect_headercur.left()+wdoffs);
        rect_headercur.setRight(rect_headercur.left()+wdc);
        rect_headercur.setRight(rect_headercur.right());

        QStyleOptionHeader opt;
        if (native)
        {
            opt.initFrom(this);
            opt.orientation = Qt::Horizontal;
            opt.direction = Qt::LeftToRight;
            if (i-1 >= 0 && i+1 < ServerList_WIDTH)
                opt.position = QStyleOptionHeader::Middle;
            else if (i == 0)
                opt.position = QStyleOptionHeader::Beginning;
            else opt.position = QStyleOptionHeader::End;
            opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
            opt.rect = rect_headercur;
            opt.rect.setRight(opt.rect.right()-1);

            opt.state = QStyle::State_None|QStyle::State_Raised|QStyle::State_Horizontal;
            if (isEnabled()) opt.state |= QStyle::State_Enabled;
            if (HoveredHeader == i)
                opt.state |= QStyle::State_MouseOver;
            if (ClickedHeader == i)
                opt.state |= QStyle::State_Sunken;
            if (window()->isActiveWindow())
                opt.state |= QStyle::State_Active;
            opt.text = title;
            //opt.textAlignment = Qt::AlignVCenter;
            if ((i == ServerList_Players) && checkFlag(Flag_ClassicView|Flag_CVPlayersTop))
            {
                opt.sortIndicator = QStyleOptionHeader::SortDown;
            }
            else if (SortingBy == i)
            {
                if (SortingOrder == 0) // asc
                    opt.sortIndicator = QStyleOptionHeader::SortUp;
                else opt.sortIndicator = QStyleOptionHeader::SortDown; // desc
            }
            //sp.drawControl(QStyle::CE_Header, opt);
            //style()->drawControl(QStyle::CE_HeaderSection, &opt, &p, this);
            //style()->drawControl(QStyle::CE_Header, &opt, &p, this);
            sp.drawControl(QStyle::CE_Header, opt);
        }
        else
        {
            if (i+1 < ServerList_WIDTH)
            {
                p.setPen(pal.dark().color());
                p.drawLine(rect_headercur.right()-2, rect_headercur.top()+3, rect_headercur.right()-2, rect_headercur.bottom()-5);
                p.drawLine(rect_headercur.right()-2, rect_headercur.top()+3, rect_headercur.right()-1, rect_headercur.top()+3);
                p.setPen(pal.light().color());
                p.drawLine(rect_headercur.right()-1, rect_headercur.top()+4, rect_headercur.right()-1, rect_headercur.bottom()-5);
            }

            if (ClickedHeader == i)
            {
                QRect fR = rect_headercur;
                fR.setTop(fR.top());
                fR.setBottom(fR.bottom()-2);
                fR.setLeft(fR.left()+1);
                fR.setRight(fR.right()-4);
                QColor fC = pal.dark().color();
                fC.setAlpha(127);
                p.fillRect(fR, fC);
            }
        }

        rect_headercur.setTop(rect_headercur.top()+1);
        rect_headercur.setBottom(rect_headercur.bottom()-4);
        rect_headercur.setLeft(rect_headercur.left()+6);
        rect_headercur.setRight(rect_headercur.right()-6);

        if (native)
        {
            //opt.rect = rect_headercur;
            //sp.drawControl(QStyle::CE_HeaderLabel, opt);
        }
        else
        {
            QTextOption to;
            to.setAlignment(Qt::AlignVCenter);
            p.setPen(pal.text().color());
            //p.setClipRect(rect_headercur);
            p.drawText(rect_headercur, title, to);
            //p.setClipping(false);

            if (SortingBy == i)
            {
                p.setPen(Qt::NoPen);
                p.setBrush(pal.text().color());
                QPolygon arrow;
                arrow.append(QPoint(0, -2));
                arrow.append(QPoint(-4, 2));
                arrow.append(QPoint(4, 2));
                QMatrix arrow_mat;

                int addC = 0;
                if (!SortingOrder)
                {
                    //arrow_mat.rotate(180);
                    arrow_mat.scale(1.0, -1.0);
                    addC = 1;
                }
                else
                {
                    addC = 1;
                }

                arrow = arrow_mat.map(arrow);
                arrow.translate(rect_headercur.right()-6, rect_headercur.top()+rect_headercur.height()/2+addC);

                p.drawPolygon(arrow);
            }
        }

        wdoffs += wdc;
    }
}

void ServerList::resizeEvent(QResizeEvent *)
{
    recalcColumnWidths();
    updateWidget();
}

void ServerList::mousePressEvent(QMouseEvent *e)
{
    mouseMoveEvent(e); // process hovering

    if (e->button() == Qt::LeftButton)
    {
        if (ClickedCategory != HoveredCategory)
        {
            ClickedCategory = HoveredCategory;
            update();
        }

        if ((SelectedServer != HoveredServer) ||
            (ClickedServer != HoveredServer))
        {
            SelectedServer = ClickedServer = HoveredServer;
            update();
        }

        if (ClickedHeader != HoveredHeader)
        {
            ClickedHeader = HoveredHeader;
            update();
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        if (ClickedServer != HoveredServer)
        {
            SelectedServer = ClickedServer = HoveredServer;
            update();
        }

        if ((SelectedServer >= 0) && (SelectedServer < ServersVisual.size()))
        {
            emit updateRequested(ServersVisual[SelectedServer].Server->IP,
                                 ServersVisual[SelectedServer].Server->Port);
        }
    }
}

void ServerList::mouseReleaseEvent(QMouseEvent *e)
{
    if (ClickedCategory >= 0)
    {
        // do something, like activate/deactivate, and repaint
        if (HoveredCategory == ClickedCategory)
        {
            Filters[ClickedCategory].setEnabled(!Filters[ClickedCategory].isEnabled());
            saveFilterList();
            updateWidget();
        }
    }

    if ((ClickedCategory != -1) || (ClickedServer != -1))
    {
        ClickedCategory = -1;
        ClickedServer = -1;
        update();
    }

    if (ClickedHeader >= 0)
    {
        if (HoveredHeader == ClickedHeader)
        {
            if (checkFlag(Flag_ClassicView) && (ClickedHeader == ServerList_Players))
            {
                setFlag(Flag_CVPlayersTop, !checkFlag(Flag_CVPlayersTop));
            }
            else if (SortingBy != ClickedHeader)
            {
                SortingBy = ClickedHeader;
                SortingOrder = 1; // 0 sounds more rational, but IDE sets it to ascending on first click.
            }
            else SortingOrder = !SortingOrder;
            updateSorting();
        }
        ClickedHeader = -1;
        update();
    }
}

void ServerList::mouseMoveEvent(QMouseEvent *e)
{
    int OldHoveredCategory = HoveredCategory;
    int OldHoveredServerColumn = HoveredServerColumn;
    int OldHoveredServer = HoveredServer;
    int OldHoveredHeader = HoveredHeader;

    QRect vr = ScrollView->rect(); vr.moveTo(ScrollArea->horizontalScrollBar()->value(), ScrollArea->verticalScrollBar()->value());

    QFontMetrics fm = fontMetrics();
    QPalette pal = palette();
    QRect rect_header = vr;
    rect_header.setHeight(fm.height()+MainWindow::dpiScale(10));

    bool inHeader = false;
    int wdoffs = 0;
    for (int i = 0; i < ServerList_WIDTH; i++)
    {
        // draw the header
        int wdc = ColumnWidths[i];

        QRect rect_headercur = rect_header;
        rect_headercur.setLeft(rect_headercur.left()+wdoffs);
        rect_headercur.setRight(rect_headercur.left()+wdc);

        if (rect_headercur.contains(e->pos()))
        {
            HoveredCategory = -1;
            HoveredServer = -1;
            HoveredServerColumn = -1;
            HoveredHeader = i;
            inHeader = true;
            break;
        }

        wdoffs += wdc;
    }

    if (!inHeader)
    {
        HoveredHeader = -1;
        HoveredCategory = -1;
        for (int i = 1; i < ServerCatCount; i++)
        {
            if (CategoriesRects[i].contains(e->pos()))
                HoveredCategory = i;
        }

        HoveredServer = -1;
        HoveredServerColumn = -1;
        for (int i = 0; i < ServersVisual.size(); i++)
        {
            QRect svrec = ServersVisual[i].Rect;
            svrec.setBottom(svrec.bottom()+1);
            if (svrec.contains(e->pos()))
            {
                HoveredServer = i;
                wdoffs = 0;
                for (int j = 0; j < ServerList_WIDTH; j++)
                {
                    int wdc = ColumnWidths[j];

                    QRect rect_headercur = svrec;
                    rect_headercur.setLeft(rect_headercur.left()+wdoffs);
                    rect_headercur.setRight(rect_headercur.left()+wdc);

                    if (rect_headercur.contains(e->pos()))
                    {
                        HoveredServerColumn = j;
                        break;
                    }

                    wdoffs += wdc;
                }
            }
        }
    }

    if (OldHoveredCategory != HoveredCategory)
        update();
    else if ((OldHoveredServer != HoveredServer) && !checkFlag(Flag_NoHighlight))
    {
        QRect oldRect = (OldHoveredServer >= 0) ? ServersVisual[OldHoveredServer].Rect : QRect(0, 0, 0, 0);
        QRect newRect = (HoveredServer >= 0) ? ServersVisual[HoveredServer].Rect : QRect(0, 0, 0, 0);
        update(oldRect);
        update(newRect);
    }
    else if (OldHoveredHeader != HoveredHeader)
        update();

    if (OldHoveredServer != HoveredServer ||
            OldHoveredServerColumn != HoveredServerColumn)
    {
        //
        if (OldHoveredServerColumn == ServerList_Players &&
                HoveredServerColumn != ServerList_Players)
        {
            PlayersPopup::hideStatic();
        }

        if (HoveredServer >= 0 && HoveredServerColumn >= 0)
        {
            if (HoveredServerColumn == ServerList_Players)
            {
                PlayersPopup::showFromServer(*ServersVisual[HoveredServer].Server, e->globalX()+4, e->globalY()-4);
            }
        }
    }
}

void ServerList::enterEvent(QEvent *)
{

}

void ServerList::leaveEvent(QEvent *)
{
    if ((HoveredCategory != -1) ||
        (HoveredServer != -1) ||
        (HoveredHeader != -1))
    {
        HoveredCategory = -1;
        HoveredServer = -1;
        if (HoveredServerColumn == ServerList_Players)
            PlayersPopup::hideStatic();
        HoveredServerColumn = -1;
        HoveredHeader = -1;
        update();
    }
}

bool ServerList::serverComparator(const ServerVisual &sv1, const ServerVisual &sv2)
{
    Server& s1 = *sv1.Server;
    Server& s2 = *sv2.Server;

    int col = serverComparatorContext->SortingBy;
    int order = serverComparatorContext->SortingOrder;

    bool B_TRUE = false ^ order;
    bool B_FALSE = true ^ order;
    bool B_EQUAL = false;

    if (!serverComparatorContext->checkFlag(Flag_ClassicView) &&
        (sv1.Category != sv2.Category))
            return (sv1.Category < sv2.Category);

    // additional sort by IP. in IDE this is done implicitly, but I have free sorting here and have to do it manually
    if (serverComparatorContext->checkFlag(Flag_ClassicView|Flag_CVSortPlayers) &&
            !!(s1.PlayerCount) && !!(s2.PlayerCount))
    {
        if ((s1.PlayerCount) >= (s2.PlayerCount))
            B_EQUAL = true;
    }
    else
    {
        if (s1.IP < s2.IP)
            B_EQUAL = true;
        else if ((s1.IP == s2.IP) && (s1.Port < s2.Port))
            B_EQUAL = true;
    }

    if (serverComparatorContext->checkFlag(Flag_ClassicView))
    {
        if (serverComparatorContext->checkFlag(Flag_CVPlayersTop) &&
            (!!(s1.PlayerCount) != !!(s2.PlayerCount)))
                return !!(s1.PlayerCount);
    }

    if ((order != 0) && (order != 1))
        return false;

    if (col == ServerList_IP)
    {
        if (s1.IP < s2.IP)
            return B_TRUE;
        else if ((s1.IP == s2.IP) && (s1.Port < s2.Port))
            return B_TRUE;
        else if ((s1.IP == s2.IP) && (s1.Port == s2.Port))
            return B_EQUAL;
        return B_FALSE;
    }
    else if (col == ServerList_Title)
    {
        int c = s1.Title.compare(s2.Title, Qt::CaseSensitive);
        if (c < 0) return B_TRUE;
        if (c == 0) return B_EQUAL;
        return B_FALSE;
    }
    else if (col == ServerList_GameMode)
    {
        int c = s1.GameModeString.compare(s2.GameModeString, Qt::CaseSensitive);
        if (c < 0) return B_TRUE;
        if (c == 0) return B_EQUAL;
        return B_FALSE;
    }
    else if ((col == ServerList_Players) && !serverComparatorContext->checkFlag(Flag_ClassicView))
    {
        if ((s1.PlayerCount) < (s2.PlayerCount))
            return B_TRUE;
        else if ((s1.PlayerCount) == (s2.PlayerCount))
            return B_EQUAL;
        return B_FALSE;
    }
    else if (col == ServerList_Ping)
    {
        if (s1.Ping < s2.Ping)
            return B_TRUE;
        else if (s1.Ping == s2.Ping)
            return B_EQUAL;
        return B_FALSE;
    }
    else if (col == ServerList_IWAD)
    {
        int c = s1.IWAD.compare(s2.IWAD, Qt::CaseInsensitive);
        if (c < 0) return B_TRUE;
        if (c == 0) return B_EQUAL;
        return B_FALSE;
    }
    else if (col == ServerList_Map)
    {
        int c = s1.Map.compare(s2.Map, Qt::CaseInsensitive);
        if (c < 0) return B_TRUE;
        if (c == 0) return B_EQUAL;
        return B_FALSE;
    }

    return false;
}

void ServerList::keyPressEvent(QKeyEvent *e)
{
    if (!ScrollArea)
        return;

    QRect oldRect = (SelectedServer >= 0) ? ServersVisual[SelectedServer].Rect : QRect(0, 0, 0, 0);

    if (e->key() == Qt::Key_Down)
    {
        if (SelectedServer < ServersVisual.size()-1)
            SelectedServer++;

        scrollToServer(SelectedServer);

        QRect newRect = (SelectedServer >= 0) ? ServersVisual[SelectedServer].Rect : QRect(0, 0, 0, 0);
        update(oldRect);
        update(newRect);
    }
    else if (e->key() == Qt::Key_Up)
    {
        if (SelectedServer > 0)
            SelectedServer--;

        scrollToServer(SelectedServer);

        QRect newRect = (SelectedServer >= 0) ? ServersVisual[SelectedServer].Rect : QRect(0, 0, 0, 0);
        update(oldRect);
        update(newRect);
    }
    else if ((e->key() == Qt::Key_Home) && !Servers.isEmpty())
    {
        SelectedServer = 0;
        if (SelectedServer > Servers.size())
            SelectedServer = -1;
        else scrollToServer(SelectedServer);

        QRect newRect = (SelectedServer >= 0) ? ServersVisual[SelectedServer].Rect : QRect(0, 0, 0, 0);
        update(oldRect);
        update(newRect);
    }
    else if ((e->key() == Qt::Key_End) && !Servers.isEmpty())
    {
        SelectedServer = Servers.size()-1;
        if (SelectedServer >= 0)
            scrollToServer(SelectedServer);

        QRect newRect = (SelectedServer >= 0) ? ServersVisual[SelectedServer].Rect : QRect(0, 0, 0, 0);
        update(oldRect);
        update(newRect);
    }
    else if ((e->key() == Qt::Key_PageUp) && !Servers.isEmpty())
    {
        int serverHeight = fontMetrics().height()+8+(checkFlag(Flag_ClassicView) ? 0 : 2);
        int serversOnPage = (ScrollArea->viewport()->height() / serverHeight)-2;
        for (int i = 0; i < serversOnPage; i++)
        {
            if (SelectedServer > 0)
                SelectedServer--;
            else break;
        }
        scrollToServer(SelectedServer);

        QRect newRect = (SelectedServer >= 0) ? ServersVisual[SelectedServer].Rect : QRect(0, 0, 0, 0);
        update(oldRect);
        update(newRect);
    }
    else if ((e->key() == Qt::Key_PageDown) && !Servers.isEmpty())
    {
        int serverHeight = fontMetrics().height()+8+(checkFlag(Flag_ClassicView) ? 0 : 2);
        int serversOnPage = (ScrollArea->viewport()->height() / serverHeight)-2;
        for (int i = 0; i < serversOnPage; i++)
        {
            if (SelectedServer < ServersVisual.size()-1)
                SelectedServer++;
            else break;
        }
        scrollToServer(SelectedServer);

        QRect newRect = (SelectedServer >= 0) ? ServersVisual[SelectedServer].Rect : QRect(0, 0, 0, 0);
        update(oldRect);
        update(newRect);
    }
    else if ((e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_C))
    {
        if ((SelectedServer >= 0) && (SelectedServer < ServersVisual.size()))
        {
            QClipboard* cb = QApplication::clipboard();
            if (cb) cb->setText(ServersVisual[SelectedServer].Server->IPString);
        }
    }
    else if ((e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Z))
    {
        if ((SelectedServer >= 0) && (SelectedServer < ServersVisual.size()))
        {
            QClipboard* cb = QApplication::clipboard();
            if (cb) cb->setText("zds://"+ServersVisual[SelectedServer].Server->IPString+"/za");
        }
    }
}

void ServerList::scrollToServer(int server)
{
    if (!ScrollArea)
        return;
    if ((server < 0) || (server >= Servers.size()))
        return;

    QRect srec = ServersVisual[server].Rect;
    //vr.moveTo(ScrollArea->horizontalScrollBar()->value(), ScrollArea->verticalScrollBar()->value());

    int hheight = fontMetrics().height()+MainWindow::dpiScale(10);

    QRect vr = ScrollView->rect(); vr.moveTo(ScrollArea->horizontalScrollBar()->value(), ScrollArea->verticalScrollBar()->value());
    vr.setTop(vr.top()+hheight);
    if (srec.intersects(vr))
        return;

    int maxScroll = srec.bottom() - ScrollArea->viewport()->height() + 2;
    int minScroll = srec.top() - hheight - 1;

    if (srec.top() > vr.bottom())
    {
        ScrollArea->verticalScrollBar()->setValue(maxScroll);
    }
    else
    {
        ScrollArea->verticalScrollBar()->setValue(minScroll);
    }

    update();
}

void ServerList::saveFilterList()
{
    // also save filters
    const QVector<ServerFilter>& v = getFilterList();
    QVariantList vl;
    for (int i = 0; i < v.size(); i++)
    {
        const ServerFilter& vf = v[i];
        QMap<QString, QVariant> vlf = vf.asMap();
        vl.append(QVariant(vlf));
    }

    Settings::get()->setValue("filters.configured", true);
    Settings::get()->setValue("filters.list", vl);
}
