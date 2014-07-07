#include "photo.h"
#include "clientplayer.h"
#include "settings.h"
#include "carditem.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "playercarddialog.h"
#include "rolecombobox.h"
#include "SkinBank.h"
#include "roomscene.h"
#include "heroskincontainer.h"
#include "jsonutils.h"
#include "pixmapanimation.h"

#include <QPainter>
#include <QDrag>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMessageBox>
#include <QMainWindow>
#include <QGraphicsProxyWidget>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QMenu>
#include <QFile>

using namespace QSanProtocol;
using namespace QSanProtocol::Utils;

// skins that remain to be extracted:
// equips
// mark
// emotions
// hp
// seatNumber
// death logo
// kingdom mask and kingdom icon (decouple from player)
// make layers (drawing order) configurable

Photo::Photo(): PlayerCardContainer() {
    _m_mainFrame = NULL;
    m_player = NULL;
    _m_focusFrame = NULL;
    _m_onlineStatusItem = NULL;
    _m_layout = &G_PHOTO_LAYOUT;
    _m_frameType = S_FRAME_NO_FRAME;
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

    _m_skillNameItem = new QGraphicsPixmapItem(_m_groupMain);
    emotion_item = new Sprite(_m_groupMain);
    _m_duanchangMask = new QGraphicsRectItem(_m_groupMain);
    _m_duanchangMask->setRect(boundingRect());
    _m_duanchangMask->setZValue(32767.0);
    _m_duanchangMask->setOpacity(0.4);
    _m_duanchangMask->hide();
    QBrush duanchang_brush(G_PHOTO_LAYOUT.m_duanchangMaskColor);
    _m_duanchangMask->setBrush(duanchang_brush);

    _createControls();
}

