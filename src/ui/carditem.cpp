#include "carditem.h"
#include "engine.h"
#include "SkinBank.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QApplication>

CardItem::CardItem(const Card *card)
    : m_backgroundPixmap(G_COMMON_LAYOUT.m_cardMainArea.size()),
    m_canvas(G_COMMON_LAYOUT.m_cardMainArea.size())
{
    initialize();
    setCard(card);
    setAcceptHoverEvents(true);
}

CardItem::CardItem(const QString &generalName)
    : m_backgroundPixmap(G_COMMON_LAYOUT.m_cardMainArea.size()),
    m_canvas(G_COMMON_LAYOUT.m_cardMainArea.size())
{
    m_cardId = Card::S_UNKNOWN_CARD_ID;
    initialize();
    changeGeneral(generalName);
}

CardItem::~CardItem()
{
    QMutexLocker locker(&m_animationMutex);
    stopAnimation();
}

void CardItem::initialize()
{
    //卡牌默认不支持任何鼠标按键，以避免出现玩家误操作导致卡牌遗留在界面上无法消失的问题
    setAcceptedMouseButtons(0);

    setFlag(QGraphicsItem::ItemIsMovable);

    m_opacityAtHome = 1.0;
    m_currentAnimation = NULL;
    _m_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    _m_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    m_showFootnote = true;
    m_isSelected = false;
    m_autoBack = true;
    m_mouseDoubleclicked = false;

    resetTransform();
    setTransform(QTransform::fromTranslate(-_m_width / 2, -_m_height / 2), true);

    m_backgroundPixmap.fill(Qt::transparent);
    m_canvas.fill(Qt::transparent);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

QRectF CardItem::boundingRect() const
{
    return G_COMMON_LAYOUT.m_cardMainArea;
}

void CardItem::setCard(const Card *card)
{
    m_cardId = (card ? card->getId() : Card::S_UNKNOWN_CARD_ID);

    const Card *engineCard = Sanguosha->getEngineCard(m_cardId);
    if (engineCard) {
        setObjectName(engineCard->objectName());
        setToolTip(engineCard->getDescription());
    }
    else {
        setObjectName("unknown");
    }

    QPainter painter(&m_backgroundPixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    painter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
        G_ROOM_SKIN.getCardMainPixmap(objectName(), true));

    if (engineCard) {
        painter.drawPixmap(G_COMMON_LAYOUT.m_cardSuitArea,
            G_ROOM_SKIN.getCardSuitPixmap(engineCard->getSuit()));

        painter.drawPixmap(G_COMMON_LAYOUT.m_cardNumberArea,
            G_ROOM_SKIN.getCardNumberPixmap(engineCard->getNumber(), engineCard->isBlack()));
    }
}

void CardItem::changeGeneral(const QString &generalName)
{
    setObjectName(generalName);

    QPainter painter(&m_backgroundPixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    const General *general = Sanguosha->getGeneral(generalName);
    if (general) {
        painter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
            G_ROOM_SKIN.getCardMainPixmap(objectName(), true));

        setToolTip(general->getSkillDescription(true));
    }
    else {
        painter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
            G_ROOM_SKIN.getPixmap("generalCardBack", QString(), true));

        setToolTip(QString());
    }
}

const Card *CardItem::getCard() const
{
    return Sanguosha->getCard(m_cardId);
}

void CardItem::goBack(bool playAnimation, bool doFade)
{
    if (playAnimation) {
        getGoBackAnimation(doFade);
        if (m_currentAnimation != NULL) {
            m_currentAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    else {
        QMutexLocker locker(&m_animationMutex);
        stopAnimation();
        setPos(homePos());
    }
}

QAbstractAnimation *CardItem::getGoBackAnimation(bool doFade, bool smoothTransition, int duration)
{
    QMutexLocker locker(&m_animationMutex);
    stopAnimation();

    QPropertyAnimation *goback = new QPropertyAnimation(this, "pos", this);
    goback->setEndValue(m_homePos);
    goback->setEasingCurve(QEasingCurve::OutQuad);
    goback->setDuration(duration);

    if (doFade) {
        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        QPropertyAnimation *disappear = new QPropertyAnimation(this, "opacity", this);
        double middleOpacity = qMax(opacity(), m_opacityAtHome);
        if (middleOpacity == 0) middleOpacity = 1.0;
        disappear->setEndValue(m_opacityAtHome);
        if (!smoothTransition) {
            disappear->setKeyValueAt(0.2, middleOpacity);
            disappear->setKeyValueAt(0.8, middleOpacity);
            disappear->setDuration(duration);
        }

        group->addAnimation(goback);
        group->addAnimation(disappear);

        m_currentAnimation = group;
    }
    else {
        m_currentAnimation = goback;
    }

    connect(m_currentAnimation, SIGNAL(finished()), this, SIGNAL(movement_animation_finished()));
    connect(m_currentAnimation, SIGNAL(finished()),
        this, SLOT(animationFinished()));

    return m_currentAnimation;
}

void CardItem::showAvatar(const General *general)
{
    m_avatarName = general->objectName();
}

CardItem *CardItem::FindItem(const QList<CardItem *> &items, int card_id)
{
    foreach (CardItem *item, items) {
        if (item->getCard() == NULL) {
            if (card_id == Card::S_UNKNOWN_CARD_ID) {
                return item;
            }
        }
        else if (item->getCard()->getId() == card_id) {
            return item;
        }
    }

    return NULL;
}

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent *)
{
    //在有可能运行下一次动画之前，先停止之前有可能还未结束的动画
    //以避免产生不必要的振动效果
    QMutexLocker locker(&m_animationMutex);
    stopAnimation();

    m_mouseDoubleclicked = false;
}

void CardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    //如果产生了双击事件，会在双击事件后，又会接着产生mouseReleaseEvent，
    //因此需要屏蔽这次release事件，否则卡牌会产生不规律的移动效果
    if (m_mouseDoubleclicked) {
        return;
    }

    emit released();

    if (m_autoBack) {
        goBack(true, false);
    }
}

void CardItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!(flags() & QGraphicsItem::ItemIsMovable)
        || QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length()
        < QApplication::startDragDistance()) {
        return;
    }

    QPointF newPos = mapToParent(event->pos());
    QPointF downPos = event->buttonDownPos(Qt::LeftButton);
    setPos(newPos - transform().map(downPos));
}

void CardItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *)
{
    m_mouseDoubleclicked = true;

    if (hasFocus()) {
        emit double_clicked();
    }
}

void CardItem::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    emit enter_hover();
}

void CardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    emit leave_hover();
}

void CardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    //由于已支持在游戏中随时退出的功能，经测试发现，
    //有时painter参数会为空，从而导致程序崩溃。
    //故增加下述判断，以避免程序崩溃
    if (NULL == painter) {
        return;
    }

    QPainter tempPainter(&m_canvas);
    tempPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (!isEnabled()) {
        tempPainter.fillRect(G_COMMON_LAYOUT.m_cardMainArea, QColor(100, 100, 100, 255));
        tempPainter.setOpacity(0.7);
    }

    tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea, m_backgroundPixmap);

    if (m_showFootnote) {
        //脚注文字始终不变暗
        tempPainter.setOpacity(1);

        tempPainter.drawImage(G_COMMON_LAYOUT.m_cardFootnoteArea,
            m_footnoteImage);
    }

    if (!m_avatarName.isEmpty()) {
        //小图像始终不变暗
        tempPainter.setOpacity(1);

        //圆角化小图像
        QPainterPath roundRectPath;
        roundRectPath.addRoundedRect(G_COMMON_LAYOUT.m_cardAvatarArea, 4, 4);
        tempPainter.setClipPath(roundRectPath);

        tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardAvatarArea,
            G_ROOM_SKIN.getCardAvatarPixmap(m_avatarName));
    }

    tempPainter.end();

    painter->drawPixmap(G_COMMON_LAYOUT.m_cardMainArea, m_canvas);
}

void CardItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *)
{
    emit context_menu_event_triggered();
}

void CardItem::setFootnote(const QString &desc)
{
    if (!desc.isEmpty()) {
        paintFootnoteImage(G_COMMON_LAYOUT.m_cardFootnoteFont, desc);
    }
}

void CardItem::setDoubleClickedFootnote(const QString &desc)
{
    //如果武将名称过长，将会导致脚注文字被小图像遮掩住，
    //因此限定：如果武将名称不超过6个字，则可以适当增加脚注字体的大小，以便更有利于看清楚脚注文字，
    //否则仍使用原脚注字体大小
    if (desc.size() < 6) {
        SanShadowTextFont font = G_COMMON_LAYOUT.m_cardFootnoteFont;
        QSize fontNewSize = font.size();
        fontNewSize.rwidth() += 2;
        fontNewSize.rheight() += 2;
        font.setSize(fontNewSize);

        paintFootnoteImage(font, desc);
    }
    else {
        paintFootnoteImage(G_COMMON_LAYOUT.m_cardFootnoteFont, desc);
    }
}

void CardItem::paintFootnoteImage(const SanShadowTextFont &footnoteFont,
    const QString &desc)
{
    QRect rect = G_COMMON_LAYOUT.m_cardFootnoteArea;
    rect.moveTopLeft(QPoint(0, 0));
    m_footnoteImage = QImage(rect.size(), QImage::Format_ARGB32);
    m_footnoteImage.fill(Qt::transparent);
    QPainter painter(&m_footnoteImage);
    footnoteFont.paintText(&painter, QRect(QPoint(0, 0), rect.size()),
        (Qt::AlignmentFlag)((int)Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWrapAnywhere), desc);
}

void CardItem::animationFinished()
{
    QMutexLocker locker(&m_animationMutex);
    m_currentAnimation = NULL;
}

void CardItem::stopAnimation()
{
    if (m_currentAnimation != NULL) {
        m_currentAnimation->stop();
        delete m_currentAnimation;
        m_currentAnimation = NULL;
    }
}
