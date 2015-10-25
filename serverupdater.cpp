#include "serverupdater.h"
#include <QtNetwork/QHostInfo>
#include <QDataStream>
#include <QDateTime>

#ifdef WIN32
#include <winsock2.h>
#endif

ServerUpdater::ServerUpdater(QObject *parent, ServerList *servers, quint16 localPort) :
    QObject(parent), Servers(servers), LocalPort(localPort), Socket(this)
{
    Timer.setInterval(1);
    Timer.start();

    MasterIP = 0;
    MasterPort = 0;

    State = State_Inactive;

    Timeout = 5000;

    TimeoutTime = 0;
    DatagramTime = 0;

    connect(&Socket, SIGNAL(readyRead()), this, SLOT(socketReady()));
    connect(&Timer, SIGNAL(timeout()), this, SLOT(timerTick()));
}

bool ServerUpdater::scheduleUpdateAll(QString masterAddress)
{
    QStringList qslP = masterAddress.split(":");
    if (qslP.size() > 2)
        return false;
    int port = 10666;
    bool port_ok = true;
    if (qslP.size() == 2)
        port = qslP[1].toInt(&port_ok);
    if (!port_ok || (port < 0) || (port > 65535))
        return false;
    QHostInfo hi = QHostInfo::fromName(qslP[0]);
    QList<QHostAddress> hia = hi.addresses();
    if (hia.isEmpty())
        return false;
    quint32 host = 0;
    for (int i = 0; i < hia.size(); i++)
    {
        if (hia[i].protocol() == QAbstractSocket::IPv4Protocol)
        {
            host = hia[i].toIPv4Address();
            break;
        }
    }
    if (!host) return false;

    scheduleUpdateAll(host, port);
    return true;
}


bool ServerUpdater::scheduleUpdateServer(QString serverAddress)
{
    QStringList qslP = serverAddress.split(":");
    if (qslP.size() > 2)
        return false;
    int port = 10666;
    bool port_ok = true;
    if (qslP.size() == 2)
        port = qslP[1].toInt(&port_ok);
    if (!port_ok || (port < 0) || (port > 65535))
        return false;
    QHostInfo hi = QHostInfo::fromName(qslP[0]);
    QList<QHostAddress> hia = hi.addresses();
    if (hia.isEmpty())
        return false;
    quint32 host = 0;
    for (int i = 0; i < hia.size(); i++)
    {
        if (hia[i].protocol() == QAbstractSocket::IPv4Protocol)
        {
            host = hia[i].toIPv4Address();
            break;
        }
    }
    if (!host) return false;

    scheduleUpdateServer(host, port);
    return true;
}

void ServerUpdater::scheduleUpdateAll(quint32 ip, quint16 port)
{
    ServerUpdaterAction ac;
    ac.Action = ServerUpdaterAction::SU_UpdateAll;
    ac.IP = ip;
    ac.Port = port;
    Actions.append(ac);
}

void ServerUpdater::scheduleUpdateServer(quint32 ip, quint16 port)
{
    ServerUpdaterAction ac;
    ac.Action = ServerUpdaterAction::SU_UpdateSingle;
    ac.IP = ip;
    ac.Port = port;
    Actions.append(ac);
}

void ServerUpdater::scheduleUpdateVisible()
{
    ServerUpdaterAction ac;
    ac.Action = ServerUpdaterAction::SU_UpdateVisible;
    ac.IP = 0;
    ac.Port = 0;
    Actions.append(ac);
}


