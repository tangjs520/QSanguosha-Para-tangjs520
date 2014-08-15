#include "cardcontainer.h"
#include "clientplayer.h"
#include "carditem.h"
#include "engine.h"
#include "client.h"

#include "backgroundrunner.h"
#include <QTimer>

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

CardContainer::CardContainer()
    : _m_background("image/system/card-container.png")
{
    setTransform(QTransform::fromTranslate(-_m_background.width() / 2, -_m_background.height() / 2), true);
    _m_boundingRect = QRectF(QPoint(0, 0), _m_background.size());
    setFlag(ItemIsFocusable);
    setFlag(ItemIsMovable);

    m_currentPlayer = NULL;
    m_delayClose = false;

    close_button = new QSanButton("card_container", "close", this);
    close_button->setPos(529, 21);

    close_button->hide();
    connect(close_button, SIGNAL(clicked()), this, SLOT(clear()));
}

void CardContainer::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->drawPixmap(0, 0, _m_background);
}

QRectF CardContainer::boundingRect() const{
    return _m_boundingRect;
}

void CardContainer::fillCards(const QList<int> &card_ids, const QList<int> &disabled_ids) {
    QList<CardItem *> card_items;
    if (card_ids.isEmpty() && items.isEmpty())
        return;
    else if (card_ids.isEmpty() && !items.isEmpty()) {
        card_items = items;
        items.clear();
    } else if (!items.isEmpty()) {
        retained_stack.push(retained());
        items_stack.push(items);
        foreach (CardItem *item, items)
            item->hide();
        items.clear();
    }

    close_button->hide();
    if (card_items.isEmpty())
        card_items = _createCards(card_ids);

    int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    QPointF pos1(30 + card_width / 2, 40 + G_COMMON_LAYOUT.m_cardNormalHeight / 2);
    QPointF pos2(30 + card_width / 2, 184 + G_COMMON_LAYOUT.m_cardNormalHeight / 2);
    int skip = 102;
    qreal whole_width = skip * 4;
    items.append(card_items);
    int n = items.length();

    for (int i = 0; i < n; ++i) {
        QPointF pos;
        if (n <= 10) {
            if (i < 5) {
                pos = pos1;
                pos.setX(pos.x() + i * skip);
            } else {
                pos = pos2;
                pos.setX(pos.x() + (i - 5) * skip);
            }
        } else {
            int half = (n + 1) / 2;
            qreal real_skip = whole_width / (half - 1);

            if (i < half) {
                pos = pos1;
                pos.setX(pos.x() + i * real_skip);
            } else {
                pos = pos2;
                pos.setX(pos.x() + (i - half) * real_skip);
            }
        }
        CardItem *item = items[i];
        item->setPos(pos);
        item->setHomePos(pos);
        item->setOpacity(1.0);
        item->setHomeOpacity(1.0);

        //不允许移动卡牌，避免在五谷丰登时，出现把电脑选中的卡牌移出到窗口外的现象
        item->setFlags(ItemIsFocusable);

        //卡牌需要支持鼠标左键，以便双击选择
        item->setAcceptedMouseButtons(Qt::LeftButton);

        if (disabled_ids.contains(item->getCard()->getEffectiveId())) {
            item->setEnabled(false);
        }

        item->show();
    }
}

bool CardContainer::_addCardItems(QList<CardItem *> &, const CardsMoveStruct &) {
    return true;
}

bool CardContainer::retained() {
    return close_button != NULL && close_button->isVisible();
}

void CardContainer::clear() {
    if (!isVisible()) {
        return;
    }

    //延时一下再关闭，以便留出时间让玩家看清最后一张牌是谁选择的或选择的最后一张牌是什么
    if (!close_button->isVisible() && m_delayClose) {
        BackgroundRunner::msleep(1000);
    }

    foreach (CardItem *item, items) {
        item->hide();

        delete item;
        item = NULL;
    }
    items.clear();

    if (!items_stack.isEmpty()) {
        items = items_stack.pop();
        bool retained = retained_stack.pop();
        fillCards();
        if (retained && close_button)
            close_button->show();
    } else {
        close_button->hide();
        hide();
    }

    m_delayClose = false;
}

QList<CardItem *> CardContainer::removeCardItems(const QList<int> &card_ids, Player::Place) {
    QList<CardItem *> result;
    foreach (int card_id, card_ids) {
        CardItem *to_take = NULL;
        foreach (CardItem *item, items) {
            if (item->getCard()->getId() == card_id) {
                to_take = item;
                break;
            }
        }
        if (to_take == NULL) continue;

        to_take->setEnabled(false);

        CardItem *copy = new CardItem(to_take->getCard());
        copy->setPos(mapToScene(to_take->pos()));
        copy->setEnabled(false);
        result.append(copy);

        //移出去的卡牌恢复成默认不支持鼠标按键的状态
        copy->setAcceptedMouseButtons(0);

        //选择完卡牌后，在卡牌上增加显示选择该卡牌的武将名称
        if (m_currentPlayer) {
            to_take->setDoubleClickedFootnote(ClientInstance->getPlayerName(m_currentPlayer->getGeneralName()));
            to_take->showAvatar(m_currentPlayer->getGeneral());
        }
    }

    m_currentPlayer = NULL;
    m_delayClose = true;

    return result;
}

