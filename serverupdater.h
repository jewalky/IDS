#ifndef SERVERUPDATER_H
#define SERVERUPDATER_H

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include "serverlist.h"

class ServerUpdaterSenderThread : public QThread
{
public:
    ServerUpdaterSenderThread(QObject* object, QUdpSocket& socket, QMutex& mutex, QVector<Server> servers, quint32 timeout);//np with duplicating the list here
    virtual void run();

private:
    QUdpSocket& Socket;
    QMutex& SocketMutex;
    QVector<Server> Servers;
    quint32 Timeout;
};

struct ServerUpdaterAction
{
    enum Action
    {
        SU_UpdateAll,
        SU_UpdateVisible,
        SU_UpdateSingle
    };

    enum Action Action;
    quint32 IP; // master IP, or server IP
    quint16 Port; // same as above
};

class ServerUpdater : public QObject
{
    Q_OBJECT
public:
    explicit ServerUpdater(QObject *parent = 0, ServerList* servers = 0, quint16 localPort = 9164);

    void setTimeout(quint32 timeout) { Timeout = timeout; }
    quint32 getTimeout() { return Timeout; }

    bool scheduleUpdateAll(QString masterAddress = "master.zandronum.com:15300");
    void scheduleUpdateAll(quint32 ip, quint16 port);
    bool scheduleUpdateServer(QString serverAddress);
    void scheduleUpdateServer(quint32 ip, quint16 port);
    void scheduleUpdateVisible();
    void abortUpdates();

    bool isActive() { return (State != State_Inactive); }

    enum
    {
        Failed_Banned,
        Failed_Flood,
        Failed_BadVersion,
        Failed_BadPacket,
        Failed_Aborted,
        Failed_Timeout,
    };

signals:
    void started();
    void masterSucceeded(); // this is only relevant when UpdateAll is used
    void succeeded();
    void failed(int reason);

public slots:
    void timerTick();
    void socketReady();

private:
    void parseDatagram(QDataStream& ds, QHostAddress host, quint16 port, qint64 timestamp);

    quint32 Timeout;
    QUdpSocket Socket;
    QMutex SocketMutex;
    QVector<ServerUpdaterAction> Actions;
    QTimer Timer;
    ServerList* Servers;
    QVector<Server> OldServers;

    struct ReceivedDatagram
    {
        QHostAddress Host;
        quint16 Port;
        qint64 ReceivedTime;
        QByteArray Datagram;
    };

    QVector<ReceivedDatagram> DatagramBuffer;
    qint64 DatagramTime;

    enum UpdaterState
    {
        State_Inactive,
        State_WaitingMaster,
        State_WaitingMasterList,
        State_WaitingServers
    };

    quint32 MasterIP;
    quint16 MasterPort;

    UpdaterState State;

    quint16 LocalPort;

    qint64 TimeoutTime;
    int ExpectingServers;
    int ReceivedServers;
};

void DecodePacket(QByteArray& array);
void EncodePacket(QByteArray& array);

#endif // SERVERUPDATER_H
