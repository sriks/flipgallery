#ifndef FLIPBOOKEFFECT_H
#define FLIPBOOKEFFECT_H

#include <QObject>

class FlipbookEffectPrivate;
class FlipbookEffect : public QObject
{
    Q_OBJECT
public:
    explicit FlipbookEffect(QObject *parent = 0);
    ~FlipbookEffect();
    void setDuration(int mSec);
    int duration() const;
signals:
    void aboutToStartAnimation(QWidget* animationWidget);
    void animationFinished();
    void error();
public slots:
    void flip(QWidget* oldWidget, QWidget* newWidget, bool rightToLeft = true,bool deleteOldWidget = false);
private slots:
    void onAnimationFinished();
private:
    FlipbookEffectPrivate* d;
};

#endif // FLIPBOOKEFFECT_H
