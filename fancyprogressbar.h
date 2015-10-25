#ifndef FANCYPROGRESSBAR_H
#define FANCYPROGRESSBAR_H

#include <QProgressBar>
#include <QColor>
#include <QVector>

class FancyProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit FancyProgressBar(QWidget *parent = 0);

    void showProgressText(bool spt)
    {
        ShowProgressText = spt;
    }

    bool isShowingProgressText() const { return ShowProgressText; }

    void clearRanges()
    {
        Ranges.clear();
    }

    void setRange(int start, QColor color)
    {
        FancyProgressBarRange br;
        br.start = start;
        br.color = color;

        bool ins = true;
        int istart = Ranges.size();
        for (int i = 0; i < Ranges.size(); i++)
        {
            // if range's start equals to this start, replace it
            if (Ranges[i].start == br.start)
            {
                ins = false;
                istart = i;
                break;
            }

            if (Ranges[i].start > br.start)
            {
                if (i < istart)
                    istart = i;
            }
        }

        if (ins)
            Ranges.insert(Ranges.begin()+istart, br);
        Ranges[istart] = br;
    }

signals:

public slots:

protected:
    virtual void paintEvent(QPaintEvent *e);

private:
    struct FancyProgressBarRange
    {
        int start;
        QColor color;
    };

    QVector<FancyProgressBarRange> Ranges;
    bool ShowProgressText;
};

#endif // FANCYPROGRESSBAR_H