int CardContainer::getFirstEnabled() const{
    foreach (CardItem *card, items) {
        if (card->isEnabled())
            return card->getCard()->getId();
    }
    return -1;
}

void CardContainer::startChoose() {
    close_button->hide();
    foreach (CardItem *item, items) {
        connect(item, SIGNAL(leave_hover()), this, SLOT(grabItem()));
        connect(item, SIGNAL(double_clicked()), this, SLOT(chooseItem()));
    }
}

void CardContainer::startGongxin(const QList<int> &enabled_ids) {
    if (enabled_ids.isEmpty()) return;
    foreach (CardItem *item, items) {
        const Card *card = item->getCard();
        if (card && enabled_ids.contains(card->getEffectiveId()))
            connect(item, SIGNAL(double_clicked()), this, SLOT(gongxinItem()));
        else
            item->setEnabled(false);
    }
}

void CardContainer::addCloseButton() {
    close_button->show();

    //必须人为关闭的卡牌候选框，也允许玩家设置成延时若干秒后自动关闭
    if (Config.AutoCloseCardContainerDelaySeconds > 0) {
        QTimer::singleShot(Config.AutoCloseCardContainerDelaySeconds * 1000, this, SLOT(clear()));
    }
}

void CardContainer::grabItem() {
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item && !collidesWithItem(card_item)) {
        card_item->disconnect(this);
        emit item_chosen(card_item->getCard()->getId());
    }
}

void CardContainer::chooseItem() {
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item) {
        card_item->disconnect(this);
        emit item_chosen(card_item->getCard()->getId());
    }
}

void CardContainer::gongxinItem() {
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item) {
        emit item_gongxined(card_item->getCard()->getId());
        clear();
    }
}

void CardContainer::view(const ClientPlayer *player) {
    QList<int> card_ids;
    QList<const Card *> cards = player->getHandcards();
    foreach (const Card *card, cards)
        card_ids << card->getEffectiveId();

    fillCards(card_ids);
}

#include "room.h"

GuanxingBox::GuanxingBox()
    : QSanSelectableItem("image/system/guanxing-box.png", true)
{
    setFlag(ItemIsFocusable);
    setFlag(ItemIsMovable);

    m_guanxingType = Room::GuanxingBothSides;
}

void GuanxingBox::doGuanxing(const QList<int> &card_ids, int guanxing_type) {
    if (card_ids.isEmpty()) {
        clear();
        return;
    }

    up_items.clear();
    down_items.clear();
    m_guanxingType = guanxing_type;
    QList<CardItem *> &guanxingCardItems
        = (Room::GuanxingDownOnly == m_guanxingType) ? down_items
        : up_items;

    foreach (int card_id, card_ids) {
        CardItem *card_item = new CardItem(Sanguosha->getCard(card_id));
        card_item->setAutoBack(false);
        card_item->setFlag(QGraphicsItem::ItemIsFocusable);
        connect(card_item, SIGNAL(released()), this, SLOT(adjust()));

        guanxingCardItems << card_item;

        card_item->setParentItem(this);

        //卡牌需要支持鼠标左键，以便拖动选择
        card_item->setAcceptedMouseButtons(Qt::LeftButton);
    }

    show();

    int start_y = (Room::GuanxingDownOnly == m_guanxingType) ? start_y2 : start_y1;
    QPointF source(start_x, start_y);
    for (int i = 0; i < guanxingCardItems.length(); ++i) {
        CardItem *card_item = guanxingCardItems.at(i);
        QPointF pos(start_x + i * skip, start_y);
        card_item->setPos(source);
        card_item->setHomePos(pos);
        card_item->goBack(true);
    }
}

void GuanxingBox::adjust() {
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == NULL) return;

    up_items.removeOne(item);
    down_items.removeOne(item);

    QList<CardItem *> *items = NULL;
    if (Room::GuanxingUpOnly == m_guanxingType) {
        items = &up_items;
    }
    else if (Room::GuanxingDownOnly == m_guanxingType) {
        items = &down_items;
    }
    else if (item->y() <= middle_y) {
        items = &up_items;
    }
    else {
        items = &down_items;
    }

    int c = (item->x() + item->boundingRect().width() / 2 - start_x) / G_COMMON_LAYOUT.m_cardNormalWidth;
    c = qBound(0, c, items->length());
    items->insert(c, item);

    for (int i = 0; i < up_items.length(); ++i) {
        QPointF pos(start_x + i * skip, start_y1);
        up_items.at(i)->setHomePos(pos);
        up_items.at(i)->goBack(true);
    }

    for (int i = 0; i < down_items.length(); ++i) {
        QPointF pos(start_x + i * skip, start_y2);
        down_items.at(i)->setHomePos(pos);
        down_items.at(i)->goBack(true);
    }
}

void GuanxingBox::clear() {
    foreach (CardItem *card_item, up_items)
        card_item->deleteLater();
    foreach (CardItem *card_item, down_items)
        card_item->deleteLater();

    up_items.clear();
    down_items.clear();

    hide();
}

void GuanxingBox::reply() {
    QList<int> up_cards, down_cards;
    foreach (CardItem *card_item, up_items)
        up_cards << card_item->getCard()->getId();

    foreach (CardItem *card_item, down_items)
        down_cards << card_item->getCard()->getId();

    ClientInstance->onPlayerReplyGuanxing(up_cards, down_cards);
    clear();
}