void Photo::refresh() {
    const char *mainFramePixmapKey = m_player ? QSanRoomSkin::S_SKIN_KEY_MAINFRAME
        : QSanRoomSkin::S_SKIN_KEY_BLANK_GENERAL;
    //之所以要将“photo-blank.png”的显示范围扩大，是因为"空位"图片
    //比"photo-back.png"要小，且四周都是透明色，如果不扩大范围，
    //当用“photo-blank.png”覆盖"photo-back.png"后，界面上就会在“空位”图片
    //周围留下"photo-back.png"的残影
    QRect mainFrameRect = m_player ? G_PHOTO_LAYOUT.m_mainFrameArea
        : G_PHOTO_LAYOUT.m_mainFrameArea.adjusted(-2, -2, 2, 2);
    _paintPixmap(_m_mainFrame, mainFrameRect, mainFramePixmapKey);

    PlayerCardContainer::refresh();
    if (!m_player) return;
    QString state_str = m_player->getState();

    //电脑玩家没必要再显示"电脑"屏幕名
    if (!state_str.isEmpty() && state_str != "online" && state_str != "robot") {
        QRect rect = G_PHOTO_LAYOUT.m_onlineStatusArea;
        QImage image(rect.size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.fillRect(QRect(0, 0, rect.width(), rect.height()), G_PHOTO_LAYOUT.m_onlineStatusBgColor);
        G_PHOTO_LAYOUT.m_onlineStatusFont.paintText(&painter, QRect(QPoint(0, 0), rect.size()),
                                                    Qt::AlignCenter,
                                                    Sanguosha->translate(state_str));
        QPixmap pixmap = QPixmap::fromImage(image);
        _paintPixmap(_m_onlineStatusItem, rect, pixmap, _m_groupMain);
        _layBetween(_m_onlineStatusItem, _m_mainFrame, _m_chainIcon);
        if (!_m_onlineStatusItem->isVisible()) _m_onlineStatusItem->show();
    } else if (_m_onlineStatusItem != NULL && state_str == "online")
        _m_onlineStatusItem->hide();
}

QRectF Photo::boundingRect() const{
    return QRect(0, 0, G_PHOTO_LAYOUT.m_normalWidth, G_PHOTO_LAYOUT.m_normalHeight);
}

void Photo::repaintAll() {
    resetTransform();
    setTransform(QTransform::fromTranslate(-G_PHOTO_LAYOUT.m_normalWidth / 2, -G_PHOTO_LAYOUT.m_normalHeight / 2), true);
    setFrame(_m_frameType);

    PlayerCardContainer::repaintAll();
    refresh();
}

void Photo::_adjustComponentZValues() {
    PlayerCardContainer::_adjustComponentZValues();

    _layBetween(_m_mainFrame, _m_faceTurnedIcon, _m_equipRegions[3]);
    _layBetween(emotion_item, _m_chainIcon, _m_roleComboBox);
    _layBetween(_m_skillNameItem, _m_chainIcon, _m_roleComboBox);

    _m_progressBarItem->setZValue(_m_groupMain->zValue() + 1);
}

void Photo::setEmotion(const QString &emotion, bool permanent) {
    if (emotion == ".") {
        hideEmotion();
        return;
    }

    QString path = QString("image/system/emotion/%1.png").arg(emotion);
    if (QFile::exists(path)) {
        QPixmap pixmap = QPixmap(path);
        emotion_item->setPixmap(pixmap);
        emotion_item->setPos((G_PHOTO_LAYOUT.m_normalWidth - pixmap.width()) / 2,
                             (G_PHOTO_LAYOUT.m_normalHeight - pixmap.height()) / 2);
        _layBetween(emotion_item, _m_chainIcon, _m_roleComboBox);

        QPropertyAnimation *appear = new QPropertyAnimation(emotion_item, "opacity", this);
        appear->setStartValue(0.0);
        if (permanent) {
            appear->setEndValue(1.0);
            appear->setDuration(500);
        } else {
            appear->setKeyValueAt(0.25, 1.0);
            appear->setKeyValueAt(0.75, 1.0);
            appear->setEndValue(0.0);
            appear->setDuration(2000);
        }
        appear->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        PixmapAnimation::GetPixmapAnimation(this, emotion);
    }
}

void Photo::tremble() {
    QPropertyAnimation *vibrate = new QPropertyAnimation(this, "x", this);
    static qreal offset = 20;

    vibrate->setKeyValueAt(0.5, x() - offset);
    vibrate->setEndValue(x());
    vibrate->setEasingCurve(QEasingCurve::OutInBounce);

    vibrate->start(QAbstractAnimation::DeleteWhenStopped);
}

void Photo::hideEmotion() {
    QPropertyAnimation *disappear = new QPropertyAnimation(emotion_item, "opacity", this);
    disappear->setStartValue(1.0);
    disappear->setEndValue(0.0);
    disappear->setDuration(500);
    disappear->start(QAbstractAnimation::DeleteWhenStopped);
}

void Photo::updateDuanchang() {
    if (!m_player) return;
    _m_duanchangMask->setVisible(m_player->getMark("@duanchang") > 0);
}

QList<CardItem *> Photo::removeCardItems(const QList<int> &card_ids, Player::Place place) {
    QList<CardItem *> result;
    if (place == Player::PlaceHand || place == Player::PlaceSpecial) {
        result = _createCards(card_ids);
        updateHandcardNum();
    } else if (place == Player::PlaceEquip) {
        result = removeEquips(card_ids);
    } else if (place == Player::PlaceDelayedTrick) {
        result = removeDelayedTricks(card_ids);
    }

    // if it is just one card from equip or judge area, we'd like to keep them
    // to start from the equip/trick icon.
    if (result.size() > 1 || (place != Player::PlaceEquip && place != Player::PlaceDelayedTrick))
        _disperseCards(result, G_PHOTO_LAYOUT.m_cardMoveRegion, Qt::AlignCenter, false, false);

    update();
    return result;
}

bool Photo::_addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo) {
    _disperseCards(card_items, G_PHOTO_LAYOUT.m_cardMoveRegion, Qt::AlignCenter, true, false);

    double homeOpacity = 0.0;
    bool destroy = true;
    Player::Place place = moveInfo.to_place;

    foreach (CardItem *card_item, card_items)
        card_item->setHomeOpacity(homeOpacity);
    if (place == Player::PlaceEquip) {
        addEquips(card_items);
        destroy = false;
    } else if (place == Player::PlaceDelayedTrick) {
        addDelayedTricks(card_items);
        destroy = false;
    } else if (place == Player::PlaceHand) {
        updateHandcardNum();
    }

    return destroy;
}

