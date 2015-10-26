#ifndef SERVERLIST_H
#define SERVERLIST_H

#include <QWidget>
#include <QScrollArea>
#include <QFrame>
#include <QMutex>
#include <QTimer>
#include <QPixmap>
#include <QVariant>

enum
{
    ServerList_Icon,
    ServerList_Players,
    ServerList_Ping,
    ServerList_Country,
    ServerList_Title,
    ServerList_IP,
    ServerList_Map,
    ServerList_IWAD,
    ServerList_PWADs,
    ServerList_GameMode,

    ServerList_WIDTH
};

struct Server;
struct ServerFilterPart
{
    enum
    {
        Flag_Empty                  = 0x00000001,
        Flag_Full                   = 0x00000002,
        Flag_Banned                 = 0x00000004,
        Flag_Flood                  = 0x00000008,
        Flag_NA                     = 0x00000010,
        Flag_ConnectPassword        = 0x00000020,
        Flag_JoinPassword           = 0x00000040,
        Flag_HasBots                = 0x00000080,
        Flag_OnlyBots               = 0x00000100,
        Flag_NotNA                  = 0x00000200,
        Flag_NoBots                 = 0x00000400,
        Flag_HasSpectators          = 0x00000800,
        Flag_NoSpectators           = 0x00001000,
        Flag_OnlySpectators         = 0x00002000,

        Flag_FilterGameModes        = 0x00010000,
        Flag_FilterServerName       = 0x00020000,
        Flag_FilterServerIP         = 0x00040000,
        Flag_FilterServerNameRegex  = 0x00080000,
        Flag_FilterServerIPRegex    = 0x00100000,
        Flag_FilterDMFlags          = 0x00200000,
        Flag_FilterDMFlags2         = 0x00400000,
        Flag_FilterZADMFlags        = 0x00800000,
        Flag_FilterCompatFlags      = 0x01000000,
        Flag_FilterZACompatFlags    = 0x02000000,
        Flag_FilterIWAD             = 0x04000000,
        Flag_FilterPWADs            = 0x08000000,
        Flag_FilterIWADRegex        = 0x10000000,
        Flag_FilterPWADsRegex       = 0x20000000
    };

    quint32 Flags;
    quint32 GameModes;
    QString ServerName;
    QString ServerIP;
    QString IWAD;
    QStringList PWADs;
    quint32 DMFlags;
    quint32 DMFlags2;
    quint32 ZADMFlags;
    quint32 CompatFlags;
    quint32 ZACompatFlags;

    QMap<QString, QVariant> asMap() const
    {
        QMap<QString, QVariant> map;
        map["flags"] = Flags;
        map["game_modes"] = GameModes;
        map["server_name"] = ServerName;
        map["server_ip"] = ServerIP;
        map["iwad"] = IWAD;
        map["pwads"] = PWADs;
        map["dmflags"] = DMFlags;
        map["dmflags2"] = DMFlags2;
        map["zadmflags"] = ZADMFlags;
        map["compatflags"] = CompatFlags;
        map["zacompatflags"] = ZACompatFlags;
        return map;
    }

    void fromMap(QMap<QString, QVariant> map)
    {
        Flags = map["flags"].toUInt();
        GameModes = map["game_modes"].toUInt();
        ServerName = map["server_name"].toString();
        ServerIP = map["server_ip"].toString();
        IWAD = map["iwad"].toString();
        PWADs = map["pwads"].toStringList();
        DMFlags = map["dmflags"].toUInt();
        DMFlags2 = map["dmflags2"].toUInt();
        ZADMFlags = map["zadmflags"].toUInt();
        CompatFlags = map["compatflags"].toUInt();
        ZACompatFlags = map["zacompatflags"].toUInt();
    }

    bool checkServer(Server& server) const;

    ServerFilterPart()
    {
        Flags = 0;
        GameModes = 0;
        DMFlags = 0;
        DMFlags2 = 0;
        ZADMFlags = 0;
        CompatFlags = 0;
        ZACompatFlags = 0;
    }
};

