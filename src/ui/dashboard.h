#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include "QSanSelectableItem.h"
#include "qsanbutton.h"
#include "carditem.h"
#include "player.h"
#include "skill.h"
#include "protocol.h"
#include "TimedProgressBar.h"
#include "GenericCardContainerUI.h"
#include "pixmapanimation.h"
#include "sprite.h"
#include "util.h"

#include <QPushButton>
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QLineEdit>
#include <QMutex>
#include <QPropertyAnimation>
#include <QGraphicsProxyWidget>

class Dashboard: public PlayerCardContainer {
    Q_OBJECT
    Q_ENUMS(SortType)

public:
    enum SortType { ByType, BySuit, ByNumber };

    explicit Dashboard(QGraphicsPixmapItem *button_widget);

    virtual QRectF boundingRect() const;
    void setWidth(int width);
    int getMiddleWidth();

    void hideControlButtons();
    virtual void showProgressBar(const QSanProtocol::Countdown &countdown);

    QRectF getProgressBarSceneBoundingRect() const {
        return _m_progressBarItem->sceneBoundingRect();
    }
    QRectF getAvatarAreaSceneBoundingRect() const {
        return _m_rightFrame->sceneBoundingRect();
    }

    QSanSkillButton *removeSkillButton(const QString &skillName);
    QSanSkillButton *addSkillButton(const QString &skillName);
    void removeAllSkillButtons();

    bool isAvatarUnderMouse();

    void highlightEquip(QString skillName, bool hightlight);

    void setTrust(bool trust);
    virtual void killPlayer();

    //由于增加了"切换全幅界面"的功能，因此dashboard也需要重载此函数来重绘自己的特殊部件
    virtual void repaintAll();

    void selectCard(const QString &pattern, bool forward = true, bool multiple = false);
    void selectEquip(int position);
    void selectOnlyCard(bool need_only = false);
    void useSelected();
    const Card *getSelected() const;
    void unselectAll(const CardItem *except = NULL);

    void disableAllCards();
    void enableCards();
    void enableAllCards();

    void adjustCards(bool playAnimation = true);

    virtual QGraphicsItem *getMouseClickReceiver() { return _m_rightFrame; }

    QList<CardItem *> removeCardItems(const QList<int> &card_ids, Player::Place place);
    virtual QList<CardItem *> cloneCardItems(QList<int> card_ids);

    // pending operations
    void startPending(const ViewAsSkill *skill);
    void stopPending();
    void updatePending();
    void clearPendings();

    void addPending(CardItem *item) { pendings << item; }
    const QList<CardItem *> &getPendings() const { return pendings; }
    bool hasHandCard(CardItem *item) const { return m_handCards.contains(item); }

    const ViewAsSkill *currentSkill() const;
    const Card *pendingCard() const;

    void expandPileCards(const QString &pile_name);
    void retractPileCards(const QString &pile_name);

    void selectCard(CardItem *item, bool isSelected);

    int getButtonWidgetWidth() const;
    int getTextureWidth() const;

    int width();
    int height();

    //获取手牌框与头像框的高度差，以允许仿照OL拉长头像区域
    int middleFrameAndRightFrameHeightDiff() const {
        return m_middleFrameAndRightFrameHeightDiff;
    }

    void showNullificationButton();
    void hideNullificationButton();

    static const int S_PENDING_OFFSET_Y = -25;

    inline void updateSkillButton() {
        if (_m_skillDock)
            _m_skillDock->update();
    }

public slots:
    virtual void updateAvatar();
    void sortCards();
    void beginSorting();
    void changeShefuState();
    void reverseSelection();
    void cancelNullification();
    void setShefuState();
    void skillButtonActivated();
    void skillButtonDeactivated();
    void selectAll();
    void controlNullificationButton(bool show);

protected:
    void _createExtraButtons();
    virtual void _adjustComponentZValues();
    virtual void addHandCards(QList<CardItem *> &cards);
    virtual QList<CardItem *> removeHandCards(const QList<int> &cardIds);

    // initialization of _m_layout is compulsory for children classes.
    inline virtual QGraphicsItem *_getEquipParent() { return _m_leftFrame; }
    inline virtual QGraphicsItem *_getDelayedTrickParent() { return _m_leftFrame; }
    inline virtual QGraphicsItem *_getAvatarParent() { return _m_rightFrame; }
    inline virtual QGraphicsItem *_getMarkParent() { return _m_floatingArea; }
    inline virtual QGraphicsItem *_getPhaseParent() { return _m_floatingArea; }
    inline virtual QGraphicsItem *_getRoleComboBoxParent() { return button_widget; }
    inline virtual QGraphicsItem *_getPileParent() { return _m_rightFrame; }
    inline virtual QGraphicsItem *_getProgressBarParent() { return _m_floatingArea; }
    inline virtual QGraphicsItem *_getFocusFrameParent() { return _m_rightFrame; }
    virtual QGraphicsItem *_getDeathIconParent() { return _m_groupDeath; }

