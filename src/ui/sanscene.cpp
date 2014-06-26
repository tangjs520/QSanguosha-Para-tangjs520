#include "sanscene.h"

#include <QGraphicsSceneMouseEvent>

SanScene::SanScene(QObject *const parent)
    : QGraphicsScene(parent), m_enableMouseHover(true)
{
}

SanScene::~SanScene()
{
}

void SanScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (!m_enableMouseHover) {
        mouseEvent->ignore();
    }
    else {
        QGraphicsScene::mouseMoveEvent(mouseEvent);
    }
}