class ServerFilter
{
public:
    ServerFilter()
    {
        Enabled = false;
        Flags = 0;
    }

    void setFilter(const ServerFilterPart& filter)
    {
        Filter = filter;
    }

    void setExceptions(const ServerFilterPart& filter)
    {
        Exceptions = filter;
    }

    const ServerFilterPart& getFilter() const
    {
        return Filter;
    }

    const ServerFilterPart& getExceptions() const
    {
        return Exceptions;
    }

    enum
    {
        Flag_Global         = 0x0001,
        Flag_HasColor       = 0x0002,
        Flag_Enabled        = 0x0004,
        Flag_HasFilter      = 0x0008,
        Flag_HasExceptions  = 0x0010
    };

    bool checkFlag(quint32 flag) const
    {
        return ((Flags & flag) == flag);
    }

    void setFlag(quint32 flag, bool on)
    {
        if (on) Flags |= flag;
        else Flags &= ~flag;
    }

    QString getName() const
    {
        return Name;
    }

    void setName(QString name)
    {
        Name = name;
    }

    bool isEnabled() const
    {
        return checkFlag(Flag_Enabled);
    }

    void setEnabled(bool enabled)
    {
        setFlag(Flag_Enabled, enabled);
    }

    QMap<QString, QVariant> asMap() const
    {
        QMap<QString, QVariant> map;
        map["flags"] = Flags;
        map["filter"] = Filter.asMap();
        map["exceptions"] = Exceptions.asMap();
        map["name"] = Name;
        map["enabled"] = Enabled;
        return map;
    }

    void fromMap(QMap<QString, QVariant> map)
    {
        Flags = map["flags"].toUInt();
        Filter.fromMap(map["filter"].value< QMap<QString, QVariant> >());
        Exceptions.fromMap(map["exceptions"].value< QMap<QString, QVariant> >());
        Name = map["name"].toString();
        Enabled = map["enabled"].toBool();
    }

private:
    quint32 Flags;
    ServerFilterPart Filter;
    ServerFilterPart Exceptions;
    QString Name;
    bool Enabled;
};

Q_DECLARE_METATYPE(ServerFilter)
Q_DECLARE_METATYPE(ServerFilterPart)

struct Team
{
    QString Name;
    QString NameClean;
    QColor Color;
    int Points;
};

struct Player
{
    QString Name;
    QString NameClean;
    int Points;
    int Ping;
    bool IsSpectator;
    bool IsBot;
    int Team;
    int Time;
};

struct Server
{
    enum
    {
        GAMEMODE_COOPERATIVE,
        GAMEMODE_SURVIVAL,
        GAMEMODE_INVASION,
        GAMEMODE_DEATHMATCH,
        GAMEMODE_TEAMPLAY,
        GAMEMODE_DUEL,
        GAMEMODE_TERMINATOR,
        GAMEMODE_LASTMANSTANDING,
        GAMEMODE_TEAMLMS,
        GAMEMODE_POSSESSION,
        GAMEMODE_TEAMPOSSESSION,
        GAMEMODE_TEAMGAME,
        GAMEMODE_CTF,
        GAMEMODE_ONEFLAGCTF,
        GAMEMODE_SKULLTAG,
        GAMEMODE_DOMINATION,
    };

    int Ping;
    QString CountryString;
    QPixmap CountryImage;
    QString Title;
    QString IPString;
    quint32 IP;
    quint16 Port;
    QString Map;
    QString IWAD;
    QStringList PWADs;
    QString GameModeString;

    int GameMode;
    bool IsInstagib;
    bool IsBuckshot;

    QString Version;

    int PlayerCount;
    int BotCount;
    int SpectatorCount;

    int MaxPlayers;
    int MaxClients;

    quint32 DMFlags;
    quint32 DMFlags2;
    quint32 ZADMFlags; // former DMFlags3
    quint32 CompatFlags;
    quint32 ZACompatFlags; // former CompatFlags2

    bool ForcePassword;
    bool ForceJoinPassword;

