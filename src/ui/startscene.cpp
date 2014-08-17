#include "startscene.h"
#include "engine.h"
#include "audio.h"

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QNetworkInterface>
#include <QGraphicsDropShadowEffect>
#include <QScrollBar>
#include <QGraphicsProxyWidget>
#include "settings.h"
#include "server.h"

StartScene::StartScene()
{
    // game logo
    logo = new QSanSelectableItem("image/logo/logo.png", false);
    addItem(logo);

    //the website URL
    QFont website_font(Config.SmallFont);
    website_font.setStyle(QFont::StyleItalic);
    website_text = addSimpleText("http://qsanguosha.org", website_font);
    website_text->setBrush(Qt::white);

    server_log = NULL;
}

void StartScene::addButton(QAction *action) {
    Button *button = new Button(action->text());
    button->setMute(false);

    connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
    addItem(button);

    buttons << button;
}

void StartScene::setServerLogBackground() {
    if (server_log) {
        // make its background the same as background, looks transparent
        QPalette palette;
        palette.setBrush(QPalette::Base, backgroundBrush());
        server_log->setPalette(palette);
    }
}

void StartScene::switchToServer(Server *server) {
    // performs leaving animation
    QPropertyAnimation *logo_shift = new QPropertyAnimation(logo, "pos", this);
    QPropertyAnimation *logo_shrink = new QPropertyAnimation(logo, "scale", this);
    logo_shrink->setEndValue(0.5);

    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(logo_shift);
    group->addAnimation(logo_shrink);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    foreach (Button *button, buttons)
        delete button;
    buttons.clear();

    server_log = new QTextEdit();
    server_log->setReadOnly(true);
    server_log->resize(700, 420);
    QRectF startSceneRect = sceneRect();
    server_log->move(startSceneRect.width() / 2 - server_log->width() / 2,
        startSceneRect.height() / 2 - server_log->height() / 2 + logo->boundingRect().height() / 4);
    server_log->setFrameShape(QFrame::NoFrame);
    server_log->setFont(QFont("Verdana", 12));
    server_log->setTextColor(Config.TextEditColor);
    setServerLogBackground();
    QGraphicsProxyWidget *widget = addWidget(server_log);
    widget->setParent(this);

    QScrollBar *logBoxVScrollBar = server_log->verticalScrollBar();
    if (NULL != logBoxVScrollBar) {
        logBoxVScrollBar->setObjectName("sgsVSB");
        logBoxVScrollBar->setStyleSheet(Settings::getQSSFileContent());
    }

    printServerInfo();
    connect(server, SIGNAL(server_message(QString)), server_log, SLOT(append(QString)));
    update();
}

void StartScene::printServerInfo() {
    QStringList items = getHostAddresses();
    items.sort();

    foreach (const QString &item, items) {
        if (item.startsWith("192.168.") || item.startsWith("10."))
            server_log->append(tr("Your LAN address: %1, this address is available only for hosts that in the same LAN").arg(item));
        else if (item.startsWith("172.")) {
            QHostAddress address(item);
            quint32 ipv4 = address.toIPv4Address();
            if (ipv4 >= 0xAC100000 && ipv4 <= 0xAC1FFFFF) {
                server_log->append(tr("Your LAN address: %1, this address is available only for hosts that in the same LAN").arg(item));
            }
        }
        else if (item == "127.0.0.1")
            server_log->append(tr("Your loopback address %1, this address is available only for your host").arg(item));
        else if (item.startsWith("5."))
            server_log->append(tr("Your Hamachi address: %1, the address is available for users that joined the same Hamachi network").arg(item));
        else if (!item.startsWith("169.254."))
            server_log->append(tr("Your other address: %1, if this is a public IP, that will be available for all cases").arg(item));
    }

    server_log->append(tr("Binding port number is %1").arg(Config.ServerPort));
    server_log->append(tr("Game mode is %1").arg(Sanguosha->getModeName(Config.GameMode)));
    server_log->append(tr("Player count is %1").arg(Sanguosha->getPlayerCount(Config.GameMode)));
    server_log->append(Config.OperationNoLimit ?
                           tr("There is no time limit") :
                           tr("Operation timeout is %1 seconds").arg(Config.OperationTimeout));
    server_log->append(Config.EnableCheat ? tr("Cheat is enabled") : tr("Cheat is disabled"));
    if (Config.EnableCheat)
        server_log->append(Config.FreeChoose ? tr("Free choose is enabled") : tr("Free choose is disabled"));

    if (Config.Enable2ndGeneral) {
        QString scheme_str;
        switch (Config.MaxHpScheme) {
        case 0: scheme_str = QString(tr("Sum - %1")).arg(Config.Scheme0Subtraction); break;
        case 1: scheme_str = tr("Minimum"); break;
        case 2: scheme_str = tr("Maximum"); break;
        case 3: scheme_str = tr("Average"); break;
        }
        server_log->append(tr("Secondary general is enabled, max hp scheme is %1").arg(scheme_str));
    } else
        server_log->append(tr("Seconardary general is disabled"));

    server_log->append(Config.EnableSame ?
                           tr("Same Mode is enabled") :
                           tr("Same Mode is disabled"));
    server_log->append(Config.EnableBasara ?
                           tr("Basara Mode is enabled") :
                           tr("Basara Mode is disabled"));
    server_log->append(Config.EnableHegemony ?
                           tr("Hegemony Mode is enabled") :
                           tr("Hegemony Mode is disabled"));

    if (Config.EnableAI) {
        server_log->append(tr("This server is AI enabled, AI delay is %1 milliseconds").arg(Config.AIDelay));
    } else
        server_log->append(tr("This server is AI disabled"));
}

void StartScene::adjustItems()
{
    QRectF startSceneRect = sceneRect();

    QRectF logoPixmapRect = logo->boundingRect();
    logo->setPos(startSceneRect.width() / 2 - logoPixmapRect.width() * 0.5,
        startSceneRect.height() / 2 - logoPixmapRect.height() * 1.05);

    for (int i = 0, n = buttons.length(); i < n; ++i) {
        Button *const &button = buttons.at(i);
        QRectF btnRect = button->boundingRect();
        if (i < 4) {
            button->setPos(startSceneRect.width() / 2 - btnRect.width() - 7,
                (i - 1) * (btnRect.height() * 1.2) + startSceneRect.height() / 2 + 40);
        }
        else {
            button->setPos(startSceneRect.width() / 2 + 7,
                (i - 5) * (btnRect.height() * 1.2) + startSceneRect.height() / 2 + 40);
        }
    }

    QRectF websiteTextRect = website_text->boundingRect();
    website_text->setPos(startSceneRect.width() / 2 + websiteTextRect.width(),
        startSceneRect.height() / 2 + 8.5 * websiteTextRect.height());

    if (server_log) {
        server_log->move(startSceneRect.width() / 2 - server_log->width() / 2,
            startSceneRect.height() / 2 - server_log->height() / 2 + logo->boundingRect().height() / 4);
    }
}

QStringList StartScene::getHostAddresses()
{
    QStringList result;
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    foreach (const QHostAddress &address, addresses) {
        quint32 ipv4 = address.toIPv4Address();
        if (ipv4) {
            result << address.toString();
        }
    }

    return result;
}
