#include "carditem.h"
#include "engine.h"
#include "skill.h"
#include "clientplayer.h"
#include "settings.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QFocusEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

void CardItem::_initialize() {
    //卡牌默认不支持任何鼠标按键，以避免出现玩家误操作导致卡牌遗留在界面上无法消失的问题
    setAcceptedMouseButtons(0);

    setFlag(QGraphicsItem::ItemIsMovable);
    m_opacityAtHome = 1.0;
    m_currentAnimation = NULL;
    _m_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    _m_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    _m_showFootnote = true;
    m_isSelected = false;
    _m_isUnknownGeneral = false;
    auto_back = true;
    frozen = false;
    _m_mouse_doubleclicked = false;

    resetTransform();
    setTransform(QTransform::fromTranslate(-_m_width / 2, -_m_height / 2), true);
}

CardItem::CardItem(const Card *card) {
    _initialize();
    setCard(card);
    setAcceptHoverEvents(true);
}

CardItem::CardItem(const QString &general_name) {
    m_cardId = Card::S_UNKNOWN_CARD_ID;
    _initialize();
    changeGeneral(general_name);
}

QRectF CardItem::boundingRect() const{
    return G_COMMON_LAYOUT.m_cardMainArea;
}

void CardItem::setCard(const Card *card) {
    if (card != NULL) {
        m_cardId = card->getId();
        const Card *engineCard = Sanguosha->getEngineCard(m_cardId);
        Q_ASSERT(engineCard != NULL);
        setObjectName(engineCard->objectName());
        QString description = engineCard->getDescription();
        setToolTip(description);
    } else {
        m_cardId = Card::S_UNKNOWN_CARD_ID;
        setObjectName("unknown");
    }
}

CardItem::~CardItem() {
    m_animationMutex.lock();
    _stopAnimation();
    m_animationMutex.unlock();
}

void CardItem::changeGeneral(const QString &general_name) {
    setObjectName(general_name);
    const General *general = Sanguosha->getGeneral(general_name);
    if (general) {
        _m_isUnknownGeneral = false;
        setToolTip(general->getSkillDescription(true));
    } else {
        _m_isUnknownGeneral = true;
        setToolTip(QString());
    }
}

const Card *CardItem::getCard() const{
    return Sanguosha->getCard(m_cardId);
}

void CardItem::setHomePos(QPointF home_pos) {
    this->home_pos = home_pos;
}

QPointF CardItem::homePos() const{
    return home_pos;
}

void CardItem::goBack(bool playAnimation, bool doFade) {
    if (playAnimation) {
        getGoBackAnimation(doFade);
        if (m_currentAnimation != NULL)
            m_currentAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        m_animationMutex.lock();
        _stopAnimation();
        setPos(homePos());
        m_animationMutex.unlock();
    }
}

QAbstractAnimation *CardItem::getGoBackAnimation(bool doFade, bool smoothTransition, int duration) {
    m_animationMutex.lock();
    _stopAnimation();

    QPropertyAnimation *goback = new QPropertyAnimation(this, "pos", this);
    goback->setEndValue(home_pos);
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
    } else {
        m_currentAnimation = goback;
    }
    m_animationMutex.unlock();

    connect(m_currentAnimation, SIGNAL(finished()), this, SIGNAL(movement_animation_finished()));
    connect(m_currentAnimation, SIGNAL(finished()),
        this, SLOT(_animationFinished()));

    return m_currentAnimation;
}

void CardItem::showAvatar(const General *general) {
    _m_avatarName = general->objectName();
}

void CardItem::hideAvatar() {
    _m_avatarName = QString();
}

void CardItem::setAutoBack(bool auto_back) {
    this->auto_back = auto_back;
}

bool CardItem::isEquipped() const{
    const Card *card = getCard();
    Q_ASSERT(card);
    return Self->hasEquip(card);
}

void CardItem::setFrozen(bool is_frozen) {
    frozen = is_frozen;
}

CardItem *CardItem::FindItem(const QList<CardItem *> &items, int card_id) {
    foreach (CardItem *item, items) {
        if (item->getCard() == NULL) {
            if (card_id == Card::S_UNKNOWN_CARD_ID)
                return item;
            else
                continue;
        }
        if (item->getCard()->getId() == card_id)
            return item;
    }

    return NULL;
}

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    //在有可能运行下一次动画之前，先停止之前有可能还未结束的动画
    //以避免产生不必要的振动效果
    m_animationMutex.lock();
    _stopAnimation();
    m_animationMutex.unlock();

    if (frozen) return;

    _m_mouse_doubleclicked = false;
    _m_lastMousePressScenePos = mapToParent(mouseEvent->pos());
}

void CardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
    if (frozen) return;

    //如果产生了双击事件，会在双击事件后，又会接着产生mouseReleaseEvent，
    //因此需要屏蔽这次release事件，否则卡牌会产生不规律的移动效果
    if (_m_mouse_doubleclicked) {
        return;
    }

    emit released();

    if (auto_back) {
        goBack(true, false);
    }
}

void CardItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    if (!(flags() & QGraphicsItem::ItemIsMovable)) return;
    QPointF newPos = mapToParent(mouseEvent->pos());
    QPointF down_pos = mouseEvent->buttonDownPos(Qt::LeftButton);
    setPos(newPos - this->transform().map(down_pos));
}

void CardItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    _m_mouse_doubleclicked = true;

    if (frozen) return;

    if (hasFocus()) {
        emit double_clicked();
    }
}

void CardItem::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
    emit enter_hover();
}

void CardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    emit leave_hover();
}

void CardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    //由于已支持在游戏中随时退出的功能，经测试发现，
    //有时painter参数会为空，从而导致程序崩溃。
    //故增加下述判断，以避免程序崩溃
    if (NULL == painter) {
        return;
    }
    //采用双缓冲机制绘图，以避免出现在卡牌之间层次切换时产生的闪烁现象
    static QPixmap tempPix(G_COMMON_LAYOUT.m_cardMainArea.size());
    tempPix.fill(Qt::transparent);

    QPainter tempPainter(&tempPix);
    tempPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (!isEnabled()) {
        tempPainter.fillRect(G_COMMON_LAYOUT.m_cardMainArea, QColor(100, 100, 100, 255 * opacity()));
        tempPainter.setOpacity(0.7 * opacity());
    }

    if (!_m_isUnknownGeneral) {
        tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
            G_ROOM_SKIN.getCardMainPixmap(objectName(), true));
    }
    else {
        tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardMainArea,
            G_ROOM_SKIN.getPixmap("generalCardBack", QString(), true));
    }

    const Card *card = Sanguosha->getEngineCard(m_cardId);
    if (card) {
        tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardSuitArea,
            G_ROOM_SKIN.getCardSuitPixmap(card->getSuit()));

        tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardNumberArea,
            G_ROOM_SKIN.getCardNumberPixmap(card->getNumber(), card->isBlack()));
    }

    if (_m_showFootnote) {
        //脚注文字始终不变暗
        tempPainter.setOpacity(1);

        tempPainter.drawImage(G_COMMON_LAYOUT.m_cardFootnoteArea,
            _m_footnoteImage);
    }

    if (!_m_avatarName.isEmpty()) {
        //小图像始终不变暗
        tempPainter.setOpacity(1);

        //圆角化小图像
        QPainterPath roundRectPath;
        roundRectPath.addRoundedRect(G_COMMON_LAYOUT.m_cardAvatarArea, 4, 4);
        tempPainter.setClipPath(roundRectPath);

        tempPainter.drawPixmap(G_COMMON_LAYOUT.m_cardAvatarArea,
            G_ROOM_SKIN.getCardAvatarPixmap(_m_avatarName));
    }

    painter->drawPixmap(G_COMMON_LAYOUT.m_cardMainArea, tempPix);
}

void CardItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *)
{
    emit context_menu_event_triggered();
}

void CardItem::setFootnote(const QString &desc) {
    if (!desc.isEmpty()) {
        paintFootnote(G_COMMON_LAYOUT.m_cardFootnoteFont, desc);
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

        paintFootnote(font, desc);
    }
    else {
        paintFootnote(G_COMMON_LAYOUT.m_cardFootnoteFont, desc);
    }
}

void CardItem::paintFootnote(const SanShadowTextFont &FootnoteFont,
    const QString &desc)
{
    QRect rect = G_COMMON_LAYOUT.m_cardFootnoteArea;
    rect.moveTopLeft(QPoint(0, 0));
    _m_footnoteImage = QImage(rect.size(), QImage::Format_ARGB32);
    _m_footnoteImage.fill(Qt::transparent);
    QPainter painter(&_m_footnoteImage);
    FootnoteFont.paintText(&painter, QRect(QPoint(0, 0), rect.size()),
        (Qt::AlignmentFlag)((int)Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWrapAnywhere), desc);
}

void CardItem::_animationFinished()
{
    m_animationMutex.lock();
    m_currentAnimation = NULL;
    m_animationMutex.unlock();
}

void CardItem::_stopAnimation()
{
    if (m_currentAnimation != NULL) {
        m_currentAnimation->stop();
        delete m_currentAnimation;
        m_currentAnimation = NULL;
    }
}
