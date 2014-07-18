#include "rolecombobox.h"
#include "clientstruct.h"
#include "settings.h"
#include "engine.h"

RoleComboBoxItem::RoleComboBoxItem(const QString &role, int number)
    : m_role(role), m_number(number)
{
    setRole(role);
    setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
}

void RoleComboBoxItem::setRole(const QString &role)
{
    m_role = role;

    QString rolePixmapPath = ((m_number != 0 && role != "unknown")
        || (HEGEMONY_ROLE_INDEX == m_number && role == "unknown"))
        ? QString("image/system/roles/%1-%2.png").arg(m_role).arg(m_number)
        : QString("image/system/roles/%1.png").arg(m_role);

    m_size = QPixmap(rolePixmapPath).size();

    load(rolePixmapPath, m_size, false);
}

void RoleComboBoxItem::mousePressEvent(QGraphicsSceneMouseEvent *)
{
    emit clicked();
}

RoleComboBox::RoleComboBox(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    int index = Sanguosha->getRoleIndex();
    m_currentRole = new RoleComboBoxItem("unknown", index);
    m_currentRole->setParentItem(this);
    connect(m_currentRole, SIGNAL(clicked()), this, SLOT(expand()));

    if (ServerInfo.EnableHegemony) {
        items << new RoleComboBoxItem("lord", index);
    }

    items << new RoleComboBoxItem("loyalist", index)
          << new RoleComboBoxItem("rebel", index)
          << new RoleComboBoxItem("renegade", index);

    if (ServerInfo.EnableHegemony && isNormalGameMode(ServerInfo.GameMode)) {
        items << new RoleComboBoxItem("careerist", index);
    }

    arrangeRoles();

    foreach (RoleComboBoxItem *item, items) {
        item->setZValue(1.0);
        item->setParentItem(this);
        item->hide();
        connect(item, SIGNAL(clicked()), this, SLOT(collapse()));

        m_roleToItemMap[item->getRole()] = item;
    }
}

void RoleComboBox::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

QRectF RoleComboBox::boundingRect() const
{
    if (items.isEmpty()) {
        return QRect(0, 0, 0, 0);
    }
    else {
        return items[0]->boundingRect();
    }
}

void RoleComboBox::collapse()
{
    disconnect(m_currentRole, SIGNAL(clicked()), this, SLOT(collapse()));
    connect(m_currentRole, SIGNAL(clicked()), this, SLOT(expand()));

    foreach (RoleComboBoxItem *item, items) {
        item->hide();
    }

    RoleComboBoxItem *clicked_item = qobject_cast<RoleComboBoxItem *>(sender());
    m_currentRole->setRole(clicked_item->getRole());
}

void RoleComboBox::expand()
{
    foreach (RoleComboBoxItem *item, items) {
        item->show();
    }

    m_currentRole->setRole("unknown");
    connect(m_currentRole, SIGNAL(clicked()), this, SLOT(collapse()));
}

void RoleComboBox::toggle()
{
    Q_ASSERT(!_m_fixedRole.isNull());
    if (!isEnabled()) {
        return;
    }

    QString displayed = m_currentRole->getRole();
    if (displayed == "unknown") {
        m_currentRole->setRole(_m_fixedRole);
    }
    else {
        m_currentRole->setRole("unknown");
    }
}

void RoleComboBox::fix(const QString &role)
{
    if (_m_fixedRole.isNull()) {
        disconnect(m_currentRole, SIGNAL(clicked()), this, SLOT(expand()));
        disconnect(m_currentRole, SIGNAL(clicked()), this, SLOT(collapse()));

        //单机模式下屏蔽该功能，因为固定角色后，继续允许再切换回脸谱(ROLE_UNKNOWN)没什么意义
        if (Config.HostAddress != "127.0.0.1") {
            connect(m_currentRole, SIGNAL(clicked()), this, SLOT(toggle()));
        }
    }

    m_currentRole->setRole(role);
    _m_fixedRole = role;

    clear();
}

void RoleComboBox::removeRoles(const QStringList &roles)
{
    foreach (const QString &role, roles) {
        if (m_roleToItemMap.contains(role)) {
            if (m_currentRole->getRole() == role) {
                m_currentRole->setRole("unknown");
            }

            RoleComboBoxItem *item = m_roleToItemMap[role];
            item->hide();
            delete item;

            items.removeAll(item);
            m_roleToItemMap.remove(role);
        }
    }

    arrangeRoles();
}

void RoleComboBox::arrangeRoles()
{
    for (int i = 0, count = items.length(); i < count; ++i) {
        RoleComboBoxItem *item = items.at(i);

        //由于国战role图标的尺寸比身份role图标的尺寸略大一些，因此要特殊处理一下
        if (ServerInfo.EnableHegemony) {
            const int topMargin = 1;
            item->setPos(-2, topMargin + (i + 1) * (item->getSize().height() - S_ROLE_COMBO_BOX_GAP));
        }
        else {
            item->setPos(0, (i + 1) * (item->getSize().height() + S_ROLE_COMBO_BOX_GAP));
        }
    }
}

void RoleComboBox::clear()
{
    foreach (RoleComboBoxItem *item, items) {
        delete item;
    }
    items.clear();

    m_roleToItemMap.clear();
}