// THESE FLAGS ONLY EXIST IN THIS FILE, SO NOT IN HEADER
#define SQF_NAME                	0x00000001
#define SQF_URL                 	0x00000002
#define SQF_EMAIL                  	0x00000004
#define SQF_MAPNAME                 0x00000008
#define SQF_MAXCLIENTS				0x00000010
#define SQF_MAXPLAYERS				0x00000020
#define SQF_PWADS               	0x00000040
#define SQF_GAMETYPE				0x00000080
#define SQF_GAMENAME				0x00000100
#define SQF_IWAD                	0x00000200
#define SQF_FORCEPASSWORD			0x00000400
#define SQF_FORCEJOINPASSWORD	    0x00000800
#define SQF_GAMESKILL				0x00001000
#define SQF_BOTSKILL				0x00002000
#define SQF_DMFLAGS             	0x00004000 // Deprecated
#define SQF_LIMITS              	0x00010000
#define SQF_TEAMDAMAGE				0x00020000
#define SQF_TEAMSCORES				0x00040000 // Deprecated
#define SQF_NUMPLAYERS				0x00080000
#define SQF_PLAYERDATA				0x00100000
#define SQF_TEAMINFO_NUMBER			0x00200000
#define SQF_TEAMINFO_NAME			0x00400000
#define SQF_TEAMINFO_COLOR			0x00800000
#define SQF_TEAMINFO_SCORE			0x01000000
#define SQF_TESTING_SERVER			0x02000000
#define SQF_DATA_MD5SUM				0x04000000
#define SQF_ALL_DMFLAGS				0x08000000
#define SQF_SECURITY_SETTINGS       0x10000000
// END OF FLAGS


void ServerUpdater::abortUpdates()
{
    State = State_Inactive;
    Servers->updateSorting();
    emit failed(Failed_Timeout);
}

