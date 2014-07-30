#ifndef _QSAN_BUTTON_H
#define _QSAN_BUTTON_H

#include <QGraphicsObject>
#include <QPixmap>
#include <QRegion>
#include <qrect.h>
#include <qlist.h>
#include "skill.h"

class QSanButton : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit QSanButton(QGraphicsItem *const parent);
    QSanButton(const QString &groupName, const QString &buttonName, QGraphicsItem *const parent);

    enum ButtonState {
        S_STATE_UP,
        S_STATE_HOVER,
        S_STATE_DOWN,
        S_STATE_DISABLED,
        S_NUM_BUTTON_STATES
    };

    enum ButtonStyle {
        S_STYLE_PUSH,
        S_STYLE_TOGGLE
    };

    const ButtonState &getState() const { return m_state; }
    const ButtonStyle &getStyle() const { return m_style; }

    bool isDown() const { return (m_state == S_STATE_DOWN); }
    bool isMouseInside() const;

    void setStyle(const ButtonStyle &style) { m_style = style; }
    void setState(const ButtonState &state);
    void setEnabled(bool enabled);
    void setSize(const QSize &size);
    void setRect(const QRect &rect) {
        setSize(rect.size()); setPos(rect.topLeft());
    }

    void redraw();

    //模拟鼠标左键单击
    void click();

    virtual QRectF boundingRect() const;

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

    //由于图片存在透明区域，所以不能只是简单地重载hoverEnterEvent和hoverLeaveEvent，
    //而需要配合hoverMoveEvent和isMouseInside来综合处理鼠标悬停事件
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void _onMouseClick(bool mouseInside);

    QString m_groupName;
    QString m_buttonName;
    ButtonState m_state;
    ButtonStyle m_style;
    QRegion m_mask;
    QSize m_size;
    QPixmap m_bgPixmap[S_NUM_BUTTON_STATES];

    // @todo: currently this is an extremely dirty hack. Refactor the button states to
    // get rid of it.
    bool m_mouseEntered;

private slots:
    void visible_changed();

private:
    void _init(const QSize &size);

    //采用掩码图片所在的区域来判断鼠标是否在按钮上
    //如果直接使用QGraphicsItem::isUnderMouse函数来判断的话，
    //会导致当鼠标在图片透明区域时也会认为鼠标已在按钮上了，
    //而此时实际给人的感觉是鼠标还未在按钮上
    bool _isMouseInside(const QPointF &pos) const {
        return m_mask.contains(QPoint(pos.x(), pos.y()));
    }

signals:
    void clicked();
    void clicked_mouse_outside();
    void enable_changed();
};

class QSanSkillButton: public QSanButton {
    Q_OBJECT

public:
    enum SkillType { S_SKILL_ATTACHEDLORD, S_SKILL_PROACTIVE, S_SKILL_FREQUENT, S_SKILL_COMPULSORY,
                     S_SKILL_AWAKEN, S_SKILL_ONEOFF_SPELL, S_NUM_SKILL_TYPES };

    static QString getSkillTypeString(SkillType type) {
        static QMap<SkillType, QString> skillType2String;
        if (skillType2String.isEmpty()) {
            skillType2String[QSanSkillButton::S_SKILL_AWAKEN] = "awaken";
            skillType2String[QSanSkillButton::S_SKILL_COMPULSORY] = "compulsory";
            skillType2String[QSanSkillButton::S_SKILL_FREQUENT] = "frequent";
            skillType2String[QSanSkillButton::S_SKILL_ONEOFF_SPELL] = "oneoff";
            skillType2String[QSanSkillButton::S_SKILL_PROACTIVE] = "proactive";
            skillType2String[QSanSkillButton::S_SKILL_ATTACHEDLORD] = "attachedlord";
        }

        return skillType2String[type];
    }

    virtual void setSkill(const Skill *skill);
    virtual const Skill *getSkill() const{ return _m_skill; }
    virtual void setEnabled(bool enabled) {
        if (!_m_canEnable && enabled) return;
        if (!_m_canDisable && !enabled) return;
        QSanButton::setEnabled(enabled);
    }
    QSanSkillButton(QGraphicsItem *parent = NULL);
    const ViewAsSkill *getViewAsSkill() const{ return _m_viewAsSkill; }

protected:
    virtual void _setSkillType(SkillType type);
    virtual void _repaint() = 0;
    const ViewAsSkill *_parseViewAsSkill() const;
    SkillType _m_skillType;
    bool _m_emitActivateSignal;
    bool _m_emitDeactivateSignal;
    bool _m_canEnable;
    bool _m_canDisable;
    const Skill *_m_skill;
    const ViewAsSkill *_m_viewAsSkill;

protected slots:
    void onMouseClick();

signals:
    void skill_activated(const Skill *);
    void skill_activated();
    void skill_deactivated(const Skill *);
    void skill_deactivated();
};

class QSanInvokeSkillButton: public QSanSkillButton {
    Q_OBJECT

public:
    QSanInvokeSkillButton(QGraphicsItem *parent = NULL): QSanSkillButton(parent) {
        _m_enumWidth = S_WIDTH_NARROW;
    }
    enum SkillButtonWidth { S_WIDTH_WIDE, S_WIDTH_MED, S_WIDTH_NARROW, S_NUM_BUTTON_WIDTHS };
    void setButtonWidth(SkillButtonWidth width) { _m_enumWidth = width; _repaint(); }
    SkillButtonWidth getButtonWidth() { return _m_enumWidth; }

protected:
    // this function does not update the button's bg and is therefore not exposed to outside
    // classes.
    virtual void _repaint();

    SkillButtonWidth _m_enumWidth;
};

class QSanInvokeSkillDock: public QGraphicsObject {
    Q_OBJECT

public:
    QSanInvokeSkillDock(QGraphicsItem *parent) : QGraphicsObject(parent), _m_width(0) {}
    ~QSanInvokeSkillDock() { removeAllSkillButtons(); }

    int width() const;
    int height() const;
    void setWidth(int width);

    void removeSkillButton(QSanInvokeSkillButton *button) {
        if (button == NULL) return;
        _m_buttons.removeAll(button);
        disconnect(button);
    }

    // Any one who call the following functions are responsible for
    // destroying the buttons returned
    QSanSkillButton *addSkillButtonByName(const QString &skillName);

    QSanSkillButton *removeSkillButtonByName(const QString &skillName) {
        QSanInvokeSkillButton *button = getSkillButtonByName(skillName);
        if (button != NULL) removeSkillButton(button);
        update();
        return button;
    }

    void removeAllSkillButtons();

    QSanInvokeSkillButton *getSkillButtonByName(const QString &skillName) const;
    void update();
    virtual QRectF boundingRect() const{ return QRectF(0, -height(), width(), height()); }

    const QList<QSanInvokeSkillButton *> &getAllSkillButtons() { return _m_buttons; }

protected:
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
    QList<QSanInvokeSkillButton *> _m_buttons;
    int _m_width;

signals:
    void skill_activated(const Skill *skill);
    void skill_deactivated(const Skill *skill);
};

#endif
