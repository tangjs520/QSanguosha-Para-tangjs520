#ifndef _BUTTON_H
#define _BUTTON_H

#include <QGraphicsObject>
#include <QFont>
#include <QBitmap>

class Button : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit Button(const QString &label, qreal scale = 1.0);

    void setMute(bool mute) { m_mute = mute; }
    void setFont(const QFont &font);

    virtual QRectF boundingRect() const;

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void timerEvent(QTimerEvent *);

    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

private:
    void drawTitle();

    QString m_label;
    QSizeF m_size;

    bool m_mute;
    bool m_hoverEnter;

    QFont m_font;
    QPixmap m_titlePixmap;
    QPixmap m_btnPixmap;

    QRegion m_maskRegion;

    QGraphicsPixmapItem m_titleItem;

    int m_glow;
    int m_timerId;

signals:
    void clicked();
};

#endif