void ServerUpdater::timerTick()
{
    if (!Servers)
    {
        Actions.clear();
        return;
    }

    if (TimeoutTime != 0 && (QDateTime::currentMSecsSinceEpoch() >= TimeoutTime))
    {
        if (State == State_WaitingServers)
        {
            QVector<Server>& servers = Servers->getServers();

            for (int i = 0; i < servers.size(); i++)
            {
                if (servers[i].LastState == Server::State_Refreshing)
                {
                    for (int j = 0; j < OldServers.size(); j++)
                    {
                        if (OldServers[j].IP == servers[i].IP &&
                                OldServers[j].Port == servers[i].Port)
                        {
                            servers[i] = OldServers[j];
                            break;
                        }
                    }

                    servers[i].LastState = Server::State_NA;
                    servers[i].Ping = 0;
                }
            }
        }

        abortUpdates();
        TimeoutTime = 0;
        DatagramTime = 0;
        qDebug("ServerUpdater: Timed out.");
    }

    if (State == State_Inactive)
    {
        if (Actions.size())
        {
            ServerUpdaterAction ac = Actions[0];
            Actions.removeFirst();
            if (ac.Action == ServerUpdaterAction::SU_UpdateAll)
            {
                MasterIP = ac.IP;
                MasterPort = ac.Port;
                State = State_WaitingMaster;
                OldServers = Servers->getServers();
                // connect to the master here
                QByteArray p;
                QDataStream ds(&p, QIODevice::WriteOnly);
                ds.setByteOrder(QDataStream::LittleEndian);
                ds << quint32(5660028); // master challenge
                ds << quint16(2);
                EncodePacket(p);
                // close old socket. packet flood is a bad thing, and sometimes it fucks up forever.
                Socket.close();
                if ((Socket.state() != QAbstractSocket::BoundState) && !Socket.bind(0, QUdpSocket::DontShareAddress))
                {
                    qDebug("Failed to bind the listener socket.");
                    State = State_Inactive;
                    return;
                }

                qDebug("Started master update from %s:%d.", QHostAddress(MasterIP).toString().toUtf8().data(), MasterPort);
                Socket.writeDatagram(p, QHostAddress(MasterIP), MasterPort);
                TimeoutTime = QDateTime::currentMSecsSinceEpoch()+Timeout;
                ExpectingServers = 0;
                ReceivedServers = 0;
                emit started();
            }
            else if (ac.Action == ServerUpdaterAction::SU_UpdateVisible)
            {
                qDebug("Started visible update.");

                MasterIP = 0;
                MasterPort = 0;
                State = State_WaitingServers;
                OldServers = Servers->getServers();
                QVector<Server>& servers = Servers->getServers();
                QVector<Server> serversFake;
                for (int i = 0; i < servers.size(); i++)
                {
                    if (servers[i].Visible)
                    {
                        serversFake.append(servers[i]);
                        servers[i].LastState = Server::State_Refreshing;
                    }
                }

                Servers->updateWidget();
                Servers->update();

                if (!serversFake.isEmpty())
                {
                    Socket.close();
                    if ((Socket.state() != QAbstractSocket::BoundState) && !Socket.bind(0, QUdpSocket::DontShareAddress))
                    {
                        qDebug("Failed to bind the listener socket.");
                        State = State_Inactive;
                        return;
                    }

                    ServerUpdaterSenderThread* senderThread = new ServerUpdaterSenderThread(this, Socket, serversFake, Timeout);
                    connect(senderThread, SIGNAL(finished()), senderThread, SLOT(deleteLater()));
                    senderThread->start();
                    int delaybetween = std::max(2u, Timeout/1000);
                    TimeoutTime = QDateTime::currentMSecsSinceEpoch()+Timeout+delaybetween*serversFake.size();
                    ExpectingServers = serversFake.size();
                    ReceivedServers = 0;
                    emit started();
                }
                else
                {
                    State = State_Inactive;
                }
            }
            else if (ac.Action == ServerUpdaterAction::SU_UpdateSingle)
            {
                // send the request to a particular server
                MasterIP = 0;
                MasterPort = 0;
                State = State_WaitingServers;
                OldServers = Servers->getServers();
                // connect to the server here
                QVector<Server> serversFake;
                Server serverFake;
                serverFake.IP = ac.IP;
                serverFake.Port = ac.Port;
                serversFake.append(serverFake);

                qDebug("Started single update for %s:%d.", QHostAddress(ac.IP).toString().toUtf8().data(), ac.Port);

                Socket.close();
                if ((Socket.state() != QAbstractSocket::BoundState) && !Socket.bind(0, QUdpSocket::DontShareAddress))
                {
                    qDebug("Failed to bind the listener socket.");
                    State = State_Inactive;
                    return;
                }

                QVector<Server>& servers = Servers->getServers();
                for (int i = 0; i < servers.size(); i++)
                {
                    if ((servers[i].IP == ac.IP) && servers[i].Port == ac.Port)
                    {
                        servers[i].LastState = Server::State_Refreshing;
                        Servers->updateWidget();
                        Servers->update();
                        break;
                    }
                }

                ServerUpdaterSenderThread* senderThread = new ServerUpdaterSenderThread(this, Socket, serversFake, Timeout);
                connect(senderThread, SIGNAL(finished()), senderThread, SLOT(deleteLater()));
                senderThread->start();
                TimeoutTime = QDateTime::currentMSecsSinceEpoch()+Timeout;
                ExpectingServers = 1;
                ReceivedServers = 0;
                emit started();
            }
        }
    }
    else // not inactive
    {
        if (DatagramTime == 0)
            DatagramTime = QDateTime::currentMSecsSinceEpoch();
    }

    if (!DatagramBuffer.isEmpty() && ((QDateTime::currentMSecsSinceEpoch()-DatagramTime > Timeout/8) || (State != State_WaitingServers)))
    {
        DatagramTime = QDateTime::currentMSecsSinceEpoch();

        qDebug("%d datagrams", DatagramBuffer.size());

        for (int i = 0; i < DatagramBuffer.size(); i++)
        {
            QByteArray& p = DatagramBuffer[i].Datagram;
            DecodePacket(p);
            QDataStream ds(p);
            ds.setByteOrder(QDataStream::LittleEndian);
            parseDatagram(ds, DatagramBuffer[i].Host, DatagramBuffer[i].Port, DatagramBuffer[i].ReceivedTime);
        }

        DatagramBuffer.clear();

        if ((ReceivedServers >= ExpectingServers) && (ExpectingServers > 0))
        {
            State = State_Inactive;
            MasterIP = 0;
            MasterPort = 0;
            Servers->updateSorting();
            TimeoutTime = 0;
            DatagramTime = 0;
            qDebug("ServerUpdater: Server update finished.");
            emit succeeded();
        }
    }
}

