#include "fancyprogressbar.h"
#include <QStyle>
#include <QStylePainter>

FancyProgressBar::FancyProgressBar(QWidget *parent) :
    QProgressBar(parent)
{
    ShowProgressText = true;
}

void FancyProgressBar::paintEvent(QPaintEvent *e)
{
    // make sure that we are using the windows style
    if (QString(style()->metaObject()->className()) != "QWindowsStyle")
    {
        QProgressBar::paintEvent(e);
        return;
    }

    // otherwise, paint the progressbar with own style
    // draw the panel
    QPainter p(this);
    QPalette pal = palette();
    QRect gr = rect();

    p.fillRect(gr, pal.background().color());

    p.setPen(pal.dark().color());
    p.drawLine(gr.left(), gr.top(), gr.right(), gr.top());
    p.drawLine(gr.left(), gr.top(), gr.left(), gr.bottom());

    p.setPen(pal.light().color());
    p.drawLine(gr.right(), gr.top(), gr.right(), gr.bottom());
    p.drawLine(gr.left(), gr.bottom(), gr.right(), gr.bottom());

    p.setPen(Qt::NoPen);

    if (!Ranges.size())
    {
        int progMax = (maximum()-minimum());
        int progCur = (value()-minimum());

        QRect dr = gr;
        dr.setTop(dr.top()+2);
        dr.setBottom(dr.bottom()-2);
        dr.setLeft(dr.left()+2);
        dr.setRight(dr.right()-2);
        dr.setWidth(dr.width()*progCur/progMax);

        p.fillRect(dr, pal.highlight().color());

        if (ShowProgressText)
        {
            dr.setTop(gr.top()+2);
            dr.setBottom(gr.bottom()-2);
            dr.setLeft(gr.left()+2);
            dr.setRight(gr.right()-2);
            QRect ctr = dr;

            dr.setWidth(dr.width()*progCur/progMax);
            p.setClipRect(dr);

            QTextOption o;
            o.setAlignment(Qt::AlignCenter);
            QFont fnt = p.font();
            fnt.setBold(true);
            p.setFont(fnt);

            QString progtext = QString("%1").arg(QString::number(100*progCur/progMax))+"%";

            p.setPen(pal.light().color());
            p.drawText(ctr, progtext, o);

            dr.setLeft(gr.left()+2);
            dr.setRight(gr.right()-2);
            dr.setLeft(dr.left()+dr.width()*progCur/progMax);
            p.setClipRect(dr);

            p.setPen(pal.text().color());
            p.drawText(ctr, progtext, o);
        }
    }
    else
    {
        int progMax2 = (maximum()-minimum());
        for (int i = 0; i < Ranges.size(); i++)
        {
            int iminF = Ranges[i].start;
            if (iminF > minimum() && !i) iminF = minimum();
            int iminFend = (i+1<Ranges.size())?Ranges[i+1].start:maximum();

            int progMax = (iminFend-iminF);
            int progCur = (value()-iminF);
            if (progCur < 0)
                break;
            if (progCur > progMax)
                progCur = progMax;

            QRect dr = gr;
            dr.setLeft(dr.left()+2);
            dr.setRight(dr.right()-2);
            dr.setTop(dr.top()+2);
            dr.setBottom(dr.bottom()-2);
            int o1 = dr.width()*(iminF-minimum())/progMax2;
            int o2 = dr.width()*(iminF-minimum()+progCur)/progMax2;
            dr.setRight(dr.left()+o2-1);
            dr.setLeft(dr.left()+o1);

            QLinearGradient qlg(dr.topLeft(), dr.bottomLeft());
            QColor c = Ranges[i].color;
            QColor c2 = c;
            c2.setHsl(c2.hslHue(), c2.hslSaturation(), c2.lightness()/3);
            qlg.setColorAt(0.0, c2);
            qlg.setColorAt(0.5, c);
            qlg.setColorAt(1.0, c2);
            p.fillRect(dr, qlg);
        }
    }
}
