#ifndef SANSCENE_H
#define SANSCENE_H

#include <QGraphicsScene>

class SanScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit SanScene(QObject *const parent = 0);
    ~SanScene();

    void setEnableMouseHover(bool enable) { m_enableMouseHover = enable; }

    virtual void adjustItems() {}

protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);

private:
    bool m_enableMouseHover;
};

#endif // SANSCENE_H
