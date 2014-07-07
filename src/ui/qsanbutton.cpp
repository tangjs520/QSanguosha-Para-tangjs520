#include "qsanbutton.h"
#include "clientplayer.h"
#include "SkinBank.h"
#include "engine.h"

#include <QPixmap>
#include <qbitmap.h>
#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>
#include <QGraphicsView>

QSanButton::QSanButton(QGraphicsItem *const parent)
    : QGraphicsObject(parent)
{
    _init(QSize(0, 0));
}

QSanButton::QSanButton(const QString &groupName,
    const QString &buttonName,
    QGraphicsItem *const parent)
    : QGraphicsObject(parent), m_groupName(groupName),
    m_buttonName(buttonName)
{
    for (int i = 0; i < (int)S_NUM_BUTTON_STATES; ++i) {
        m_bgPixmap[i] = G_ROOM_SKIN.getButtonPixmap(m_groupName,
            m_buttonName, (const ButtonState &)i);
    }

    _init(m_bgPixmap[0].size());
}

void QSanButton::_init(const QSize &size)
{
    m_state = S_STATE_UP;
    m_style = S_STYLE_PUSH;
    m_mouseEntered = false;

    setSize(size);
    setAcceptsHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    connect(this, SIGNAL(visibleChanged()), this, SLOT(visible_changed()));
}

void QSanButton::click()
{
    if (isEnabled()) {
        _onMouseClick(true);
    }
}

bool QSanButton::isMouseInside() const
{
    QGraphicsScene *scenePtr = scene();
    if (NULL == scenePtr) {
        return false;
    }

    QPoint cursorPos = QCursor::pos();
    foreach (QGraphicsView *view, scenePtr->views()) {
        QPointF pos = mapFromScene(view->mapToScene(view->mapFromGlobal(cursorPos)));
        if (_isMouseInside(pos)) {
            return true;
        }
    }

    return false;
}

QRectF QSanButton::boundingRect() const
{
    return QRectF(0, 0, m_size.width(), m_size.height());
}

void QSanButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->drawPixmap(0, 0, m_bgPixmap[(int)m_state]);
}

void QSanButton::setSize(const QSize &newSize)
{
    m_size = newSize;
    if (m_size.width() == 0 || m_size.height() == 0) {
        m_mask = QRegion();
        return;
    }

    const QPixmap &pixmap = m_bgPixmap[0];
    m_mask = QRegion(pixmap.mask().scaled(newSize));
}

void QSanButton::setEnabled(bool enabled)
{
    bool changed = (enabled != isEnabled());
    if (!changed) {
        return;
    }

    if (enabled) {
        setState(S_STATE_UP);
        m_mouseEntered = false;
    }
    else {
        setState(S_STATE_DISABLED);
    }

    QGraphicsObject::setEnabled(enabled);

    emit enable_changed();
}

void QSanButton::setState(const ButtonState &state)
{
    if (m_state != state) {
        m_state = state;
        update();
    }
}

void QSanButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (m_state == S_STATE_DISABLED) {
        return;
    }

    if (m_mouseEntered || !_isMouseInside(event->pos())) {
        // fake event
        return;
    }

    m_mouseEntered = true;
    if (m_state == S_STATE_UP) {
        setState(S_STATE_HOVER);
    }
}

void QSanButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    if (m_state == S_STATE_DISABLED) {
        return;
    }
    if (!m_mouseEntered) {
        return;
    }

    if (m_state == S_STATE_HOVER) {
        setState(S_STATE_UP);
    }
    m_mouseEntered = false;
}

void QSanButton::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (_isMouseInside(event->pos())) {
        if (!m_mouseEntered) {
            hoverEnterEvent(event);
        }
    }
    else {
        if (m_mouseEntered) {
            hoverLeaveEvent(event);
        }
    }
}

void QSanButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_style == S_STYLE_TOGGLE) {
        return;
    }

    //如果左键按下时鼠标处于透明区域，则忽略之
    if (!_isMouseInside(event->pos())) {
        return;
    }

    setState(S_STATE_DOWN);
}

void QSanButton::_onMouseClick(bool mouseInside)
{
    if (m_style == S_STYLE_PUSH) {
        if (mouseInside) {
            setState(S_STATE_HOVER);
        }
        else {
            setState(S_STATE_UP);
        }
    }
    else if (m_style == S_STYLE_TOGGLE) {
        if (m_state == S_STATE_HOVER) {
            // temporarily set, do not use setState!
            m_state = S_STATE_UP;
        }

        if (m_state == S_STATE_DOWN && mouseInside) {
            m_state = S_STATE_HOVER;
        }
        else if (m_state == S_STATE_UP && mouseInside) {
            m_state = S_STATE_DOWN;
        }

        update();
    }

    if (mouseInside) {
        emit clicked();
    }
    else {
        m_mouseEntered = false;
        emit clicked_mouse_outside();
    }
}

void QSanButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    bool mouseInside = _isMouseInside(event->pos());
    _onMouseClick(mouseInside);
}