    inline virtual QString getResourceKeyName() { return QSanRoomSkin::S_SKIN_KEY_DASHBOARD; }

    virtual const SanShadowTextFont &getSkillNameFont() const {
        return G_DASHBOARD_LAYOUT.m_skillNameFont;
    }
    virtual const QRect &getSkillNameArea() const { return G_DASHBOARD_LAYOUT.m_skillNameArea; }
    virtual QGraphicsItem *_getHandCardNumParent() const { return button_widget; }
    virtual QPointF getHeroSkinContainerPosition() const;

    bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);

    //装备牌增加支持双击事件
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    //支持即使手牌被disabled了，如果当时确定按钮可用，也能双击打出的功能
    //增加此功能的起因：在虎牢关模式，当询问是否发动"武器重铸"技能时（武器牌已被disabled了），
    //希望能继续左键双击表示接受重铸，或右键双击表示不重铸而是装备该武器。
    virtual bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

    void _onCardItemDoubleClicked(CardItem *card_item);

    void _addHandCard(CardItem *card_item, bool prepend = false, const QString &footnote = QString());
    void _adjustCards();
    void _adjustCards(const QList<CardItem *> &list, int y);

    int _m_width;
    // sync objects
    QMutex m_mutex;
    QMutex m_mutexEnableCards;

    QSanButton *m_btnReverseSelection;
    QSanButton *m_btnSortHandcard;
    QSanButton *m_btnNoNullification;
    QSanButton *m_btnShefu;
    QGraphicsPixmapItem *_m_leftFrame, *_m_middleFrame, *_m_rightFrame;
    QGraphicsPixmapItem *button_widget;

    CardItem *selected;
    QList<CardItem *> m_handCards;

    QGraphicsPathItem *trusting_item;
    QGraphicsSimpleTextItem *trusting_text;

    QSanInvokeSkillDock* _m_skillDock;
    const QSanRoomSkin::DashboardLayout *_dlayout;

    //for animated effects
    EffectAnimation *animations;

    // for parts creation
    void _createLeft();
    void _createRight();
    void _createMiddle();
    void _updateFrames();

    void _paintLeftFrame();
    void _paintMiddleFrame(const QRect &rect);
    void _paintRightFrame();

    // for pendings
    QList<CardItem *> pendings;
    const Card *pending_card;
    const ViewAsSkill *view_as_skill;
    const FilterSkill *filter;
    QStringList _m_pile_expanded;

    // for equip skill/selections
    PixmapAnimation *_m_equipBorders[S_EQUIP_AREA_LENGTH];
    QSanSkillButton *_m_equipSkillBtns[S_EQUIP_AREA_LENGTH];
    bool _m_isEquipsAnimOn[S_EQUIP_AREA_LENGTH];
    QList<QSanSkillButton *> _m_button_recycle;

    void _createEquipBorderAnimations();
    void _setEquipBorderAnimation(int index, bool turnOn);

    void drawEquip(QPainter *painter, const CardItem *equip, int order);
    void setSelectedItem(CardItem *card_item);

    QMenu *_m_sort_menu;
    QMenu *_m_shefu_menu;
    QMenu *_m_carditem_context_menu;

    //保存当前移进Dashboard可使用的卡牌
    QList<CardItem *> _m_cardItemsAnimationFinished;
    QMutex m_mutexCardItemsAnimationFinished;

    CardItem *m_selectedEquipCardItem;
    int m_middleFrameAndRightFrameHeightDiff;

protected slots:
    virtual void _onEquipSelectChanged();

    //在播放卡牌移进Dashboard的动画过程中，玩家误操作时可产生按住卡牌后不让其移动的Bug，
    //为避免该问题，Dashboard需要重写这个槽方法
    virtual void onAnimationFinished();

    //自己的头像区不固定显示昵称，而是在鼠标悬停在上面时才显示
    virtual void onAvatarHoverEnter();

    virtual void doAvatarHoverLeave() { _m_screenNameItem->hide(); }
    virtual bool isItemUnderMouse(QGraphicsItem *item);

private slots:
    void onCardItemClicked();

    //将主界面移除的"反选"和"整理手牌"功能，转移到右键菜单来实现
    void onCardItemContextMenu();

    //增加双击手牌直接出牌的功能
    void onCardItemDoubleClicked();

    void onCardItemHover();
    void onCardItemLeaveHover();
    void onMarkChanged();

    void updateRolesForHegemony(const QString &generalName);

signals:
    void card_selected(const Card *card);
    void progressBarTimedOut();
};

#endif
