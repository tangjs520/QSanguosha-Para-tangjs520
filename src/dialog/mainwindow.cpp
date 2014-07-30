#include "mainwindow.h"
#include "startscene.h"
#include "roomscene.h"
#include "server.h"
#include "client.h"
#include "generaloverview.h"
#include "cardoverview.h"
#include "ui_mainwindow.h"
#include "scenario-overview.h"
#include "window.h"
#include "pixmapanimation.h"
#include "record-analysis.h"
#include "audio.h"
#include "backgroundrunner.h"
#include "replayer.h"

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QVariant>
#include <QMessageBox>
#include <QTime>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDesktopServices>
#include <QLabel>
#include <QProcess>
#include <QSystemTrayIcon>

class FitView: public QGraphicsView {
public:
    FitView(QGraphicsScene *scene): QGraphicsView(scene) {
        setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing
            | QPainter::SmoothPixmapTransform);
    }

    virtual void resizeEvent(QResizeEvent *event) {
        QGraphicsView::resizeEvent(event);

        SanScene *const sanScene = qobject_cast<SanScene *const>(scene());
        if (NULL != sanScene) {
            QRectF newSceneRect = QRectF(0, 0, event->size().width(), event->size().height());
            sanScene->setSceneRect(newSceneRect);
            sanScene->adjustItems();

            QRectF adjustedSceneRect = sanScene->sceneRect();
            if (newSceneRect != adjustedSceneRect) {
                fitInView(adjustedSceneRect, Qt::KeepAspectRatioByExpanding);
            }
            else {
                resetTransform();
            }
        }
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    m_menuBarVisible = true;
    m_server = NULL;
    m_client = NULL;
    m_consoleStart = true;
    m_generalOverview = NULL;
    m_cardOverview = NULL;

    ui->setupUi(this);
    scene = NULL;

    connection_dialog = new ConnectionDialog(this);
    connect(ui->actionStart_Game, SIGNAL(triggered()), connection_dialog, SLOT(exec()));

    connect(connection_dialog, SIGNAL(accepted()), this, SLOT(setConsoleStartFalse()));
    connect(connection_dialog, SIGNAL(accepted()), this, SLOT(startConnection()));

    config_dialog = new ConfigDialog(this);
    connect(ui->actionConfigure, SIGNAL(triggered()), config_dialog, SLOT(exec()));

    connect(config_dialog, SIGNAL(bg_changed()), this, SLOT(changeBackground()));

    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionAcknowledgement_2, SIGNAL(triggered()), this, SLOT(on_actionAcknowledgement_triggered()));

    StartScene *start_scene = new StartScene;

    QList<QAction *> actions;
    actions << ui->actionStart_Server
            << ui->actionStart_Game
            << ui->actionPC_Console_Start
            << ui->actionReplay
            << ui->actionConfigure
            << ui->actionGeneral_Overview
            << ui->actionCard_Overview
            << ui->actionScenario_Overview
            << ui->actionAbout
            << ui->actionAcknowledgement;

    foreach (QAction *action, actions)
        start_scene->addButton(action);

    view = new FitView(scene);
    view->setFrameShape(QFrame::NoFrame);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setObjectName("QSanguoshaFrame");

    setCentralWidget(view);
    restoreFromConfig();

    backgroundLoadResources();

    gotoScene(start_scene);

    addAction(ui->actionShow_Hide_Menu);
    addAction(ui->actionFullscreen);

    systray = NULL;
}

void MainWindow::restoreFromConfig() {
    resize(Config.value("WindowSize", QSize(1366, 706)).toSize());
    move(Config.value("WindowPosition", QPoint(-8, -8)).toPoint());
    Qt::WindowStates window_state = static_cast<Qt::WindowStates>(Config.value("WindowState", 0).toInt());
    if (window_state & Qt::WindowMinimized) {
        window_state &= ~Qt::WindowMinimized;
    }
    setWindowState(window_state);

    QFont font;
    if (Config.UIFont != font)
        QApplication::setFont(Config.UIFont, "QTextEdit");

    ui->actionEnable_Hotkey->setChecked(Config.EnableHotKey);
    ui->actionNever_nullify_my_trick->setChecked(Config.NeverNullifyMyTrick);
    ui->actionNever_nullify_my_trick->setEnabled(false);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    int result = QMessageBox::Ok;

    if (Config.ShowMsgBoxWhenExit) {
        QMessageBox msgBox(QMessageBox::Question, tr("Sanguosha"),
            tr("Are you sure to exit?"), QMessageBox::Ok | QMessageBox::Cancel,
            this);

        QCheckBox dontShowAgainCheckBox(tr("Don't Show Again"), &msgBox);

        QGridLayout *msgBoxLayout = qobject_cast<QGridLayout *>(msgBox.layout());
        if (NULL != msgBoxLayout) {
            QMargins margins = msgBoxLayout->contentsMargins();
            margins.setBottom(5);
            msgBoxLayout->setContentsMargins(margins);
            msgBoxLayout->addWidget(&dontShowAgainCheckBox, 3, 0, 1, 2, Qt::AlignLeft);
        }

        result = msgBox.exec();

        if (dontShowAgainCheckBox.isChecked()) {
            Config.ShowMsgBoxWhenExit = false;
            Config.setValue("ShowMsgBoxWhenExit", Config.ShowMsgBoxWhenExit);
        }
    }

    if (QMessageBox::Cancel == result) {
        event->ignore();
    }
    else {
        Sanguosha->blockAllRoomSignals(true);

        delete systray;
        systray = NULL;

        event->accept();
    }

    Config.setValue("WindowSize", size());
    Config.setValue("WindowPosition", pos());
    Config.setValue("WindowState", static_cast<int>(windowState()));
}

