#include "window.h"
#include "settings.h"
#include "button.h"

#include <QPainter>
#include <QGraphicsScale>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

QStack<QGraphicsItem*> Window::s_panelStack;

Window::Window(const QString &title, const QSizeF &size,
    const QString &path/* = QString()*/, bool isModal/* = false*/, bool isShowTitle/* = true*/)
    : size(size), keep_when_disappear(false), m_isModal(isModal)
{
    setFlags(ItemIsMovable);

    setAcceptedMouseButtons(Qt::LeftButton);
    setData(0, title);

    if (!path.isEmpty()) {
        outimg = new QImage(path);
    }
    else {
        outimg = size.width() > size.height()
            ? new QImage("image/system/tip.png")
            : new QImage("image/system/about.png");
    }

    scaleTransform = new QGraphicsScale(this);
    scaleTransform->setXScale(1.05);
    scaleTransform->setYScale(0.95);
    scaleTransform->setOrigin(QVector3D(boundingRect().width() / 2, boundingRect().height() / 2, 0));

    QList<QGraphicsTransform *> transforms;
    transforms << scaleTransform;
    setTransformations(transforms);

    setOpacity(0.0);

    if (isShowTitle) {
        titleItem = new QGraphicsTextItem(this);
        setTitle(title);
    }
}

void Window::addContent(const QString &content)
{
    QGraphicsTextItem *content_item = new QGraphicsTextItem(this);
    content_item->moveBy(15, 40);
    content_item->setHtml(content);
    content_item->setDefaultTextColor(Qt::white);
    content_item->setTextWidth(size.width() - 30);

    QFont font;
    font.setBold(true);
    font.setPointSize(10);
    content_item->setFont(font);
}

Button *Window::addCloseButton(const QString &label)
{
    Button *ok_button = new Button(label, 0.6);
    QFont font = Config.TinyFont;
    font.setBold(true);
    ok_button->setFont(font);
    ok_button->setParentItem(this);

    qreal x = size.width() - ok_button->boundingRect().width() - 25;
    qreal y = size.height() - ok_button->boundingRect().height() - 25;
    ok_button->setPos(x, y);

    connect(ok_button, SIGNAL(clicked()), this, SLOT(disappear()));
    return ok_button;
}

void Window::shift(int pos_x, int pos_y)
{
    resetTransform();
    setTransform(QTransform::fromTranslate((pos_x - size.width()) / 2, (pos_y - size.height()) / 2), true);
}

QRectF Window::boundingRect() const
{
    return QRectF(QPointF(), size);
}

void Window::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF window_rect = boundingRect();

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |QPainter::SmoothPixmapTransform);
    painter->drawImage(window_rect, *outimg);
}

void Window::appear()
{
    //模态化该窗口
    //用户必须通过单击该按钮来关闭窗口，在关闭窗口之前，
    //不能操作当前scene下的其他items
    if (m_isModal) {
        setFlags(flags() | ItemIsPanel);

        QGraphicsScene *const currentScene = scene();
        if (currentScene) {
            if (!s_panelStack.empty()) {
                QGraphicsItem* item = s_panelStack.top();
                item->setPanelModality(NonModal);
            }

            setPanelModality(SceneModal);
            s_panelStack.push(this);

            currentScene->setActivePanel(this);
        }
    }

    QPropertyAnimation *scale_x = new QPropertyAnimation(scaleTransform, "xScale", this);
    QPropertyAnimation *scale_y = new QPropertyAnimation(scaleTransform, "yScale", this);
    QPropertyAnimation *opacity = new QPropertyAnimation(this, "opacity", this);
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    scale_x->setEndValue(1);
    scale_y->setEndValue(1);
    opacity->setEndValue(1.0);
    group->addAnimation(scale_x);
    group->addAnimation(scale_y);
    group->addAnimation(opacity);

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void Window::disappear()
{
    QPropertyAnimation *scale_x = new QPropertyAnimation(scaleTransform, "xScale", this);
    QPropertyAnimation *scale_y = new QPropertyAnimation(scaleTransform, "yScale", this);
    QPropertyAnimation *opacity = new QPropertyAnimation(this, "opacity", this);
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    scale_x->setEndValue(1.05);
    scale_y->setEndValue(0.95);
    opacity->setEndValue(0.0);
    group->addAnimation(scale_x);
    group->addAnimation(scale_y);
    group->addAnimation(opacity);

    if (!keep_when_disappear) {
        connect(group, SIGNAL(finished()), this, SLOT(deleteLater()));
    }

    group->start(QAbstractAnimation::DeleteWhenStopped);

    if (m_isModal) {
        QGraphicsScene *const currentScene = scene();
        if (currentScene) {
            if (!s_panelStack.empty()) {
                s_panelStack.pop();

                if (!s_panelStack.empty()) {
                    QGraphicsItem* item = s_panelStack.top();
                    item->setPanelModality(SceneModal);

                    currentScene->setActivePanel(item);
                }
            }
        }
    }
}

void Window::setTitle(const QString &title)
{
    QString style;
    style.append("font-size:18pt; ");
    style.append("color:#77c379; ");
    style.append(QString("font-family: %1").arg(Config.SmallFont.family()));

    QString content;
    content.append(QString("<h style=\"%1\">%2</h>").arg(style).arg(title));

    titleItem->setHtml(content);
    titleItem->setPos(size.width() / 2 - titleItem->boundingRect().width() / 2, 10);
}

void Window::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //当鼠标指针不在窗口上时，忽略左键按下事件，以避免产生"仍能拖动窗口"的效果
    if (!isUnderMouse()) {
        event->ignore();
    }
    else {
        QGraphicsObject::mousePressEvent(event);
    }
}

void Window::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsObject::mouseReleaseEvent(event);

    const QGraphicsScene *const currentScene = scene();
    if (currentScene) {
        QRectF currentSceneRect = currentScene->sceneRect();
        QRectF windowRect = sceneBoundingRect();

        //防止窗口移出当前scene
        if (!currentSceneRect.intersects(windowRect)) {
            setPos(m_windowLastPos);
        }
        else {
            m_windowLastPos = pos();
        }
    }
}
