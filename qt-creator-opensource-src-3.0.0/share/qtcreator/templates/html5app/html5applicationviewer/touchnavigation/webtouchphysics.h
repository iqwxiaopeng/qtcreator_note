/*
  This file was generated by the Html5 Application wizard of Qt Creator.
  Html5ApplicationViewer is a convenience class containing mobile device specific
  code such as screen orientation handling.
  It is recommended not to modify this file, since newer versions of Qt Creator
  may offer an updated version of it.
*/

#ifndef WEBTOUCHPHYSICS_H
#define WEBTOUCHPHYSICS_H

#include <QTimer>
#include "webtouchphysicsinterface.h"

static const int KCumulativeDistanceThreshold = 40;
static const int KDecelerationCount = 4;
static const int KDecelerationTimerSpeed = 10;
static const int KFlickScrollThreshold = 400;
static const int KJitterBufferThreshold = 200;
static const qreal KDecelerationSlowdownFactor = 0.975;

static const int KTouchDownStartTime = 200;
static const int KHoverTimeoutThreshold = 100;
static const int KNodeSearchThreshold = 400;

class WebTouchPhysics : public WebTouchPhysicsInterface
{
    Q_OBJECT

public:
    WebTouchPhysics(QObject *parent = 0);
    virtual ~WebTouchPhysics();

    virtual bool inMotion();
    virtual void stop();
    virtual void start(const QPointF &pressPoint, const QWebFrame *frame);
    virtual bool move(const QPointF &pressPoint);
    virtual bool release(const QPointF &pressPoint);

signals:
    void setRange(const QRectF &range);

public slots:
    void decelerationTimerFired();
    void changePosition(const QPointF &point);
    bool isFlick(const QPointF &point, int distance) const;
    bool isPan(const QPointF &point, int distance) const;

private:
    QPointF m_previousPoint;
    QPoint m_startPressPoint;
    QPointF m_decelerationSpeed;
    QList<QPointF> m_decelerationPoints;
    QTimer m_decelerationTimer;
    QPointF m_cumulativeDistance;
    const QWebFrame* m_frame;
    bool m_inMotion;
};

#endif // WEBTOUCHPHYSICS_H