void QSanButton::visible_changed()
{
    if (!isVisible() && S_STYLE_PUSH == m_style && S_STATE_HOVER == m_state) {
        m_mouseEntered = false;
        setState(S_STATE_UP);
    }
}

void QSanButton::redraw()
{
    for (int i = 0; i < (int)S_NUM_BUTTON_STATES; ++i) {
        m_bgPixmap[i] = G_ROOM_SKIN.getButtonPixmap(m_groupName,
            m_buttonName, (const ButtonState &)i);
    }

    setSize(m_bgPixmap[0].size());
}

QSanSkillButton::QSanSkillButton(QGraphicsItem *parent)
    : QSanButton(parent)
{
    _m_emitActivateSignal = false;
    _m_emitDeactivateSignal = false;
    _m_canEnable = true;
    _m_canDisable = true;
    _m_skill = NULL;
    _m_viewAsSkill = NULL;
    connect(this, SIGNAL(clicked()), this, SLOT(onMouseClick()));
    _m_skill = NULL;
}

void QSanSkillButton::_setSkillType(SkillType type) {
    _m_skillType = type;
}

void QSanSkillButton::onMouseClick() {
    if (_m_skill == NULL) return;
    if ((m_style == S_STYLE_TOGGLE && isDown() && _m_emitActivateSignal) || m_style == S_STYLE_PUSH) {
        emit skill_activated();
        emit skill_activated(_m_skill);
    } else if (!isDown() && _m_emitDeactivateSignal) {
        emit skill_deactivated();
        emit skill_deactivated(_m_skill);
    }
}

void QSanSkillButton::setSkill(const Skill *skill) {
     Q_ASSERT(skill != NULL);
     _m_skill = skill;
     // This is a nasty trick because the server side decides to choose a nasty design
     // such that sometimes the actual viewas skill is nested inside a trigger skill.
     // Since the trigger skill is not relevant, we flatten it before we create the button.
     _m_viewAsSkill = ViewAsSkill::parseViewAsSkill(_m_skill);
     if (skill == NULL) skill = _m_skill;

     Skill::Frequency freq = skill->getFrequency();
     if (freq == Skill::Frequent
         || (freq == Skill::NotFrequent && skill->inherits("TriggerSkill") && !skill->inherits("WeaponSkill")
             && !skill->inherits("ArmorSkill") && _m_viewAsSkill == NULL)) {
         setStyle(QSanButton::S_STYLE_TOGGLE);
         setState(freq == Skill::Frequent ? QSanButton::S_STATE_DOWN : QSanButton::S_STATE_UP);
         _setSkillType(QSanInvokeSkillButton::S_SKILL_FREQUENT);
         _m_emitActivateSignal = false;
         _m_emitDeactivateSignal = false;
         _m_canEnable = true;
         _m_canDisable = false;
     } else if (freq == Skill::Limited || freq == Skill::NotFrequent) {
         setState(QSanButton::S_STATE_DISABLED);
         if (skill->isAttachedLordSkill())
             _setSkillType(QSanInvokeSkillButton::S_SKILL_ATTACHEDLORD);
         else if (freq == Skill::Limited)
             _setSkillType(QSanInvokeSkillButton::S_SKILL_ONEOFF_SPELL);
         else
             _setSkillType(QSanInvokeSkillButton::S_SKILL_PROACTIVE);

         setStyle(QSanButton::S_STYLE_TOGGLE);

         _m_emitDeactivateSignal = true;
         _m_emitActivateSignal = true;
         _m_canEnable = true;
         _m_canDisable = true;
     } else if (freq == Skill::Wake) {
         setState(QSanButton::S_STATE_DISABLED);
         setStyle(QSanButton::S_STYLE_PUSH);
         _setSkillType(QSanInvokeSkillButton::S_SKILL_AWAKEN);
         _m_emitActivateSignal = false;
         _m_emitDeactivateSignal = false;
         _m_canEnable = true;
         _m_canDisable = true;
     } else if (freq == Skill::Compulsory) { // we have to set it in such way for WeiDi
         setState(QSanButton::S_STATE_UP);
         setStyle(QSanButton::S_STYLE_PUSH);
         _setSkillType(QSanInvokeSkillButton::S_SKILL_COMPULSORY);
         _m_emitActivateSignal = false;
         _m_emitDeactivateSignal = false;
         _m_canEnable = true;
         _m_canDisable = true;
     } else Q_ASSERT(false);
     setToolTip(skill->getDescription());

     Q_ASSERT((int)_m_skillType <= 5 && m_state <= 3);
     _repaint();
}

