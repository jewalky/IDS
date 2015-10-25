#include "serverscroller.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QScrollBar>

#include "mainwindow.h"

ServerScroller::ServerScroller(QWidget *parent) :
    QScrollArea(parent)
{
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onVertChanged(int)));
    oldValue = 0;
}

void ServerScroller::onVertChanged(int nv)
{
    // also force the update of the new header position
    //qDebug("nv = %d; ov = %d", nv, oldValue);
    // apparently this doesn't work either. meh, double repaint.
    serverList->repaint();

    oldValue = nv;
}