void Photo::setFrame(FrameType type) {
    _m_frameType = type;
    if (type == S_FRAME_NO_FRAME) {
        if (_m_focusFrame) {
            if (_m_saveMeIcon && _m_saveMeIcon->isVisible())
                setFrame(S_FRAME_SOS);
            else if (m_player->getPhase() != Player::NotActive)
                setFrame(S_FRAME_PLAYING);
            else
                _m_focusFrame->hide();
        }
    } else {
        _paintPixmap(_m_focusFrame, G_PHOTO_LAYOUT.m_focusFrameArea,
                     _getPixmap(QSanRoomSkin::S_SKIN_KEY_FOCUS_FRAME, QString::number(type)),
                     _m_groupMain);
        _layBetween(_m_focusFrame, _m_avatarArea, _m_mainFrame);
        _m_focusFrame->show();
    }

    update();
}

void Photo::updatePhase() {
    PlayerCardContainer::updatePhase();

    if (m_player->getPhase() != Player::NotActive)
        setFrame(S_FRAME_PLAYING);
    else
        setFrame(S_FRAME_NO_FRAME);
}

void Photo::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

QGraphicsItem *Photo::getMouseClickReceiver() {
    return this;
}

QPointF Photo::getHeroSkinContainerPosition() const
{
    QRectF tableRect = RoomSceneInstance->getTableRect();

    QRectF photoRect = sceneBoundingRect();
    int photoWidth = photoRect.width();
    int photoHeight = photoRect.height();

    QRectF heroSkinContainerRect = (m_primaryHeroSkinContainer != NULL)
        ? m_primaryHeroSkinContainer->boundingRect()
        : m_secondaryHeroSkinContainer->boundingRect();
    int heroSkinContainerWidth = heroSkinContainerRect.width();
    int heroSkinContainerHeight = heroSkinContainerRect.height();

    const int tablePadding = 5;

    //左边区域
    if (photoRect.right() <= tableRect.left()) {
        QPointF result(photoRect.right() + 10, photoRect.top()
            - ((heroSkinContainerHeight - photoHeight) / 2));

        int yBottomDiff = (result.y() + heroSkinContainerHeight)
            - (tableRect.bottom() + tablePadding);
        if (yBottomDiff > 0) {
            result.setY(result.y() - yBottomDiff);
        }
        else if (result.y() < tableRect.top() - tablePadding) {
            result.setY(tableRect.top() - tablePadding);
        }

        return result;
    }
    //上边区域
    else if (photoRect.bottom() <= tableRect.top()) {
        QPointF result(photoRect.left()
            - ((heroSkinContainerWidth - photoWidth) / 2), photoRect.bottom() + 10);

        int xRightDiff = (result.x() + heroSkinContainerWidth)
            - (tableRect.right() + tablePadding);
        if (xRightDiff > 0) {
            result.setX(result.x() - xRightDiff);
        }
        else if (result.x() < tableRect.left() - tablePadding) {
            result.setX(tableRect.left() - tablePadding);
        }

        return result;
    }
    //右边区域
    else {
        QPointF result(photoRect.left() - heroSkinContainerWidth - 10, photoRect.top()
            - ((heroSkinContainerHeight - photoHeight) / 2));

        int yBottomDiff = (result.y() + heroSkinContainerHeight)
            - (tableRect.bottom() + tablePadding);
        if (yBottomDiff > 0) {
            result.setY(result.y() - yBottomDiff);
        }
        else if (result.y() < tableRect.top() - tablePadding) {
            result.setY(tableRect.top() - tablePadding);
        }

        return result;
    }
}

void Photo::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (m_player && Self && Self->isOwner()
        && (m_player->getState() == "online" || m_player->getState() == "trust")) {
        QMenu menu;
        QAction *kickPlayerAction = menu.addAction(tr("Kick Player"));
        QAction *selectedAction = menu.exec(event->screenPos());
        if (selectedAction == kickPlayerAction) {
            QMessageBox::StandardButton choice = QMessageBox::question(RoomSceneInstance->mainWindow(),
                tr("Sanguosha"),
                tr("Are you sure to kick player: [%1]?").arg(m_player->screenName()),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Cancel);
            //下面对m_player是否为空的判断，是为了解决
            //“在弹框期间，如果被踢的玩家已经主动离线了，然后选择确定后，程序崩溃”的问题
            if (choice == QMessageBox::Ok && m_player != NULL) {
                ClientInstance->notifyServer(S_COMMAND_KICK_PLAYER, toJsonString(m_player->objectName()));
            }
        }
    }
}
