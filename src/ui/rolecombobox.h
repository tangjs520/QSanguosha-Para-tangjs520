#ifndef _ROLE_COMBO_BOX_H
#define _ROLE_COMBO_BOX_H

#include "QSanSelectableItem.h"

class RoleComboBoxItem : public QSanSelectableItem
{
    Q_OBJECT

public:
    RoleComboBoxItem(const QString &role, int number);

    const QString &getRole() const { return m_role; }
    void setRole(const QString &role);

    const QSize &getSize() const { return m_size; }

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    QString m_role;
    int m_number;
    QSize m_size;

signals:
    void clicked();
};

class RoleComboBox : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit RoleComboBox(QGraphicsItem *parent);
    ~RoleComboBox() {
        clear();
    }

    static const int S_ROLE_COMBO_BOX_GAP = 3;

    void removeRoles(const QStringList &roles);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    void fix(const QString &role);

protected:
    QString _m_fixedRole;

private:
    QList<RoleComboBoxItem *> items;
    RoleComboBoxItem *m_currentRole;
    QMap<QString, RoleComboBoxItem *> m_roleToItemMap;

    void arrangeRoles();
    void clear();

private slots:
    void collapse();
    void expand();
    void toggle();
};

#endif