MainWindow::~MainWindow() {
    delete ui;
    view->deleteLater();
    if (scene) {
        scene->deleteLater();
    }
    QSanSkinFactory::destroyInstance();
}

void MainWindow::gotoScene(QGraphicsScene *scene) {
    //由于已支持在游戏中随时退出的功能，经测试发现，
    //如果马上释放RoomScene对象的话，程序流程仍有可能进入到Dashboard::onAnimationFinished函数中，
    //从而导致程序崩溃。故在此使用后台线程来延时释放RoomScene对象，以避免程序崩溃。
    if (NULL != this->scene) {
        if (NULL != qobject_cast<RoomScene *>(this->scene)) {
            BackgroundRunner *const bgRunner
                = new BackgroundRunner(new SceneDeleter(this->scene));
            bgRunner->addSlot(SLOT(deleteScene()));
            bgRunner->start();
        }
        else {
            this->scene->deleteLater();
        }
    }

    this->scene = scene;
    view->setScene(scene);
    /* @todo: Need a better way to replace the magic number '4' */
    QResizeEvent e(QSize(view->size().width() - 4, view->size().height() - 4), view->size());
    view->resizeEvent(&e);
    changeBackground();
}

void MainWindow::on_actionExit_triggered() {
    close();
}

void MainWindow::on_actionStart_Server_triggered() {
    ServerDialog *dialog = new ServerDialog(this, tr("Start server"));
    if (!dialog->config())
        return;

    //需要更新Config.HostAddress的值，以避免“先单机启动，退出单机后，再启动服务器，
    //Config.HostAddress的值仍为127.0.0.1”的问题
    if (Config.HostAddress == "127.0.0.1") {
        QStringList addressList = StartScene::getHostAddresses();
        foreach (const QString &address, addressList) {
            if (address != "127.0.0.1" && !address.startsWith("169.254.")) {
                Config.HostAddress = address;
                break;
            }
        }
    }

    Server *server = new Server(this, false);
    if (!server->listen()) {
        server->deleteLater();

        QMessageBox::warning(this, tr("Warning"), tr("Can not start server!"));
        return;
    }

    m_server = server;

    server->daemonize();

    ui->actionStart_Game->disconnect();
    connect(ui->actionStart_Game, SIGNAL(triggered()), this, SLOT(startGameInAnotherInstance()));

    ui->actionStart_Server->setEnabled(false);
    ui->actionPC_Console_Start->setEnabled(false);
    ui->actionReplay->setEnabled(false);

    StartScene *start_scene = qobject_cast<StartScene *>(scene);
    if (start_scene) {
        start_scene->switchToServer(server);
        if (Config.value("EnableMinimizeDialog", false).toBool())
            this->on_actionMinimize_to_system_tray_triggered();
    }
}

void MainWindow::checkVersion(const QString &server_version, const QString &server_mod) {
    QString client_mod = Sanguosha->getMODName();
    if (client_mod != server_mod) {
        QMessageBox::warning(this, tr("Warning"), tr("Client MOD name is not same as the server!"));
        deleteClient();
        return;
    }

    Client *client = qobject_cast<Client *>(sender());
    QString client_version = Sanguosha->getVersionNumber();

    if (server_version == client_version) {
        client->signup();
        connect(client, SIGNAL(server_connected()), SLOT(enterRoom()));
        return;
    }

    client->disconnectFromHost();

    static QString link = "http://pan.baidu.com/share/link?shareid=396750&uk=1442992357";
    QString text = tr("Server version is %1, client version is %2 <br/>").arg(server_version).arg(client_version);
    if (server_version > client_version)
        text.append(tr("Your client version is older than the server's, please update it <br/>"));
    else
        text.append(tr("The server version is older than your client version, please ask the server to update<br/>"));

    text.append(tr("Download link : <a href='%1'>%1</a> <br/>").arg(link));
    QMessageBox::warning(this, tr("Warning"), text);

    deleteClient();
}

void MainWindow::startConnection() {
    if (NULL == m_server && m_consoleStart) {
        on_actionPC_Console_Start_triggered();
        return;
    }

    Audio::resetCustomBackgroundMusicFileName();

    deleteClient();

    m_client = new Client(this);

    connect(m_client, SIGNAL(version_checked(const QString &, const QString &)),
        SLOT(checkVersion(const QString &, const QString &)));
    connect(m_client, SIGNAL(error_message(const QString &)), SLOT(networkError(const QString &)));
}

