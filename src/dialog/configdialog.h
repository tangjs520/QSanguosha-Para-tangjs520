#ifndef _CONFIG_DIALOG_H
#define _CONFIG_DIALOG_H

#include <QDialog>

namespace Ui {
    class ConfigDialog;
}

class QLineEdit;

class ConfigDialog: public QDialog {
    Q_OBJECT
public:
    explicit ConfigDialog(QWidget *parent = 0);
    ~ConfigDialog();

//由于RoomScene::toggleFullSkin()会修改Config::UseFullSkin的值，
//为使对话框的fullSkinCheckBox控件能同步修改后的结果，
//需要在每次显示对话框时更新fullSkinCheckBox控件
protected:
    virtual void showEvent(QShowEvent *event);

private:
    Ui::ConfigDialog *ui;
    void showFont(QLineEdit *lineedit, const QFont &font);

    static QString m_defaultMusicPath;

private slots:
    void on_setTextEditColorButton_clicked();
    void on_setTextEditFontButton_clicked();
    void on_changeAppFontButton_clicked();
    void on_resetBgMusicButton_clicked();
    void on_browseBgMusicButton_clicked();
    void on_resetBgButton_clicked();
    void on_browseBgButton_clicked();
    void saveConfig();

    void on_resetGameBgButton_clicked();
    void on_browseGameBgButton_clicked();

signals:
    void bg_changed();
};

#endif
