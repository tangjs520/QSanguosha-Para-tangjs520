#ifndef _CARD_ITEM_H
#define _CARD_ITEM_H

#include "QSanSelectableItem.h"

#include <QMutex>

class SanShadowTextFont;
class QAbstractAnimation;
class Card;
class General;

class CardItem : public QSanSelectableItem
{
    Q_OBJECT

public:
    explicit CardItem(const Card *card);
    explicit CardItem(const QString &generalName);
    ~CardItem();

    virtual QRectF boundingRect() const;

    const Card *getCard() const;
    void setCard(const Card *card);
    int getId() const { return m_cardId; }

    // For move card animation
    void setHomePos(const QPointF &homePosition) { m_homePos = homePosition; }
    QPointF homePos() const { return m_homePos; }

    QAbstractAnimation *getGoBackAnimation(bool doFadeEffect, bool smoothTransition = false,
                                           int duration = S_MOVE_CARD_ANIMATION_DURATION);
    void goBack(bool playAnimation, bool doFade = true);

    void setHomeOpacity(double opacity) { m_opacityAtHome = opacity; }

    void showAvatar(const General *general);
    void setAutoBack(bool autoBack) { m_autoBack = autoBack; }
    void changeGeneral(const QString &generalName);
    void setFootnote(const QString &desc);

    //在卡牌选择框通过双击卡牌选择时显示的脚注
    void setDoubleClickedFootnote(const QString &desc);

    bool isSelected() const { return m_isSelected; }
    void setSelected(bool selected) { m_isSelected = selected; }

    void showFootnote() { m_showFootnote = true; }
    void hideFootnote() { m_showFootnote = false; }

    static CardItem *FindItem(const QList<CardItem *> &items, int card_id);

    struct UiHelper {
        int tablePileClearTimeStamp;
        UiHelper() : tablePileClearTimeStamp(0) {}
    } m_uiHelper;

    void clickItem() {
        m_mouseDoubleclicked = false;
        emit released();
    }

protected:
    void initialize();

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *);

private slots:
    void animationFinished();

private:
    void paintFootnoteImage(const SanShadowTextFont &footnoteFont, const QString &desc);
    void stopAnimation();

private:
    QAbstractAnimation *m_currentAnimation;

    bool m_showFootnote;
    QMutex m_animationMutex;
    double m_opacityAtHome;
    bool m_isSelected;
    int m_cardId;
    QString m_avatarName;
    QPointF m_homePos;
    bool m_autoBack;
    bool m_mouseDoubleclicked;

    QPixmap m_backgroundPixmap;
    QPixmap m_canvas;
    QImage m_footnoteImage;

    static const int S_MOVE_CARD_ANIMATION_DURATION = 600;

signals:
    void double_clicked();
    void released();
    void enter_hover();
    void leave_hover();
    void movement_animation_finished();
    void context_menu_event_triggered();
};

#endif
