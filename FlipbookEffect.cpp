/*
FlipbookEffect
Author: Srikanth Sombhatla
email: srikanthsombhatla@gmail.com
*/
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRotation>
#include <QGraphicsDropShadowEffect>
#include <QDebug>
#include <QImage>
#include <QMainWindow>
#include <QLayout>
#include "FlipbookEffect.h"

struct FlipbookEffectPrivate {
    int mDuration;
    QWidget* mHostParent; // non-owing
    QWidget* mCentralWidgetToRestore; // non-owning
};

FlipbookEffect::FlipbookEffect(QObject *parent) :
    QObject(parent)
{
    d = new FlipbookEffectPrivate;
    d->mCentralWidgetToRestore = NULL;
    d->mDuration = 700;
}

FlipbookEffect::~FlipbookEffect() {
    delete d;
}

void FlipbookEffect::setDuration(int mSec) {
    d->mDuration = mSec;
}

int FlipbookEffect::duration() const {
    return d->mDuration;
}

void FlipbookEffect::flip(QWidget* aOldWidget, QWidget* aNewWidget, bool rightToLeft, bool deleteOldWidget) {

    bool canAnimate = false;
    if(aOldWidget || aNewWidget && (aOldWidget != aNewWidget) &&
       aOldWidget->isVisible() && aNewWidget->isVisible()) {
        canAnimate = true;
    }

    if(!canAnimate) {
        qWarning()<<"cannot animate, invalid widgets.";
        emit error();
        return;
    }

    // Flipborad effect http://www.substanceofcode.com/2011/03/14/flipboard-page-flip-with-qt-qml/
    // Basic idea is to capture snapshots of two widgets and animate them in a Graphics Scene.
    // This is because, only half of the widget should be animated.
    // Hence seperate pixmapitems are created for each half and are animated.
    // First oldwidget second half is rotated to -90. After this animation is complted,
    // new widget first half is roated to -360. This gives the flipboard effect.
    // New widget snapshot is laid out with a lower z order so that it looks realistic.

    // Capture snapshots of supplied widgets.
    QPixmap oldWidgetSnapShot(aOldWidget->size());
    aOldWidget->render(&oldWidgetSnapShot);
    QPixmap newWidgetSnapShot(aNewWidget->size());
    aNewWidget->render(&newWidgetSnapShot);

    // Break old widget into two halfs.
    QImage img = oldWidgetSnapShot.toImage();
    img = img.copy(0,0,img.width()/2,img.height());
    QPixmap oldFirstHalf = QPixmap::fromImage(img);
    img = oldWidgetSnapShot.toImage();
    img = img.copy(img.width()/2,0,img.width()/2,img.height());
    QPixmap oldSecondHalf = QPixmap::fromImage(img);

    // Take copy of new widget first half
    img = newWidgetSnapShot.toImage();
    img = img.copy(0,0,img.width()/2,img.height());
    QPixmap newFirstHalf = QPixmap::fromImage(img);

    QPixmap newSecondHalf;
    if(!rightToLeft) {
    img = newWidgetSnapShot.toImage();
    img = img.copy(img.width()/2,0,img.width()/2,img.height());
    newSecondHalf = QPixmap::fromImage(img);
    }

    QGraphicsView* view = new QGraphicsView;
    view->setScene(new QGraphicsScene);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing|
                         QPainter::SmoothPixmapTransform|QPainter::HighQualityAntialiasing);
    view->setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing|QGraphicsView::DontSavePainterState);
    view->setInteractive(false);
    view->setWindowFlags(Qt::FramelessWindowHint);
    view->setFrameShape(QFrame::NoFrame);

    QGraphicsPixmapItem* oldFirstHalfPixmapItem  = NULL;
    QGraphicsPixmapItem* oldSecondHalfPixmapItem = NULL;
    QGraphicsPixmapItem* newFirstHalfPixmapItem  = NULL;
    QGraphicsPixmapItem* newSecondHalfPixmapItem = NULL;
    QGraphicsPixmapItem* newFullPixmapItem       = NULL;

    oldFirstHalfPixmapItem = view->scene()->addPixmap(oldFirstHalf);
    oldFirstHalfPixmapItem->setPos(0,0);
    oldSecondHalfPixmapItem = view->scene()->addPixmap(oldSecondHalf);
    oldSecondHalfPixmapItem->setPos(oldSecondHalf.width(),0);
    newFullPixmapItem = view->scene()->addPixmap(newWidgetSnapShot);
    newFullPixmapItem->setZValue(-10); // make sure this item appears as background for a realistic effect.

    if(rightToLeft) {
        newFirstHalfPixmapItem = view->scene()->addPixmap(newFirstHalf);
        newFirstHalfPixmapItem->setPos(0,0);
    }
    else  {
        newSecondHalfPixmapItem = view->scene()->addPixmap(newSecondHalf);
        newSecondHalfPixmapItem->setPos(newSecondHalf.width(),0);
    }

    QSequentialAnimationGroup* seqAnim = new QSequentialAnimationGroup(this);
    QPropertyAnimation* oldWidgetAnim = new QPropertyAnimation(seqAnim);
    QGraphicsRotation* graphicsRotation = new QGraphicsRotation(oldWidgetAnim);
    graphicsRotation->setAxis(Qt::YAxis);
    graphicsRotation->setAngle(0);
    QList<QGraphicsTransform*> t;
    t.append(graphicsRotation);
    if(rightToLeft) {
        oldSecondHalfPixmapItem->setTransformations(t);
    }
    else {
        graphicsRotation->setOrigin(QVector3D(QPoint(oldFirstHalf.width(),0)));
        oldFirstHalfPixmapItem->setTransformations(t);
    }
    oldWidgetAnim->setTargetObject(graphicsRotation);
    oldWidgetAnim->setPropertyName("angle");
    oldWidgetAnim->setStartValue(graphicsRotation->angle());
    oldWidgetAnim->setEndValue( (rightToLeft)?(-90):(90) );
    oldWidgetAnim->setEasingCurve(QEasingCurve::OutQuad);
    oldWidgetAnim->setDuration(d->mDuration/2);

    QPropertyAnimation* newWidgetAnim = new QPropertyAnimation(seqAnim);
    QGraphicsRotation* graphicsRotationForNewWidget = new QGraphicsRotation(newWidgetAnim);
    graphicsRotationForNewWidget->setAxis(Qt::YAxis);
    graphicsRotationForNewWidget->setAngle( (rightToLeft)?(-270):(270) ); // rotating to -270 so that it appears mirrored and when roated to
                                                                          // -360 it is laid out properly.
    if(rightToLeft)
        graphicsRotationForNewWidget->setOrigin(QVector3D(QPoint(newFirstHalf.width(),0)));
    else
        graphicsRotationForNewWidget->setOrigin(QVector3D(QPoint(0,0)));
    QList<QGraphicsTransform*> t2;
    t2.append(graphicsRotationForNewWidget);
    if(rightToLeft)
        newFirstHalfPixmapItem->setTransformations(t2);
    else
        newSecondHalfPixmapItem->setTransformations(t2);
    newWidgetAnim->setTargetObject(graphicsRotationForNewWidget);
    newWidgetAnim->setPropertyName("angle");
    newWidgetAnim->setStartValue(graphicsRotationForNewWidget->angle());
    newWidgetAnim->setEndValue( (rightToLeft)?(-360):(360) );
    newWidgetAnim->setDuration(d->mDuration/2);
    newWidgetAnim->setEasingCurve(QEasingCurve::InOutQuad);
    seqAnim->addAnimation(oldWidgetAnim);
    seqAnim->addAnimation(newWidgetAnim);

    connect(seqAnim,SIGNAL(finished()),this,SLOT(onAnimationFinished()));
    connect(seqAnim,SIGNAL(finished()),view,SLOT(deleteLater()));
    if(deleteOldWidget) {
        connect(seqAnim,SIGNAL(finished()),aOldWidget,SLOT(deleteLater()));
    }
    emit aboutToStartAnimation(view);
    seqAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

// internal method
void FlipbookEffect::onAnimationFinished() {
    emit animationFinished();
}
// eof