    QVector<Player> Players;
    QVector<Team> Teams;

    bool Visible;
    int CategoryCount;

    enum State
    {
        State_NA,
        State_Banned,
        State_Flood,
        State_Good,
        State_BadPacket,
        State_Refreshing,
    };

    State LastState;

    Server()
    {
        Ping = 0;
        IP = 0;
        Port = 0;
        GameMode = 0;
        IsInstagib = false;
        IsBuckshot = false;
        PlayerCount = 0;
        SpectatorCount = 0;
        BotCount = 0;
        MaxPlayers = 0;
        MaxClients = 0;
        DMFlags = 0;
        DMFlags2 = 0;
        ZADMFlags = 0;
        CompatFlags = 0;
        ZACompatFlags = 0;
        ForcePassword = false;
        ForceJoinPassword = false;
        LastState = State_Refreshing;
        Visible = false;
        CategoryCount = 0;
    }

    bool isNA() const
    {
        /*
        if ((LastState == State_BadPacket) ||
            (LastState == State_NA) ||
            (GameModeString.isEmpty())) // basically the GameModeString check means that the server doesn't have valid info.
                return true;
        return false;*/
        return GameModeString.isEmpty();
    }

private:
};

struct ServerVisual
{
    QRect Rect;
    int Category;
    struct Server* Server; // index into servers array
};

class ServerList : public QFrame
{
    Q_OBJECT
public:
    enum
    {
        Flag_ClassicView    = 0x00000001, // use classic view (like IDE and DS, without categories)
        Flag_CVSortPlayers  = 0x00000002, // additional sort by player column (if players on top enabled)
        Flag_CVGrid         = 0x00000004, // full grid instead of horizontal only in classic view
        Flag_CVNoGrid       = 0x00000008, // no grid at all in classic view
        Flag_NoHighlight    = 0x00000010, // don't display hovered rows
        Flag_CVLanTop       = 0x00000020, // LAN servers (?) on top in classic view
        Flag_CVCustomTop    = 0x00000040, // Custom servers on top in classic view
        Flag_CVPlayersTop   = 0x00000080, // active servers on top in classic view
        Flag_UpdateOften    = 0x00000100, // enables updating every 80ms (for refresh animation)
        Flag_AnimateRefresh = 0x00000200, // enable animated refresh
        Flag_SmoothPing     = 0x00000400, // colour gradient from horrible/bad to good ping
    };

    explicit ServerList(QWidget* parent = 0);
    void setScrollData(QScrollArea* area, QWidget* viewport);

    void updateWidget();

    bool isCategoryActive(int cat)
    {
        return (cat < Filters.size()) ? Filters[cat].isEnabled() : false;
    }

    void setCategoryActive(int cat, bool active)
    {
        if (cat < Filters.size())
            Filters[cat].setEnabled(active);
        updateWidget();
    }

    QString getColumnName(int col)
    {
        static QStringList l;

        if (!l.size())
        {
            l   << " "
                << "Players"
                << "Ping"
                << " "
                << "Title"
                << "IP address"
                << "Map"
                << "IWAD"
                << "PWADs"
                << "Game mode";
        }

        if (col < 0 || col >= l.size())
            return "";
        return l[col];
    }

    QString getCategoryName(int cat)
    {
        if (cat < Filters.size())
            return Filters[cat].getName();
        return "";
    }

    void setSorting(int column, int order)
    {
        column = std::min(std::max(0, column), ServerList_WIDTH-1);
        if (order != 0)
            order = 1;

        SortingBy = column;
        SortingOrder = order;

        updateSorting();
    }

    void updateSorting(bool repaint = true, bool updatew = true)
    {
        QString addressOld = ((SelectedServer >= 0) && (SelectedServer < ServersVisual.size())) ? ServersVisual[SelectedServer].Server->IPString : "";
        serverComparatorContext = this;
        qSort(ServersVisual.begin(), ServersVisual.end(), &ServerList::serverComparator);
        if (updatew)
            updateWidget();
        if (repaint)
        {
            if (!addressOld.isEmpty())
            {
                for (int i = 0; i < ServersVisual.size(); i++)
                {
                    if (ServersVisual[i].Server->IPString == addressOld)
                    {
                        scrollToServer(i);
                        break;
                    }
                }
            }

            update();
        }
    }

