#ifndef SERVERSCROLLER_H
#define SERVERSCROLLER_H

#include <QScrollArea>
#include "serverlist.h"

class ServerScroller : public QScrollArea
{
    Q_OBJECT
public:
    explicit ServerScroller(QWidget *parent = 0);

    void setServerList(ServerList* sl) { serverList = sl; }
    ServerList* getServerList() const { return serverList; }

signals:

public slots:
    void onVertChanged(int nv);

private:
    ServerList* serverList;
    int oldValue;

protected:

};

#endif // SERVERSCROLLER_H
