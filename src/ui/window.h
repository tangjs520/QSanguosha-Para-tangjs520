#ifndef _WINDOW_H
#define _WINDOW_H

#include <QGraphicsObject>
#include <QStack>

class QGraphicsScale;
class Button;

class Window : public QGraphicsObject
{
    Q_OBJECT

public:
    Window(const QString &title, const QSizeF &size,
        const QString &path = QString(), bool isModal = false, bool isShowTitle = true);
    ~Window() { delete outimg; }

    void addContent(const QString &content);
    Button *addCloseButton(const QString &label);
    void shift(int pos_x = 0, int pos_y = 0);
    void keepWhenDisappear() { keep_when_disappear = true; }
    void setTitle(const QString &title);

    virtual QRectF boundingRect() const;

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public slots:
    void appear();
    void disappear();

private:
    QGraphicsTextItem *titleItem;
    QGraphicsScale *scaleTransform;
    QSizeF size;
    bool keep_when_disappear;
    QImage *outimg;

    QPointF m_windowLastPos;
    static QStack<QGraphicsItem*> s_panelStack;
    bool m_isModal;
};

#endif