void ServerUpdater::socketReady()
{
#ifdef WIN32
    // Basically, Windows doesn't handle receiving 600+ UDP packets at the same second properly.
    // For it to not drop servers randomly, the recv buffer needs to be enlarged.
    // AFAIK this isn't a problem on Linux.
    static bool WSAFail = false;
    int v = -1;
    if (!WSAFail && (::setsockopt(Socket.socketDescriptor(), SOL_SOCKET, SO_RCVBUF, (char*)&v, sizeof(v)) < 0))
    {
        qDebug("ServerUpdater: Win32: Failed to increase the buffer limit; WSAGetLastError() = %d", WSAGetLastError());
        WSAFail = true;
    }
#endif

    while (Socket.hasPendingDatagrams())
    {
        char pData[40960];
        QHostAddress pHost;
        quint16 pPort;
        int realLen = Socket.readDatagram(pData, sizeof(pData), &pHost, &pPort);
        QByteArray p(pData, realLen);
        ReceivedDatagram rd;
        rd.ReceivedTime = QDateTime::currentMSecsSinceEpoch();
        rd.Datagram = p;
        rd.Host = pHost;
        rd.Port = pPort;
        DatagramBuffer.append(rd);
    }
}

ServerUpdaterSenderThread::ServerUpdaterSenderThread(QObject* object, QUdpSocket &socket, QVector<Server> servers, quint32 timeout) :
    QThread(object), Socket(socket), Servers(servers), Timeout(timeout)
{
    // hi
}

