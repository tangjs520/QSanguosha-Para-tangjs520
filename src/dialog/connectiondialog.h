#ifndef _CONNECTION_DIALOG_H
#define _CONNECTION_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QComboBox>
#include <QButtonGroup>

class Detector;

namespace Ui {
    class ConnectionDialog;
}

class General;
class BackgroundRunner;

class ConnectionDialog: public QDialog {
    Q_OBJECT

public:
    ConnectionDialog(QWidget *parent);
    ~ConnectionDialog();
    void hideAvatarList();
    void showAvatarList();

protected:
    void showEvent(QShowEvent *event);

private:
    void backLoadAvatarIcons();

private:
    Ui::ConnectionDialog *ui;

    bool m_isClosed;
    BackgroundRunner *m_bgRunner;
    friend class AvatarLoader;

private slots:
    void on_detectLANButton_clicked();
    void on_clearHistoryButton_clicked();
    void on_avatarList_itemDoubleClicked(QListWidgetItem *item);
    void on_changeAvatarButton_clicked();
    void on_connectButton_clicked();
    void dialog_closed();
};

class AvatarLoader : public QObject
{
    Q_OBJECT

public:
    AvatarLoader(ConnectionDialog *connectDlg,
        const QList<const General *> &generals, int generalCount,
        int avatarIconCount) : m_connectDlg(connectDlg), m_generals(generals),
        m_generalCount(generalCount), m_avatarIconCount(avatarIconCount) {
    }

public slots:
    void loadAvatarIcons();

private:
    ConnectionDialog *m_connectDlg;
    const QList<const General *> m_generals;
    const int m_generalCount;
    int m_avatarIconCount;

signals:
    void slot_finished();
};

class UdpDetectorDialog: public QDialog {
    Q_OBJECT

public:
    UdpDetectorDialog(QDialog *parent);

private:
    QListWidget *list;
    Detector *detector;
    QPushButton *detect_button;

private slots:
    void startDetection();
    void stopDetection();
    void chooseAddress(QListWidgetItem *item);
    void addServerAddress(const QString &server_name, const QString &address);

signals:
    void address_chosen(const QString &address);
};

#endif
