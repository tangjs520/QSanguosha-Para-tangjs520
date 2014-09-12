#include <QApplication>
#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <QTime>
#include <QTextCodec>

#include "mainwindow.h"
#include "settings.h"
#include "banpair.h"
#include "server.h"
#include "companion-table.h"

#if defined(WIN32) && defined(VS2010)
#include "breakpad/client/windows/handler/exception_handler.h"

using namespace google_breakpad;

static bool callback(const wchar_t *dump_path, const wchar_t *id,
                     void *context, EXCEPTION_POINTERS *exinfo,
                     MDRawAssertionInfo *assertion,
                     bool succeeded) {
    if (succeeded)
        qWarning("Dump file created in %s, dump guid is %ws\n", dump_path, id);
    else
        qWarning("Dump failed\n");
    return succeeded;
}

int main(int argc, char *argv[]) {
    ExceptionHandler eh(L"./dmp", NULL, callback, NULL,
                        ExceptionHandler::HANDLER_ALL);
#else
int main(int argc, char *argv[]) {
#endif
    if (argc > 1 && strcmp(argv[1], "-server") == 0)
        new QCoreApplication(argc, argv);
    else
        new QApplication(argc, argv);

#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    QDir::setCurrent(qApp->applicationDirPath());
#endif
#endif

    //我们把程序要用到的Qt插件统一放在程序根目录下的plugins子目录中，
    //因此需要把插件的查找路径通知Qt系统
    QString pluginPath = qApp->applicationDirPath();
    pluginPath += QString("/plugins");
    qApp->addLibraryPath(pluginPath);

    // initialize random seed for later use
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

    //支持带中文的路径和文件名
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("GB18030"));

    QTranslator qt_translator, translator;
    qt_translator.load("qt_zh_CN.qm");
    translator.load("sanguosha.qm");

    qApp->installTranslator(&qt_translator);
    qApp->installTranslator(&translator);

    //在Engine的构造函数中会对全局指针Sanguosha进行赋值操作
    new Engine;

    //增加珠联璧合系统
    CompanionTable::init();

    Config.init();
    qApp->setFont(Config.AppFont);
    BanPair::loadBanPairs();

    if (qApp->arguments().contains("-server")) {
        Server *server = new Server(qApp);
        printf("Server is starting on port %u\n", Config.ServerPort);

        if (server->listen()) {
            printf("Starting successfully\n");
        }
        else {
            delete server;
            printf("Starting failed!\n");
        }

        return qApp->exec();
    }

    qApp->setStyleSheet(Settings::getQSSFileContent());

    MainWindow main_window;
    Sanguosha->setParent(&main_window);
    main_window.show();

    foreach (const QString &arg, qApp->arguments()) {
        if (arg.startsWith("-connect:")) {
            const_cast<QString &>(arg).remove("-connect:");
            Config.HostAddress = arg;
            Config.setValue("HostAddress", arg);

            main_window.startConnection();
            break;
        }
    }

    return qApp->exec();
}