// we are doing this in a separate thread, because the SENDING is laggy (laggy enough to introduce a +1 second delay to servers' ping)
void ServerUpdaterSenderThread::run()
{
    if (Socket.state() != QAbstractSocket::BoundState)
        return;

#ifdef WIN32
    // same as for RCVBUF
    static bool WSAFail = false;
    int v = -1;
    if (!WSAFail && (::setsockopt(Socket.socketDescriptor(), SOL_SOCKET, SO_SNDBUF, (char*)&v, sizeof(v)) < 0))
    {
        qDebug("ServerUpdater: Win32: Failed to increase the buffer limit for sending; WSAGetLastError() = %d", WSAGetLastError());
        WSAFail = true;
    }
#endif

    for (int i = 0; i < Servers.size(); i++)
    {
        QThread::msleep(std::max(2u, Timeout/1000));

        QByteArray p;
        QDataStream ds(&p, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds << quint32(199);
        ds << quint32(SQF_NAME|
                      SQF_MAPNAME|
                      SQF_GAMETYPE|
                      SQF_IWAD|
                      SQF_ALL_DMFLAGS|
                      SQF_FORCEPASSWORD|SQF_FORCEJOINPASSWORD|
                      SQF_PLAYERDATA|SQF_NUMPLAYERS|SQF_MAXCLIENTS|SQF_MAXPLAYERS);
        ds << quint32(QDateTime::currentMSecsSinceEpoch());
        EncodePacket(p);

        // try to wait for when we actually CAN WRITE SOMETHING.
        // waitForBytesWritten will NOT work because we are using the socket from another thread.
        // will wait at least 1ms
        Socket.writeDatagram(p, QHostAddress(Servers[i].IP), Servers[i].Port);
    }
}

static QString GetStringFromStream(QDataStream& ds)
{
    QByteArray a;
    while (true)
    {
        qint8 c;
        ds >> c;
        if (!c) break;
        a.append(c);
    }

    return QString::fromUtf8(a);
}

void ServerUpdater::parseDatagram(QDataStream& ds, QHostAddress host, quint16 port, qint64 timestamp)
{
    static QVector<int> ReceivedMasterPck;
    static int ReceivedMasterPckLast = 0;
    static bool ReceivedMasterPckMarked = false;
    static qint64 ServersReceived = QDateTime::currentMSecsSinceEpoch();

    if ((host.toIPv4Address() == MasterIP) && (port == MasterPort))
    {
        if (State == State_WaitingMaster)
        {
            quint32 response;
            ds >> response;
            if (response == 6) // valid
            {
                Servers->getServers().clear();
                ds.device()->seek(0);
                State = State_WaitingMasterList;
                ReceivedMasterPckLast = 0;
                ReceivedMasterPckMarked = false;
                ReceivedMasterPck.clear();
                ServersReceived = QDateTime::currentMSecsSinceEpoch();
                qDebug("ServerUpdater: Accepted. Proceeding to the server IP list.");
            }
            else
            {
                if (response == 3)
                {
                    qDebug("ServerUpdater: Denied; your IP is banned (MSC_IPISBANNED).");
                    State = State_Inactive;
                    emit failed(Failed_Banned);
                }
                else if (response == 4)
                {
                    qDebug("ServerUpdater: Denied; your IP has made a request in the past 3 seconds (MSC_REQUESTIGNORED).");
                    State = State_Inactive;
                    emit failed(Failed_Flood);
                }
                else if (response == 5)
                {
                    qDebug("ServerUpdater: Denied; you're using an older version of the master protocol (MSC_WRONGVERSION).");
                    State = State_Inactive;
                    emit failed(Failed_BadVersion);
                }
                else
                {
                    qDebug("ServerUpdater: Unknown packet received from the master (header %d).", response);
                    State = State_Inactive;
                    emit failed(Failed_BadPacket);
                }

                return;
            }
        }

        if (State == State_WaitingMasterList)
        {
            quint32 response;
            ds >> response;
            if (response != 6)
            {
                qDebug("ServerUpdater: Unexpected master packet begins with %d!", response);
                State = State_Inactive;
                emit failed(Failed_BadPacket);
            }

            quint8 p_num, p_sig;
            ds >> p_num >> p_sig;
            if (p_sig != 8)
            {
                qDebug("ServerUpdater: Bad master packet, server list begins with %d!", p_sig);
                State = State_Inactive;
                emit failed(Failed_BadPacket);
            }

            while (true)
            {
                quint8 p_numservers;
                ds >> p_numservers;
                if (!p_numservers)
                    break;

                quint8 p_o1, p_o2, p_o3, p_o4;
                ds >> p_o1 >> p_o2 >> p_o3 >> p_o4;

                quint32 ip = p_o1;
                ip <<= 8;
                ip |= p_o2;
                ip <<= 8;
                ip |= p_o3;
                ip <<= 8;
                ip |= p_o4;

                for (quint8 i = 0; i < p_numservers; i++)
                {
                    quint16 p_port;
                    ds >> p_port;
                    Server s;
                    s.IP = ip;
                    s.Port = p_port;
                    s.IPString = QString("%1.%2.%3.%4:%5").arg(QString::number(p_o1),
                                                               QString::number(p_o2),
                                                               QString::number(p_o3),
                                                               QString::number(p_o4),
                                                               QString::number(p_port));
                    Servers->getServers().append(s);
                }

            }

            qDebug("ServerUpdater: Received %d servers total...", Servers->getServers().size());
            Servers->updateSorting();

            quint8 p_end = 2;
            ds >> p_end;

            if (p_end == 2)
                ReceivedMasterPckMarked = true;

            qDebug("p_num = %d; p_end = %d", p_num, p_end);

            bool finished = true;
            ReceivedMasterPck.append(p_num);
            ReceivedMasterPckLast = 0;
            for (int i = 0; i < ReceivedMasterPck.size(); i++)
            {
                if (ReceivedMasterPck[i] > ReceivedMasterPckLast)
                    ReceivedMasterPckLast = ReceivedMasterPck[i];
            }

            if (ReceivedMasterPckMarked)
            {
                for (int i = 0; i < ReceivedMasterPckLast; i++)
                {
                    bool found = false;
                    for (int j = 0; j < ReceivedMasterPck.size(); j++)
                    {
                        if (ReceivedMasterPck[j] == i)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        qDebug("%d not found", i);
                        finished = false;
                        break;
                    }
                }
            }
            else
            {
                qDebug("ReceivedMasterPckMarked = false");
                finished = false;
            }

            if (finished)
            {
                State = State_WaitingServers;
                MasterIP = 0;
                MasterPort = 0;
                emit masterSucceeded();
                qDebug("ServerUpdater: Update finished, proceeding with individual server updates...");
                ServerUpdaterSenderThread* senderThread = new ServerUpdaterSenderThread(this, Socket, Servers->getServers(), Timeout);
                connect(senderThread, SIGNAL(finished()), senderThread, SLOT(deleteLater()));
                senderThread->start();
                ExpectingServers = Servers->getServers().size();
                int delaybetween = std::max(2u, Timeout/1000);
                TimeoutTime = QDateTime::currentMSecsSinceEpoch()+Timeout+delaybetween*Servers->getServers().size();
            }

            return;
        }
    }
    else if (State == State_WaitingServers)
    {
        quint32 response;
        ds >> response;

        if ((response != 5660023) &&
            (response != 5660024) &&
            (response != 5660025))
        {
            qDebug("ServerUpdater: Received odd packet from %s:%d (begins with %d)", host.toString().toUtf8().data(), port, response);
            return;
        }

        QVector<Server>& servers = Servers->getServers();
        Server* srv = 0;
        for (int i = 0; i < servers.size(); i++)
        {
            if ((servers[i].IP == host.toIPv4Address()) &&
                (servers[i].Port == port))
            {
                srv = &servers[i];
                break;
            }
        }

        if (!srv)
        {
            Server srvm;
            srvm.IP = host.toIPv4Address();
            srvm.Port = port;
            srvm.IPString = QString("%1.%2.%3.%4:%5").arg(QString::number((srvm.IP>>24)&0xFF),
                                                          QString::number((srvm.IP>>16)&0xFF),
                                                          QString::number((srvm.IP>>8)&0xFF),
                                                          QString::number(srvm.IP&0xFF),
                                                          QString::number(srvm.Port));
            servers.append(srvm);
            srv = &servers[servers.size()-1];
        }

        ReceivedServers++;

        //quint32 tC = QDateTime::currentMSecsSinceEpoch();
        quint32 tC = (quint32)timestamp;
        quint32 tP;
        ds >> tP;
        srv->Ping = tC-tP;
        if (srv->Ping <= 0)
            srv->Ping = 1; // for the list to not show "n/a" for valid localhost servers

        if (response == 5660024)
        {
            int oldPing = srv->Ping;
            for (int i = 0; i < OldServers.size(); i++)
            {
                if (OldServers[i].IP == srv->IP &&
                        OldServers[i].Port == srv->Port)
                {
                    *srv = OldServers[i];
                    break;
                }
            }
            srv->Ping = oldPing;
            srv->LastState = Server::State_Flood;
            return;
        }
        else if (response == 5660025)
        {
            int oldPing = srv->Ping;
            for (int i = 0; i < OldServers.size(); i++)
            {
                if (OldServers[i].IP == srv->IP &&
                        OldServers[i].Port == srv->Port)
                {
                    *srv = OldServers[i];
                    break;
                }
            }
            srv->Ping = oldPing;
            srv->LastState = Server::State_Banned;
            return;
        }

        srv->LastState = Server::State_Good;

        srv->Version = GetStringFromStream(ds);
        quint32 flags;
        ds >> flags;

        if (flags & SQF_NAME)
        {
            srv->Title = GetStringFromStream(ds);
        }

        if (flags & SQF_MAPNAME)
        {
            srv->Map = GetStringFromStream(ds);
        }

        if (flags & SQF_MAXCLIENTS)
        {
            quint8 p_maxclients;
            ds >> p_maxclients;
            srv->MaxClients = p_maxclients;
        }

        if (flags & SQF_MAXPLAYERS)
        {
            quint8 p_maxplayers;
            ds >> p_maxplayers;
            srv->MaxPlayers = p_maxplayers;
        }

        if (flags & SQF_GAMETYPE)
        {
            quint8 p_gametype;
            quint8 p_instagib;
            quint8 p_buckshot;
            ds >> p_gametype >> p_instagib >> p_buckshot;
            srv->IsInstagib = !!p_instagib;
            srv->IsBuckshot = !!p_buckshot;
            srv->GameMode = p_gametype;
            QString gs = QString("Unknown (%1)").arg(QString::number(srv->GameMode));
            static QString GameModeStrings[] =
            {
                "Cooperative",
                "Survival",
                "Invasion",
                "Deathmatch",
                "TeamDM",
                "Duel",
                "Terminator",
                "LMS",
                "TeamLMS",
                "Possession",
                "TeamPossession",
                "TeamGame",
                "CTF",
                "OneFlagCTF",
                "Skulltag",
                "Domination"
            };

            if (srv->GameMode < 16)
                gs = GameModeStrings[srv->GameMode];
            srv->GameModeString = gs;
        }

        if (flags & SQF_IWAD)
        {
            srv->IWAD = GetStringFromStream(ds);
        }

        if (flags & SQF_FORCEPASSWORD)
        {
            quint8 p_forcepassword;
            ds >> p_forcepassword;
            srv->ForcePassword = !!p_forcepassword;
        }

        if (flags & SQF_FORCEJOINPASSWORD)
        {
            quint8 p_forcejoinpassword;
            ds >> p_forcejoinpassword;
            srv->ForceJoinPassword = !!p_forcejoinpassword;
        }

        if (flags & SQF_NUMPLAYERS)
        {
            quint8 p_numplayers;
            ds >> p_numplayers;
            srv->PlayerCount = p_numplayers;
        }

        if (flags & SQF_PLAYERDATA)
        {
            for (int i = 0; i < srv->PlayerCount; i++)
            {
                QString pp_name = GetStringFromStream(ds);
                qint16 pp_points, pp_ping;
                quint8 pp_spectator, pp_bot, pp_time;
                qint8 pp_team;
                ds >> pp_points >> pp_ping;
                ds >> pp_spectator >> pp_bot;
                Player p;
                p.Name = pp_name;
                p.NameClean = "";

                bool inEscape = false;
                bool inSeq = false;
                for (int i = 0; i < pp_name.size(); i++)
                {
                    // \034 is the \c thing
                    if (!inEscape && (pp_name[i] == '\034'))
                        inEscape = true;
                    else if (inEscape)
                    {
                        if (inSeq)
                        {
                            if (pp_name[i] == ']')
                            {
                                inEscape = false;
                                inSeq = false;
                            }
                        }
                        else if (pp_name[i] == '[')
                            inSeq = true;
                        else inEscape = false;
                    }
                    else p.NameClean += pp_name[i];
                }

                //qDebug("player = %s", p.NameClean.toUtf8().data());

                p.Points = pp_points;
                p.Ping = pp_ping;

                if ((srv->GameMode == Server::GAMEMODE_CTF) ||
                    (srv->GameMode == Server::GAMEMODE_SKULLTAG) ||
                    (srv->GameMode == Server::GAMEMODE_TEAMGAME) ||
                    (srv->GameMode == Server::GAMEMODE_TEAMLMS) ||
                    (srv->GameMode == Server::GAMEMODE_TEAMPLAY) ||
                    (srv->GameMode == Server::GAMEMODE_TEAMPOSSESSION) ||
                    (srv->GameMode == Server::GAMEMODE_DOMINATION))
                {
                    ds >> pp_team;
                    p.Team = pp_team;
                }
                else
                {
                    p.Team = -1;
                }

                ds >> pp_time;
                p.IsSpectator = !!pp_spectator;
                p.IsBot = !!pp_bot;
                p.Time = pp_time;
                srv->Players.append(p);

                if (p.IsSpectator)
                    srv->SpectatorCount++;
                if (p.IsBot)
                    srv->BotCount++;
            }
        }

        if (flags & SQF_ALL_DMFLAGS)
        {
            quint8 p_dmflaglen;
            ds >> p_dmflaglen;
            if (p_dmflaglen != 5)
            {
                qDebug("ServerUpdater: %s: Unknown count of DMFlags (%d).", srv->IPString.toUtf8().data(), p_dmflaglen);
                //srv->LastState = Server::State_BadPacket;
                //return;
            }

            ds >> srv->DMFlags;
            ds >> srv->DMFlags2;
            ds >> srv->ZADMFlags;
            ds >> srv->CompatFlags;
            ds >> srv->ZACompatFlags;
        }

        if ((QDateTime::currentMSecsSinceEpoch()-ServersReceived) > 100)
        {
            ServersReceived = QDateTime::currentMSecsSinceEpoch();
            Servers->updateSorting(true);
        }
        else Servers->updateSorting(false);
    }
}

#include "huffman.h"
static bool hhuff = false;

void DecodePacket(QByteArray& array)
{
    if (!hhuff)
    {
        HUFFMAN_Construct();
        hhuff = true;
    }

    unsigned char out[40960];
    int outlen;
    HUFFMAN_Decode((unsigned char*)array.data(), out, array.size(), &outlen);
    array.setRawData((char*)out, outlen);
}

void EncodePacket(QByteArray& array)
{
    if (!hhuff)
    {
        HUFFMAN_Construct();
        hhuff = true;
    }

    unsigned char out[40960];
    int outlen;
    HUFFMAN_Encode((unsigned char*)array.data(), out, array.size(), &outlen);
    array.setRawData((char*)out, outlen);
}
