#ifndef _START_SCENE_H
#define _START_SCENE_H

#include "button.h"
#include "QSanSelectableItem.h"

#include <QGraphicsScene>
#include <QAction>
#include <QTextEdit>

#include "sanscene.h"

class Server;

class StartScene : public SanScene {
    Q_OBJECT

public:
    StartScene();

    ~StartScene() {
        delete server_log;
    }

    void addButton(QAction *action);
    void setServerLogBackground();
    void switchToServer(Server *server);

    static QStringList getHostAddresses();

    virtual void adjustItems();

private:
    void printServerInfo();

    QSanSelectableItem *logo;
    QTextEdit *server_log;
    QList<Button *> buttons;
    QGraphicsSimpleTextItem *website_text;
};

#endif
