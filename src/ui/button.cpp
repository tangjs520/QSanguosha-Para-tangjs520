#include "button.h"
#include "settings.h"
#include "engine.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsDropShadowEffect>

static QRectF ButtonRect(0, 0, 190, 50);

Button::Button(const QString &label, qreal scale)
    : m_label(label), m_size(ButtonRect.size() * scale),
    m_mute(true), m_hoverEnter(false), m_font(Config.SmallFont),
    m_titlePixmap(m_size.toSize()), m_btnPixmap("image/system/button/button.png"),
    m_maskRegion(m_btnPixmap.mask().scaled(m_size.toSize())),
    m_titleItem(this), m_glow(0), m_timerId(0)
{
    setFlags(ItemIsFocusable);

    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    drawTitle();

    QGraphicsDropShadowEffect *de = new QGraphicsDropShadowEffect(this);
    de->setOffset(0);
    de->setBlurRadius(12);
    de->setColor(QColor(255, 165, 0));
    m_titleItem.setGraphicsEffect(de);

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(5);
    effect->setOffset(boundingRect().height() / 7.0);
    effect->setColor(QColor(0, 0, 0, 200));
    setGraphicsEffect(effect);
}

void Button::setFont(const QFont &font)
{
    m_font = font;
    drawTitle();
}

void Button::drawTitle()
{
    m_titlePixmap.fill(QColor(0, 0, 0, 0));

    QPainter pt(&m_titlePixmap);
    pt.setFont(m_font);
    pt.setPen(Config.TextEditColor);
    pt.setRenderHint(QPainter::TextAntialiasing);
    pt.drawText(boundingRect(), Qt::AlignCenter, m_label);

    m_titleItem.setPixmap(m_titlePixmap);
}

void Button::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    if (m_hoverEnter || !isUnderMouse()) {
        // fake event
        return;
    }

    m_hoverEnter = true;

    if (!m_mute) {
        Sanguosha->playSystemAudioEffect("button-hover");
    }

    if (!m_timerId) {
        m_timerId = QObject::startTimer(40);
    }
}

void Button::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    moveBy(2, 2);
    grabMouse();

    event->accept();
}

void Button::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    ungrabMouse();
    moveBy(-2, -2);

    hoverLeaveEvent(NULL);

    if (contains(event->lastPos())) {
        if (!m_mute) {
            Sanguosha->playSystemAudioEffect("button-down");
        }

        emit clicked();
    }
}

QRectF Button::boundingRect() const
{
    return QRectF(QPointF(), m_size);
}

void Button::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF rect = boundingRect();
    painter->drawPixmap(rect.toRect(), m_btnPixmap);
    painter->setClipRegion(m_maskRegion);
    painter->fillRect(rect, QColor(255, 255, 255, m_glow * 10));
}

void Button::timerEvent(QTimerEvent *)
{
    update();

    if (m_hoverEnter && m_glow < 5) {
        ++m_glow;
    }
    else if (!m_hoverEnter && m_glow > 0) {
        --m_glow;
    }
    else if (m_timerId) {
        QObject::killTimer(m_timerId);
        m_timerId = 0;
    }
}

void Button::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    if (!m_hoverEnter) {
        return;
    }

    m_hoverEnter = false;

    if (!m_timerId) {
        m_timerId = QObject::startTimer(40);
    }
}

void Button::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isUnderMouse()) {
        if (!m_hoverEnter) {
            hoverEnterEvent(event);
        }
    }
    else {
        if (m_hoverEnter) {
            hoverLeaveEvent(event);
        }
    }
}