void MainWindow::on_actionReplay_triggered() {
    if (m_client && !m_client->m_isGameOver) {
        return;
    }

    QString location = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
    QString last_dir = Config.value("LastReplayDir").toString();
    if (!last_dir.isEmpty())
        location = last_dir;

    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Select a reply file"),
                                                    location,
                                                    tr("Pure text replay file (*.txt);;Image replay file (*.png)"));

    if (filename.isEmpty())
        return;

    QFileInfo file_info(filename);
    last_dir = file_info.absoluteDir().path();
    Config.setValue("LastReplayDir", last_dir);

    ReplayFile file(filename);
    if (!file.isValid()) {
        QMessageBox::warning(this, tr("Replay"), tr("Parse file failed!"));
        return;
    }
    deleteClient();
    m_client = new Client(this, new Replayer(file));
    m_replayPath = filename;
    connect(m_client, SIGNAL(server_connected()), SLOT(enterRoom()));
    m_client->signup();
}

void MainWindow::networkError(const QString &error_msg) {
    //当游戏结束出现结果对话框，此时若服务器出错，再点击“重新开始”按钮，
    //程序会崩溃。目前出错点在弹出模态消息框这条语句上（也曾尝试使用模态的Window对象
    //也不行，只要是模态的，必然崩溃）。在未找到真正原因前，暂时采取规避方案，
    //即在游戏结束未重新开始前，不弹网络错误消息框
    if (isVisible() && (!RoomSceneInstance || RoomSceneInstance->isGameStarted()
        || RoomSceneInstance->isReturnMainMenuButtonVisible())) {
        QMessageBox::warning(this, tr("Network error"), error_msg);
    }

    if (NULL != RoomSceneInstance) {
        RoomSceneInstance->stopHeroSkinChangingAnimations();
    }

    restart();
}

void ResourceLoader::loadAnimations()
{
    QStringList emotions = G_ROOM_SKIN.getAnimationFileNames();
    foreach (const QString &emotion, emotions) {
        int frameCount = PixmapAnimation::GetFrameCount(emotion);
        for (int i = 0; i < frameCount; ++i) {
            QString fileName = QString("image/system/emotion/%1/%2.png").arg(emotion).arg(QString::number(i));
            G_ROOM_SKIN.getPixmapFromFileName(fileName, true);

            //由于加载动画文件是个比较耗时的操作，如果当前线程和主线程
            //过于频繁的竞争共享资源QSanPixmapCache(该类使用了线程同步机制)，
            //将会造成主界面有比较明显的停顿现象。因此为了避免产生停顿现象，
            //当前线程在每加载完一个图片文件之后暂停若干毫秒。
            BackgroundRunner::msleep(1);
        }
    }

    emit slot_finished();
}

void SceneDeleter::deleteScene()
{
    BackgroundRunner::msleep(5000);
    m_scene->deleteLater();
    emit slot_finished();
}

void MainWindow::enterRoom() {
    // add current ip to history
    if (!Config.HistoryIPs.contains(Config.HostAddress)) {
        Config.HistoryIPs << Config.HostAddress;
        Config.HistoryIPs.sort();
        Config.setValue("HistoryIPs", Config.HistoryIPs);
    }

    ui->actionStart_Game->setEnabled(false);
    ui->actionStart_Server->setEnabled(false);

    ui->actionPC_Console_Start->setEnabled(false);
    ui->actionReplay->setEnabled(false);

    RoomScene *room_scene = new RoomScene(this);
    ui->actionView_Discarded->setEnabled(true);
    ui->actionView_distance->setEnabled(true);
    ui->actionServerInformation->setEnabled(true);
    ui->actionSurrender->setEnabled(true);
    ui->actionNever_nullify_my_trick->setEnabled(true);
    ui->actionSaveRecord->setEnabled(true);
    ui->actionPause_Resume->setEnabled(true);
    ui->actionHide_Show_chat_box->setEnabled(true);

    connect(ClientInstance, SIGNAL(surrender_enabled(bool)), ui->actionSurrender, SLOT(setEnabled(bool)));

    connect(ui->actionView_Discarded, SIGNAL(triggered()), room_scene, SLOT(toggleDiscards()));
    connect(ui->actionView_distance, SIGNAL(triggered()), room_scene, SLOT(viewDistance()));
    connect(ui->actionServerInformation, SIGNAL(triggered()), room_scene, SLOT(showServerInformation()));
    connect(ui->actionSurrender, SIGNAL(triggered()), room_scene, SLOT(surrender()));
    connect(ui->actionSaveRecord, SIGNAL(triggered()), room_scene, SLOT(saveReplayRecord()));
    connect(ui->actionPause_Resume, SIGNAL(triggered()), room_scene, SLOT(pause()));
    connect(ui->actionHide_Show_chat_box, SIGNAL(triggered()), room_scene, SLOT(setChatBoxVisibleSlot()));

    if (ServerInfo.EnableCheat) {
        ui->menuCheat->setEnabled(true);

        connect(ui->actionDeath_note, SIGNAL(triggered()), room_scene, SLOT(makeKilling()));
        connect(ui->actionDamage_maker, SIGNAL(triggered()), room_scene, SLOT(makeDamage()));
        connect(ui->actionRevive_wand, SIGNAL(triggered()), room_scene, SLOT(makeReviving()));
        connect(ui->actionExecute_script_at_server_side, SIGNAL(triggered()), room_scene, SLOT(doScript()));
    } else {
        ui->menuCheat->setEnabled(false);
        ui->actionDeath_note->disconnect();
        ui->actionDamage_maker->disconnect();
        ui->actionRevive_wand->disconnect();
        ui->actionSend_lowlevel_command->disconnect();
        ui->actionExecute_script_at_server_side->disconnect();
    }

    connect(room_scene, SIGNAL(restart()), this, SLOT(startConnection()));
    connect(room_scene, SIGNAL(return_to_start()), this, SLOT(gotoStartScene()));

    gotoScene(room_scene);
}