    static bool serverComparator(const ServerVisual& sv1, const ServerVisual& sv2);

    void setFlag(int flag, bool value)
    {
        if (value)
            Flags |= flag;
        else Flags &= ~flag;

        if (flag & (Flag_ClassicView))
            updateSorting();
        else if ((Flags & Flag_ClassicView) &&
                 (flag & (Flag_CVCustomTop|Flag_CVLanTop|Flag_CVPlayersTop|Flag_CVSortPlayers)))
                    updateSorting();
        else if ((Flags & Flag_ClassicView) &&
                 (flag & (Flag_CVGrid|Flag_CVNoGrid)))
                    update();
        else if (flag & Flag_NoHighlight)
            update();
    }

    bool checkFlag(int flag)
    {
        return ((Flags & flag) == flag);
    }

    QVector<Server>& getServers() { return Servers; }

    void scrollToServer(int server);

    void setFilterList(QVector<ServerFilter> filters)
    {
        Filters = filters;
        ServerCatCount = Filters.size();
        CategoriesServers.resize(ServerCatCount);
        CategoriesRects.resize(ServerCatCount);
        updateWidget();
    }

    QVector<ServerFilter> getFilterList() // copying on purpose
    {
        return Filters;
    }

    int getServerCount(int cat)
    {
        if ((cat >= 0) && (cat < ServerCatCount))
            return CategoriesServers[cat];
        return Servers.size();
    }

    Server* getSelectedServer()
    {
        if ((SelectedServer < 0) || (SelectedServer >= ServersVisual.size()))
            return 0;
        return ServersVisual[SelectedServer].Server;
    }

    Server* getHoveredServer()
    {
        if ((HoveredServer < 0) || (HoveredServer >= ServersVisual.size()))
            return 0;
        return ServersVisual[HoveredServer].Server;
    }

signals:
    void updateRequested(quint32 ip, quint16 port);

public slots:
    void ISrvTimeout()
    {
        ISrvIcon++;// = (ISrvIcon+1) % 8;
        if (checkFlag(Flag_UpdateOften|Flag_AnimateRefresh))
            update();
    }

private:
    static ServerList* serverComparatorContext;

    QScrollArea* ScrollArea;
    QWidget* ScrollView;

    void recalcColumnWidths();
    int ColumnWidths[ServerList_WIDTH];

protected:
    virtual void paintEvent(QPaintEvent *);
    void paintHeader(QPainter& p, QRect vr);
    virtual void resizeEvent(QResizeEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void keyPressEvent(QKeyEvent *);

    QVector<Server> Servers;
    QVector<ServerVisual> ServersVisual;
    int ServerCatCount;
    QVector<int> CategoriesServers;//[ServerCat_COUNT];
    QVector<QRect> CategoriesRects;//[ServerCat_COUNT];
    QVector<ServerFilter> Filters;

    int HoveredServer;
    int ClickedServer;
    int SelectedServer;

    int HoveredCategory;
    int ClickedCategory;

    int HoveredHeader;
    int ClickedHeader;

    int SortingBy;
    int SortingOrder; // 0 = asc, 1 = desc

    int Flags;

    friend class ServerScroller;

    // data
    QPixmap ISrv_Good;
    QPixmap ISrv_Bad;
    QPixmap ISrv_UtterShit;
    QPixmap ISrv_Banned;
    QPixmap ISrv_Flood;
    QPixmap ISrv_Na;
    QPixmap ISrv_Refresh[8];

    QPixmap IPl_Player;
    QPixmap IPl_Bot;
    QPixmap IPl_Spectator;
    QPixmap IPl_Free;
    QPixmap IPl_Empty;

    // helpers
    QTimer ISrvTimer;
    int ISrvIcon;
};

#endif // SERVERLIST_H