void QSanInvokeSkillButton::_repaint() {
    for (int i = 0; i < (int)S_NUM_BUTTON_STATES; ++i) {
        m_bgPixmap[i] = G_ROOM_SKIN.getSkillButtonPixmap((ButtonState)i, _m_skillType, _m_enumWidth);
        Q_ASSERT(!m_bgPixmap[i].isNull());
        const SanShadowTextFont &font = G_DASHBOARD_LAYOUT.getSkillTextFont((ButtonState)i, _m_skillType, _m_enumWidth);
        QPainter painter(&m_bgPixmap[i]);
        QString skillName = Sanguosha->translate(_m_skill->objectName());
        if (_m_enumWidth != S_WIDTH_WIDE) skillName = skillName.left(2);
        font.paintText(&painter,
                       (ButtonState)i == S_STATE_DOWN ? G_DASHBOARD_LAYOUT.m_skillTextAreaDown[_m_enumWidth] :
                                                        G_DASHBOARD_LAYOUT.m_skillTextArea[_m_enumWidth],
                       Qt::AlignCenter, skillName);
    }
    setSize(m_bgPixmap[0].size());
}

QSanSkillButton *QSanInvokeSkillDock::addSkillButtonByName(const QString &skillName) {
    Q_ASSERT(getSkillButtonByName(skillName) == NULL);
    QSanInvokeSkillButton *button = new QSanInvokeSkillButton(this);

    const Skill *skill = Sanguosha->getSkill(skillName);
    button->setSkill(skill);
    connect(button, SIGNAL(skill_activated(const Skill *)), this, SIGNAL(skill_activated(const Skill *)));
    connect(button, SIGNAL(skill_deactivated(const Skill *)), this, SIGNAL(skill_deactivated(const Skill *)));
    _m_buttons.append(button);
    update();
    return button;
}

int QSanInvokeSkillDock::width() const{
    return _m_width;
}

int QSanInvokeSkillDock::height() const{
    return ((_m_buttons.length() - 1) / 3 + 1) * G_DASHBOARD_LAYOUT.m_skillButtonsSize[0].height();
}

void QSanInvokeSkillDock::setWidth(int width) {
    _m_width = width;
}

#include "roomscene.h"
void QSanInvokeSkillDock::update() {
    if (!_m_buttons.isEmpty()) {
        QList<QSanInvokeSkillButton *> regular_buttons, lordskill_buttons, all_buttons;
        foreach (QSanInvokeSkillButton *btn, _m_buttons) {
            if (btn->getSkill()->isAttachedLordSkill())
                lordskill_buttons << btn;
            else
                regular_buttons << btn;
        }
        all_buttons = regular_buttons + lordskill_buttons;

        int numButtons = regular_buttons.length();
        int lordskillNum = lordskill_buttons.length();
        Q_ASSERT(lordskillNum <= 6); // HuangTian, ZhiBa and XianSi
        int rows = (numButtons == 0) ? 0 : (numButtons - 1) / 3 + 1;
        int rowH = G_DASHBOARD_LAYOUT.m_skillButtonsSize[0].height();
        int *btnNum = new int[rows + 2 + 1]; // we allocate one more row in case we need it.
        int remainingBtns = numButtons;
        for (int i = 0; i < rows; ++i) {
            btnNum[i] = qMin(3, remainingBtns);
            remainingBtns -= 3;
        }
        if (lordskillNum > 3) {
            int half = lordskillNum / 2;
            btnNum[rows] = half;
            btnNum[rows + 1] = lordskillNum - half;
        } else if (lordskillNum > 0) {
            btnNum[rows] = lordskillNum;
        }

        // If the buttons in rows are 3, 1, then balance them to 2, 2
        if (rows >= 2) {
            if (btnNum[rows - 1] == 1 && btnNum[rows - 2] == 3) {
                btnNum[rows - 1] = 2;
                btnNum[rows - 2] = 2;
            }
        } else if (rows == 1 && btnNum[0] == 3 && lordskillNum == 0) {
            btnNum[0] = 2;
            btnNum[1] = 1;
            rows = 2;
        }

        int m = 0;
        int x_ls = 0;
        if (lordskillNum > 0) ++x_ls;
        if (lordskillNum > 3) ++x_ls;
        for (int i = 0; i < rows + x_ls; ++i) {
            int rowTop = (RoomSceneInstance->m_skillButtonSank) ? (-rowH - 2 * (rows + x_ls - i - 1)) :
                                                                  ((-rows - x_ls + i) * rowH);
            int btnWidth = _m_width / btnNum[i];
            for (int j = 0; j < btnNum[i]; ++j) {
                QSanInvokeSkillButton *button = all_buttons[m++];
                button->setButtonWidth((QSanInvokeSkillButton::SkillButtonWidth)(btnNum[i] - 1));
                button->setPos(btnWidth * j, rowTop);
            }
        }

        delete [] btnNum;
    }
    QGraphicsObject::update();
}

QSanInvokeSkillButton *QSanInvokeSkillDock::getSkillButtonByName(const QString &skillName) const{
    foreach (QSanInvokeSkillButton *button, _m_buttons) {
        if (button->getSkill()->objectName() == skillName)
            return button;
    }
    return NULL;
}

void QSanInvokeSkillDock::removeAllSkillButtons()
{
    foreach (QSanInvokeSkillButton *const &button, _m_buttons) {
        button->hide();
        button->deleteLater();
    }

    _m_buttons.clear();
}
