#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include "engine.h"
#include "connectiondialog.h"
#include "configdialog.h"

#include <QMainWindow>
#include <QSettings>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QGraphicsItem>

namespace Ui {
    class MainWindow;
}

class FitView;
class QGraphicsScene;
class QSystemTrayIcon;
class Server;
class QTextEdit;
class QToolButton;
class QGroupBox;
class RoomItem;
class GeneralOverview;
class CardOverview;

class BroadcastBox: public QDialog {
    Q_OBJECT

public:
    BroadcastBox(Server *server, QWidget *parent = 0);

protected:
    virtual void accept();

private:
    Server *server;
    QTextEdit *text_edit;
};

class ResourceLoader : public QObject
{
    Q_OBJECT

public slots:
    //加载与卡牌相关的动画文件(由一系列图片文件组成)
    void loadAnimations();

signals:
    void slot_finished();
};

class SceneDeleter : public QObject
{
    Q_OBJECT

public:
    explicit SceneDeleter(QGraphicsScene *scene) : m_scene(scene) {}

public slots:
    void deleteScene();

private:
    QGraphicsScene *m_scene;

signals:
    void slot_finished();
};

class MainWindow: public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setBackgroundBrush(bool center_as_origin);

    void forceRestart();
    const QString &getReplayPath() const { return m_replayPath; }
    void deleteClient();

protected:
    virtual void closeEvent(QCloseEvent *);
    virtual void keyPressEvent(QKeyEvent *event);

private:
    FitView *view;
    QGraphicsScene *scene;
    Ui::MainWindow *ui;
    ConnectionDialog *connection_dialog;
    ConfigDialog *config_dialog;
    QSystemTrayIcon *systray;

    bool m_menuBarVisible;
    QByteArray m_lastPosBeforeFullScreen;
    Server *m_server;
    Client *m_client;
    GeneralOverview *m_generalOverview;
    CardOverview *m_cardOverview;
    QString m_replayPath;

    void restoreFromConfig();

    QGraphicsItem *const _isExistItem(const QString &title) const;
    void restart();
    void closeAllDialog();
    static void backgroundLoadResources();

public slots:
    void startConnection();

private slots:
    void on_actionAbout_GPLv3_triggered();
    void on_actionAbout_Lua_triggered();
    void on_actionAbout_fmod_triggered();
    void on_actionReplay_file_convert_triggered();
    void on_actionRecord_analysis_triggered();
    void on_actionAcknowledgement_triggered();
    void on_actionBroadcast_triggered();
    void on_actionScenario_Overview_triggered();
    void on_actionRole_assign_table_triggered();
    void on_actionMinimize_to_system_tray_triggered();
    void on_actionShow_Hide_Menu_triggered();
    void on_actionFullscreen_triggered();
    void on_actionReplay_triggered();
    void on_actionAbout_triggered();
    void on_actionEnable_Hotkey_toggled(bool);
    void on_actionNever_nullify_my_trick_toggled(bool);
    void on_actionCard_Overview_triggered();
    void on_actionGeneral_Overview_triggered();
    void on_actionStart_Server_triggered();
    void on_actionExit_triggered();

    void checkVersion(const QString &server_version, const QString &server_mod);
    void networkError(const QString &error_msg);
    void enterRoom();
    void gotoScene(QGraphicsScene *scene);
    void gotoStartScene();

    void startGameInAnotherInstance();
    void changeBackground();
    void on_actionView_ban_list_triggered();
};

#endif
