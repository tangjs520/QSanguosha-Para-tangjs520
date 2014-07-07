#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "settings.h"
#include "engine.h"
#include "detector.h"
#include "SkinBank.h"
#include "backgroundrunner.h"

#include <QMessageBox>
#include <QTimer>
#include <QRadioButton>
#include <QBoxLayout>

static const int ShrinkWidth = 291;
static const int ExpandWidth = 753;

void ConnectionDialog::hideAvatarList() {
    if (!ui->avatarList->isVisible()) return;
    ui->avatarList->hide();
}

void ConnectionDialog::backLoadAvatarIcons()
{
    if (NULL == m_bgRunner) {
        QList<const General *> generals = Sanguosha->getAllGenerals();
        int generalCount = generals.size();
        int avatarIconCount = ui->avatarList->count();
        if (avatarIconCount < generalCount) {
            m_bgRunner = new BackgroundRunner(
                new AvatarLoader(this, generals, generalCount, avatarIconCount),
                true);
            m_bgRunner->addSlot(SLOT(loadAvatarIcons()));
            m_bgRunner->start();
        }
    }
}

void ConnectionDialog::showAvatarList() {
    if (ui->avatarList->isVisible()) return;
    backLoadAvatarIcons();
    ui->avatarList->show();
}

void ConnectionDialog::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        m_isClosed = false;

        if (ui->avatarList->isVisible()) {
            backLoadAvatarIcons();
        }
    }
}

void ConnectionDialog::dialog_closed()
{
    m_isClosed = true;
}

void AvatarLoader::loadAvatarIcons()
{
    //对话框关闭后需停止加载头像图标，否则在继续加载的情况下，进入游戏界面后会崩溃
    while (!m_connectDlg->m_isClosed && m_avatarIconCount < m_generalCount) {
        QString generalName = m_generals.at(m_avatarIconCount)->objectName();
        QIcon icon(G_ROOM_SKIN.getGeneralPixmap(generalName,
            QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));
        QString text = Sanguosha->translate(generalName);

        QListWidgetItem *item = new QListWidgetItem(icon, text, m_connectDlg->ui->avatarList);
        item->setData(Qt::UserRole, generalName);

        ++m_avatarIconCount;
        BackgroundRunner::msleep(1);
    }

    m_connectDlg->m_bgRunner = NULL;

    emit slot_finished();
}

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ConnectionDialog)
    , m_isClosed(false), m_bgRunner(NULL)
{
    ui->setupUi(this);

    ui->nameLineEdit->setText(Config.UserName);
    ui->nameLineEdit->setMaxLength(64);

    ui->hostComboBox->addItems(Config.HistoryIPs);
    ui->hostComboBox->lineEdit()->setText(Config.HostAddress);

    ui->connectButton->setFocus();

    ui->avatarPixmap->setPixmap(G_ROOM_SKIN.getGeneralPixmap(Config.UserAvatar,
                                QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));

    //avatarList必须显示的调用一下hide，
    //否则在第一次调用on_changeAvatarButton_clicked时，
    //avatarList->isVisible()会返回true，导致第一次按"修改头像"按钮无效。
    //但实际上如果在此调用avatarList->isVisible的话，它返回false，
    //按理说，应该没必要调用hide了，但实际情况却不是这样。暂未找到真正原因。
    ui->avatarList->hide();

    ui->reconnectionCheckBox->setChecked(Config.value("EnableReconnection", false).toBool());

    setFixedHeight(height());
    setFixedWidth(ShrinkWidth);

    connect(this, SIGNAL(accepted()), this, SLOT(dialog_closed()));
    connect(this, SIGNAL(rejected ()), this, SLOT(dialog_closed()));
}

ConnectionDialog::~ConnectionDialog() {
    delete ui;
}

void ConnectionDialog::on_connectButton_clicked() {
    QString username = ui->nameLineEdit->text();

    if (username.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("The user name can not be empty!"));
        return;
    }

    Config.UserName = username;
    Config.HostAddress = ui->hostComboBox->lineEdit()->text();

    Config.setValue("UserName", Config.UserName);
    Config.setValue("HostAddress", Config.HostAddress);
    Config.setValue("EnableReconnection", ui->reconnectionCheckBox->isChecked());

    accept();
}

void ConnectionDialog::on_changeAvatarButton_clicked() {
    if (ui->avatarList->isVisible()) {
        QListWidgetItem *selected = ui->avatarList->currentItem();
        if (selected)
            on_avatarList_itemDoubleClicked(selected);
        else {
            hideAvatarList();
            setFixedWidth(ShrinkWidth);
        }
    } else {
        showAvatarList();
        setFixedWidth(ExpandWidth);
    }
}

void ConnectionDialog::on_avatarList_itemDoubleClicked(QListWidgetItem *item) {
    QString general_name = item->data(Qt::UserRole).toString();
    QPixmap avatar(G_ROOM_SKIN.getGeneralPixmap(general_name, QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));
    ui->avatarPixmap->setPixmap(avatar);
    Config.UserAvatar = general_name;
    Config.setValue("UserAvatar", general_name);
    hideAvatarList();

    setFixedWidth(ShrinkWidth);
}

void ConnectionDialog::on_clearHistoryButton_clicked() {
    ui->hostComboBox->clear();
    ui->hostComboBox->lineEdit()->clear();

    Config.HistoryIPs.clear();
    Config.remove("HistoryIPs");
}

void ConnectionDialog::on_detectLANButton_clicked() {
    UdpDetectorDialog detector_dialog(this);
    connect(&detector_dialog, SIGNAL(address_chosen(QString)),
        ui->hostComboBox->lineEdit(), SLOT(setText(QString)));
    detector_dialog.exec();
}

// -----------------------------------

UdpDetectorDialog::UdpDetectorDialog(QDialog *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Detect available server's addresses at LAN"));
    detect_button = new QPushButton(tr("Refresh"));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(detect_button);

    list = new QListWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(list);
    layout->addLayout(hlayout);

    setLayout(layout);

    detector = new Detector(Config.DetectorPort, this);
    connect(detector, SIGNAL(detect_finished(const QString &, const QString &)),
        this, SLOT(addServerAddress(const QString &, const QString &)));

    connect(detect_button, SIGNAL(clicked()), this, SLOT(startDetection()));
    connect(list, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(chooseAddress(QListWidgetItem *)));

    detect_button->click();
}

void UdpDetectorDialog::startDetection() {
    list->clear();
    detect_button->setEnabled(false);

    QTimer::singleShot(2000, this, SLOT(stopDetection()));

    //这里使用配置文件的端口号，防止出现“若之前曾单机启动过，
    //Config.ServerPort则会是随机值，从而无法探测到局域网”的问题
    ushort port = Config.value("ServerPort", "9527").toString().toUShort();
    detector->detect(port);
}

void UdpDetectorDialog::stopDetection() {
    detect_button->setEnabled(true);
}

void UdpDetectorDialog::addServerAddress(const QString &server_name, const QString &address) {
    QString label = QString("%1 [%2]").arg(server_name).arg(address);
    QListWidgetItem *item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, address);

    list->addItem(item);
}

void UdpDetectorDialog::chooseAddress(QListWidgetItem *item) {
    accept();

    QString address = item->data(Qt::UserRole).toString();
    emit address_chosen(address);
}
