#ifndef _SPRITE_H
#define _SPRITE_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsEffect>

class Sprite : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal scale READ scale WRITE setScale)

public:
    Sprite(QGraphicsItem *parent = NULL) : QGraphicsPixmapItem(parent) {}
};

class QAnimatedEffect : public QGraphicsEffect
{
    Q_OBJECT
    Q_PROPERTY(int index READ getIndex WRITE setIndex)

public:
    explicit QAnimatedEffect(QObject *const parent = 0)
        : QGraphicsEffect(parent), stay(false), index(0) {}

    void setStay(bool stay);

    int getIndex() const { return index; }
    void setIndex(int ind)  { index = ind; }

protected:
    bool stay;
    int index;
};

class EffectAnimation : public QObject
{
    Q_OBJECT

public:
    explicit EffectAnimation(QObject *const parent = 0) : QObject(parent) {}

    //emphasize必须与effectOut配套使用，否则会出现内存泄露
    void emphasize(QGraphicsItem *const item);
    void effectOut(QGraphicsItem *const item);

    void sendBack(QGraphicsItem *const item);
};

class EmphasizeEffect : public QAnimatedEffect
{
    Q_OBJECT

public:
    explicit EmphasizeEffect(QObject *const parent = 0);

protected:
    virtual void draw(QPainter *painter);
    virtual QRectF boundingRectFor(const QRectF &sourceRect) const;
};

class SentbackEffect : public QAnimatedEffect
{
    Q_OBJECT

public:
    SentbackEffect(bool stay = false, QObject *parent = 0);

protected:
    virtual void draw(QPainter *painter);
    virtual QRectF boundingRectFor(const QRectF &sourceRect) const;
};

#endif