void MainWindow::gotoStartScene() {
    ServerInfo.DuringGame = false;
    delete systray;
    systray = NULL;

    m_consoleStart = true;
    deleteClient();
    RoomSceneInstance = NULL;

    if (NULL != m_server) {
        m_server->requestDeleteSelf();
        m_server = NULL;
    }

    StartScene *start_scene = new StartScene;

    QList<QAction *> actions;
    actions << ui->actionStart_Server
            << ui->actionStart_Game
            << ui->actionPC_Console_Start
            << ui->actionReplay
            << ui->actionConfigure
            << ui->actionGeneral_Overview
            << ui->actionCard_Overview
            << ui->actionScenario_Overview
            << ui->actionAbout
            << ui->actionAcknowledgement;

    ui->actionStart_Game->setEnabled(true);
    ui->actionStart_Server->setEnabled(true);
    ui->actionReplay->setEnabled(true);
    ui->actionPC_Console_Start->setEnabled(true);

    foreach (QAction *action, actions)
        start_scene->addButton(action);

    setCentralWidget(view);

    ui->menuCheat->setEnabled(false);
    ui->actionDeath_note->disconnect();
    ui->actionDamage_maker->disconnect();
    ui->actionRevive_wand->disconnect();
    ui->actionSend_lowlevel_command->disconnect();
    ui->actionExecute_script_at_server_side->disconnect();
    gotoScene(start_scene);

    addAction(ui->actionShow_Hide_Menu);
    addAction(ui->actionFullscreen);

    //这里使用配置文件的端口号，防止出现“若之前曾单机启动过，
    //Config.ServerPort则会是随机值，再次加入游戏时无法连接服务器”的问题
    Config.ServerPort = Config.value("ServerPort", "9527").toString().toUShort();
}

void MainWindow::startGameInAnotherInstance() {
    QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
}

void MainWindow::on_actionGeneral_Overview_triggered() {
    //修复”若对GeneralOverview对话框中的列表进行过排序操作后，再次打开该对话框后，
    //列表显示异常“的问题
    if (NULL == m_generalOverview) {
        m_generalOverview = GeneralOverview::getInstance(this);
        m_generalOverview->fillGenerals(Sanguosha->getAllGenerals());
    }
    m_generalOverview->show();
}

void MainWindow::on_actionCard_Overview_triggered() {
    if (NULL == m_cardOverview) {
        m_cardOverview = CardOverview::getInstance(this);
        m_cardOverview->loadFromAll();
    }
    m_cardOverview->show();
}

void MainWindow::on_actionEnable_Hotkey_toggled(bool checked) {
    if (Config.EnableHotKey != checked) {
        Config.EnableHotKey = checked;
        Config.setValue("EnableHotKey", checked);
    }
}

void MainWindow::on_actionNever_nullify_my_trick_toggled(bool checked) {
    if (Config.NeverNullifyMyTrick != checked) {
        Config.NeverNullifyMyTrick = checked;
        Config.setValue("NeverNullifyMyTrick", checked);
    }
}

