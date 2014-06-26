#ifndef _CLIENT_LOG_BOX_H
#define _CLIENT_LOG_BOX_H

#include <QTextEdit>

class ClientLogBox : public QTextEdit
{
    Q_OBJECT

public:
    explicit ClientLogBox(QWidget *parent = 0);

    void appendLog(const QString &type,
        const QString &from_general,
        const QStringList &to,
        const QString &card_str = QString(),
        const QString &arg = QString(),
        const QString &arg2 = QString());

    //先暂时仅支持"蛊惑"技能，以后如还要支持更多的技能，则需要重新考虑与之相关的底层数据结构
    const QString &getSpecialSkillLogText() const { return m_specialSkillLogText; }
    void clearSpecialSkillLogText() { m_specialSkillLogText.clear(); }

private:
    QString bold(const QString &str, QColor color) const;

    QString m_specialSkillLogText;

public slots:
    void appendLog(const QStringList &log_str);
    QString append(const QString &text);
};

#endif