void MainWindow::on_actionAbout_triggered() {
    QString title(tr("About QSanguosha"));
    QGraphicsItem *const item = _isExistItem(title);
    if (item) {
        return;
    }

    // Cao Cao's pixmap
    QString content = "<center> <br/> <img src='image/system/shencc.png'> <br/> </center>";

    // Cao Cao' poem
    QString poem = tr("Disciples dressed in blue, my heart worries for you. You are the cause, of this song without pause <br/>"
        "\"A Short Song\" by Cao Cao");
    content.append(QString("<p align='right'><i>%1</i></p>").arg(poem));

    QString email = "moligaloo@gmail.com";
    content.append(tr("This is the open source clone of the popular <b>Sanguosha</b> game,"
        "totally written in C++ Qt GUI framework <br/>"
        "My Email: <a href='mailto:%1' style = \"color:#0072c1; \">%1</a> <br/>"
        "My QQ: 365840793 <br/>"
        "My Weibo: http://weibo.com/moligaloo <br/>").arg(email));

    QString config;

#ifdef QT_NO_DEBUG
    config = "release";
#else
    config = "debug";
#endif

    content.append(tr("Current version: %1 %2 (%3)<br/>")
        .arg(Sanguosha->getVersion())
        .arg(config)
        .arg(Sanguosha->getVersionName()));

    const char *date = __DATE__;
    const char *time = __TIME__;

    QLocale englishLocale(QLocale::C);
    QDate compileDate = englishLocale.toDate(date, "MMM d yyyy");
    if (!compileDate.isValid()) {
        compileDate = englishLocale.toDate(date, "MMM  d yyyy");
    }
    content.append(tr("Compilation time: %1 %2 <br/>")
        .arg(compileDate.toString(Qt::ISODate))
        .arg(time));

    QString project_url = "https://github.com/Paracel/QSanguosha-Para";
    content.append(tr("Source code: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(project_url));

    QString forum_url = "http://qsanguosha.org";
    content.append(tr("Forum: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(forum_url));

    Window *window = new Window(title, QSize(420, 470), QString(), true);
    scene->addItem(window);

    window->setZValue(32766);
    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->shift(scene->width(), scene->height());
    window->appear();
}

void MainWindow::setBackgroundBrush(bool centerAsOrigin) {
    bool backgroundImageExist = QFile::exists(Config.BackgroundImage);
    bool gameBackgroundImageExist = QFile::exists(Config.GameBackgroundImage);

    QString BackgroundImagePath;
    if (backgroundImageExist && gameBackgroundImageExist) {
        BackgroundImagePath = centerAsOrigin ? Config.BackgroundImage : Config.GameBackgroundImage;
    }
    else if (backgroundImageExist && !gameBackgroundImageExist) {
        BackgroundImagePath = Config.BackgroundImage;
    }
    else if (!backgroundImageExist && gameBackgroundImageExist) {
        BackgroundImagePath = Config.GameBackgroundImage;
    }

    view->setStyleSheet(
        QString("QFrame#%1{border-image : url(%2)}")
        .arg("QSanguoshaFrame")
        .arg(BackgroundImagePath));
}

void MainWindow::changeBackground() {
    bool centerAsOrigin = scene != NULL && !scene->inherits("RoomScene");
    setBackgroundBrush(centerAsOrigin);

    if (scene->inherits("StartScene")) {
        StartScene *start_scene = qobject_cast<StartScene *>(scene);
        start_scene->setServerLogBackground();
    }
}

void MainWindow::on_actionFullscreen_triggered()
{
    SanScene *san_scene = qobject_cast<SanScene *>(this->scene);
    if (san_scene != NULL) {
        //切换全屏时需暂时关闭鼠标悬停功能，以免产生不准确的按钮悬停效果
        san_scene->setEnableMouseHover(false);
    }

    QMenuBar *const mainMenuBar = menuBar();
    if (isFullScreen())
    {
        mainMenuBar->setVisible(m_menuBarVisible);
        showMaximized();
        restoreGeometry(m_lastPosBeforeFullScreen);
    }
    else
    {
        m_lastPosBeforeFullScreen = saveGeometry();
        mainMenuBar->setVisible(false);
        showFullScreen();
    }

    if (san_scene != NULL) {
        san_scene->setEnableMouseHover(true);
    }
}

void MainWindow::on_actionShow_Hide_Menu_triggered()
{
    QMenuBar *const mainMenuBar = menuBar();
    m_menuBarVisible = !mainMenuBar->isVisible();
    mainMenuBar->setVisible(m_menuBarVisible);
}

void MainWindow::on_actionMinimize_to_system_tray_triggered()
{
    if (systray == NULL) {
        QIcon icon("image/system/magatamas/5.png");
        systray = new QSystemTrayIcon(icon, this);

        QAction *appear = new QAction(tr("Show main window"), this);
        connect(appear, SIGNAL(triggered()), this, SLOT(show()));

        QMenu *menu = new QMenu;
        menu->addAction(appear);
        menu->addMenu(ui->menuGame);
        menu->addMenu(ui->menuView);
        menu->addMenu(ui->menuOptions);
        menu->addMenu(ui->menuHelp);

        systray->setContextMenu(menu);
    }

    systray->show();
    systray->showMessage(windowTitle(), tr("Game is minimized"));
    hide();
}

void MainWindow::on_actionRole_assign_table_triggered()
{
    QString title(tr("Role assign table"));
    QGraphicsItem *const item = _isExistItem(title);
    if (item) {
        return;
    }

    QString content;

    QStringList headers;
    headers << tr("Count") << tr("Lord") << tr("Loyalist") << tr("Rebel") << tr("Renegade");
    foreach (QString header, headers)
        content += QString("<th>%1</th>").arg(header);

    content = QString("<tr>%1</tr>").arg(content);

    QStringList rows;
    rows << "2 1 0 1 0" << "3 1 0 1 1" << "4 1 0 2 1"
         << "5 1 1 2 1" << "6 1 1 3 1" << "6d 1 1 2 2"
         << "7 1 2 3 1" << "8 1 2 4 1" << "8d 1 2 3 2"
         << "8z 1 3 4 0" << "9 1 3 4 1" << "10 1 3 4 2"
         << "10z 1 4 5 0" << "10o 1 3 5 1";

    foreach (QString row, rows) {
        QStringList cells = row.split(" ");
        QString header = cells.takeFirst();
        if (header.endsWith("d")) {
            header.chop(1);
            header += tr(" (double renegade)");
        }
        if (header.endsWith("z")) {
            header.chop(1);
            header += tr(" (no renegade)");
        }
        if (header.endsWith("o")) {
            header.chop(1);
            header += tr(" (single renegade)");
        }

        QString row_content;
        row_content = QString("<td>%1</td>").arg(header);
        foreach (QString cell, cells)
            row_content += QString("<td>%1</td>").arg(cell);

        content += QString("<tr>%1</tr>").arg(row_content);
    }

    content = QString("<table border='1'>%1</table").arg(content);

    Window *window = new Window(title, QSize(240, 450), QString(), true);
    scene->addItem(window);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->shift(scene->width(), scene->height());
    window->setZValue(32766);
    window->appear();
}

void MainWindow::on_actionScenario_Overview_triggered()
{
    ScenarioOverview *dialog = new ScenarioOverview(this);
    dialog->show();
}

BroadcastBox::BroadcastBox(Server *server, QWidget *parent)
    : QDialog(parent), server(server)
{
    setWindowTitle(tr("Broadcast"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(tr("Please input the message to broadcast")));

    text_edit = new QTextEdit;
    layout->addWidget(text_edit);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    QPushButton *ok_button = new QPushButton(tr("OK"));
    hlayout->addWidget(ok_button);

    layout->addLayout(hlayout);

    setLayout(layout);

    connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
}

void BroadcastBox::accept() {
    QDialog::accept();
    server->broadcast(text_edit->toPlainText());
}

void MainWindow::on_actionBroadcast_triggered() {
    Server *server = findChild<Server *>();
    if (server == NULL) {
        QMessageBox::warning(this, tr("Warning"), tr("Server is not started yet!"));
        return;
    }

    BroadcastBox dialog(server, this);
    dialog.exec();
}

void MainWindow::on_actionAcknowledgement_triggered() {
    QString title(tr("Acknowledgement"));
    QGraphicsItem *const item = _isExistItem(title);
    if (item) {
        return;
    }

    Window *window = new Window(title, QSize(1000, 677), "image/system/acknowledgement.png", true, false);
    scene->addItem(window);

    Button *button = window->addCloseButton(tr("OK"));
    button->moveBy(-85, -35);

    window->setZValue(32766);
    window->shift(scene->width(), scene->height());
    window->appear();
}

void MainWindow::on_actionPC_Console_Start_triggered() {
    if (m_client && !m_client->m_isGameOver) {
        return;
    }

    ServerDialog *dialog = new ServerDialog(this, tr("PC Console Start"));
    dialog->ensureEnableAI();

    if (!dialog->config()) {
        //战局重放结束后，如果用户选择了“重新开始”，然后在弹出的“单机游戏”对话框中点击了“取消”按钮，
        //需要直接返回到主菜单界面，否则程序将处于“假死”状态，不能进行任何游戏了。
        if (m_client && m_client->isReplayState()) {
            gotoStartScene();
        }
        return;
    }

    Config.HostAddress = "127.0.0.1";
    //单机启动时端口号可以使用随机数
    Config.ServerPort = qrand() % 9999 + 10000;

    Server *server = new Server(this);

    //因为使用的端口号是随机值，所以有可能出现端口号已经被其他应用程序占用的情况，
    //因此需要不断地尝试使用不同的端口号(尝试次数暂定为最多100次)
    int attemptCount = 0;
    while (!server->listen()) {
        if (++attemptCount > 100) {
            QMessageBox::warning(this, tr("Warning"), tr("Can not start server!"));
            server->deleteLater();
            return;
        }
        Config.ServerPort = qrand() % 9999 + 10000;
        BackgroundRunner::msleep(1);
    }

    m_server = server;
    m_consoleStart = true;

    server->createNewRoom();

    startConnection();
}

#include <QGroupBox>
#include <QToolButton>
#include <QCommandLinkButton>
#include <QFormLayout>

void MainWindow::on_actionReplay_file_convert_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Please select a replay file"),
        Config.value("LastReplayDir").toString(),
        tr("Pure text replay file (*.txt);;Image replay file (*.png)"));

    if (fileName.isEmpty()) {
        return;
    }

    ReplayFile file(fileName);
    if (file.isValid() && file.saveAsFormatConverted()) {
        QMessageBox::information(this, tr("Replay file convert"), tr("Conversion done!"));
    }
    else {
        QMessageBox::warning(this, tr("Replay file convert"), tr("Conversion failed!"));
    }
}

void MainWindow::on_actionRecord_analysis_triggered() {
    QString location = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Load replay record"),
                                                    location,
                                                    tr("Pure text replay file (*.txt);;Image replay file (*.png)"));

    if (filename.isEmpty()) return;

    QDialog rec_dialog(this);
    rec_dialog.setWindowTitle(tr("Record Analysis"));
    rec_dialog.resize(800, 500);

    QTableWidget *table = new QTableWidget;

    RecAnalysis *record = new RecAnalysis(filename);
    QMap<QString, PlayerRecordStruct *> record_map = record->getRecordMap();
    table->setColumnCount(11);
    table->setRowCount(record_map.keys().length());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    static QStringList labels;
    if (labels.isEmpty()) {
        labels << tr("ScreenName") << tr("General") << tr("Role") << tr("Living") << tr("WinOrLose") << tr("TurnCount")
               << tr("Recover") << tr("Damage") << tr("Damaged") << tr("Kill") << tr("Designation");
    }
    table->setHorizontalHeaderLabels(labels);
    table->setSelectionBehavior(QTableWidget::SelectRows);

    int i = 0;
    foreach (PlayerRecordStruct *rec, record_map.values()) {
        QTableWidgetItem *item = new QTableWidgetItem;
        QString screen_name = Sanguosha->translate(rec->m_screenName);
        if (rec->m_statue == "robot")
            screen_name += "(" + Sanguosha->translate("robot") + ")";

        item->setText(screen_name);
        table->setItem(i, 0, item);

        item = new QTableWidgetItem;
        QString generals = Sanguosha->translate(rec->m_generalName);
        if (!rec->m_general2Name.isEmpty())
            generals += "/" + Sanguosha->translate(rec->m_general2Name);
        item->setText(generals);
        table->setItem(i, 1, item);

        item = new QTableWidgetItem;
        item->setText(Sanguosha->translate(rec->m_role));
        table->setItem(i, 2, item);

        item = new QTableWidgetItem;
        item->setText(rec->m_isAlive ? tr("Alive") : tr("Dead"));
        table->setItem(i, 3, item);

        item = new QTableWidgetItem;
        bool is_win = record->getRecordWinners().contains(rec->m_role)
                      || record->getRecordWinners().contains(record_map.key(rec));
        item->setText(is_win ? tr("Win") : tr("Lose"));
        table->setItem(i, 4, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_turnCount));
        table->setItem(i, 5, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_recover));
        table->setItem(i, 6, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damage));
        table->setItem(i, 7, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damaged));
        table->setItem(i, 8, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_kill));
        table->setItem(i, 9, item);

        item = new QTableWidgetItem;
        item->setText(rec->m_designation.join(", "));
        table->setItem(i, 10, item);
        ++i;
    }

    table->resizeColumnsToContents();

    QLabel *label = new QLabel;
    label->setText(tr("Packages:"));

    QTextEdit *package_label = new QTextEdit;
    package_label->setReadOnly(true);
    package_label->setText(record->getRecordPackages().join(", "));

    QLabel *label_game_mode = new QLabel;
    label_game_mode->setText(tr("GameMode:") + Sanguosha->getModeName(record->getRecordGameMode()));

    QLabel *label_options = new QLabel;
    label_options->setText(tr("ServerOptions:") + record->getRecordServerOptions().join(","));

    QTextEdit *chat_info = new QTextEdit;
    chat_info->setReadOnly(true);
    chat_info->setText(record->getRecordChat());

    QLabel *table_chat_title = new QLabel;
    table_chat_title->setText(tr("Chat Information:"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(package_label);
    layout->addWidget(label_game_mode);
    layout->addWidget(label_options);
    layout->addWidget(table);
    layout->addSpacing(15);
    layout->addWidget(table_chat_title);
    layout->addWidget(chat_info);

    rec_dialog.setLayout(layout);
    rec_dialog.exec();
}

void MainWindow::on_actionView_ban_list_triggered() {
    BanlistDialog dialog(this, true);
    dialog.exec();
}

#include "audio.h"

void MainWindow::on_actionAbout_fmod_triggered() {
    QString title(tr("About fmod"));
    QGraphicsItem *const item = _isExistItem(title);
    if (item) {
        return;
    }

    QString content = tr("FMOD is a proprietary audio library made by Firelight Technologies.");
    content.append("<p align='center'> <img src='image/logo/fmod.png' /> </p> <br/>");

    QString address = "http://www.fmod.org";
    content.append(tr("Official site: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(address));

#ifdef AUDIO_SUPPORT
    content.append(tr("Current versionn %1 <br/>").arg(Audio::getVersion()));
#endif

    Window *window = new Window(title, QSize(500, 260), QString(), true);
    scene->addItem(window);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->setZValue(32766);
    window->shift(scene->width(), scene->height());
    window->appear();
}

#include "lua.hpp"

void MainWindow::on_actionAbout_Lua_triggered() {
    QString title(tr("About Lua"));
    QGraphicsItem *const item = _isExistItem(title);
    if (item) {
        return;
    }

    QString content = tr("Lua is a powerful, fast, lightweight, embeddable scripting language.");
    content.append("<p align='center'> <img src='image/logo/lua.png' /> </p> <br/>");

    QString address = "http://www.lua.org";
    content.append(tr("Official site: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(address));

    content.append(tr("Current versionn %1 <br/>").arg(LUA_RELEASE));
    content.append(LUA_COPYRIGHT);

    Window *window = new Window(title, QSize(500, 585), QString(), true);
    scene->addItem(window);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->setZValue(32766);
    window->shift(scene->width(), scene->height());
    window->appear();
}

void MainWindow::on_actionAbout_GPLv3_triggered() {
    QString title(tr("About GPLv3"));
    QGraphicsItem *const item = _isExistItem(title);
    if (item) {
        return;
    }

    QString content = tr("The GNU General Public License is the most widely used free software license, which guarantees end users the freedoms to use, study, share, and modify the software.");
    content.append("<p align='center'> <img src='image/logo/gplv3.png' /> </p> <br/>");

    QString address = "http://gplv3.fsf.org";
    content.append(tr("Official site: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(address));

    Window *window = new Window(title, QSize(500, 225), QString(), true);
    scene->addItem(window);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->setZValue(32766);
    window->shift(scene->width(), scene->height());
    window->appear();
}

QGraphicsItem *const MainWindow::_isExistItem(const QString &title) const
{
    foreach (QGraphicsItem *const &sceneItem, this->scene->items()) {
        if ((sceneItem->data(0)).toString() == title) {
            return sceneItem;
        }
    }

    return NULL;
}

void MainWindow::setConsoleStartFalse()
{
    m_consoleStart = false;
}

void MainWindow::backgroundLoadResources()
{
    BackgroundRunner *const bgRunner
        = new BackgroundRunner(new ResourceLoader, true);

    //预加载与卡牌相关的动画文件，
    //以避免在游戏过程中再加载时出现短暂的停顿现象
    bgRunner->addSlot(SLOT(loadAnimations()));

    //所有已添加进bgRunner对象的槽都执行完毕后，
    //会自动释放后台线程，ResourceLoader对象和bgRunner对象
    bgRunner->start();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);

    //支持"ESC"键退出全屏状态
    if (isFullScreen() && event->key() == Qt::Key_Escape) {
        on_actionFullscreen_triggered();
    }
}

void MainWindow::deleteClient()
{
    if (NULL != m_client) {
        //由于在创建RoomScene对象时，
        //执行过prompt_box_widget->setDocument(ClientInstance->getPromptDoc());
        //现在如果删除Client对象，Client对象中的prompt_doc也会一并删除，到时
        //prompt_box_widget中的QTextDocument指针将成为"野指针"，程序有可能崩溃，
        //为避免崩溃，因此需在删除Client对象前，先删除prompt_box_widget对象
        if (NULL != RoomSceneInstance) {
            RoomSceneInstance->deletePromptInfoItem();
        }

        Replayer *replayer = m_client->getReplayer();
        if (NULL != replayer) {
            replayer->close();
            m_replayPath.clear();
        }

        m_client->disconnectFromHost();
        m_client->disconnect();
        m_client->deleteLater();

        Sanguosha->unregisterRoom();

        //此处不需要显式 delete Self 或 Self->deleteLater()，
        //因为 Self 的父对象是 m_client，随着 m_client 的释放，
        //Self 对象也会跟着释放
        Self = NULL;
        m_client = NULL;
        ClientInstance = NULL;
    }
}

void MainWindow::restart()
{
    if (ServerInfo.DuringGame) {
#ifdef AUDIO_SUPPORT
        Audio::stopAll();
#endif
        //如果界面上还遗留对话框的话，则关闭之
        closeAllDialog();

        gotoStartScene();
    }
    else {
        deleteClient();
    }
}

void MainWindow::forceRestart()
{
    QThread::currentThread()->blockSignals(true);

    if (NULL != RoomSceneInstance) {
        RoomSceneInstance->stopHeroSkinChangingAnimations();
    }

    if (NULL != m_server) {
#ifdef AUDIO_SUPPORT
        Audio::stopAll();
#endif
        //如果界面上还遗留对话框的话，则关闭之
        closeAllDialog();

        startConnection();
    }
    else {
        restart();
    }

    QThread::currentThread()->blockSignals(false);
}

void MainWindow::closeAllDialog()
{
    QList<QDialog *> all_dialogs = findChildren<QDialog *>();
    foreach (QDialog *const &dialog, all_dialogs) {
        if (dialog->isVisible()) {
            dialog->reject();
        }
    }
}
