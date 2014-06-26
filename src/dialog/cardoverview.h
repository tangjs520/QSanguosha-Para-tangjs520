#ifndef _CARD_OVERVIEW_H
#define _CARD_OVERVIEW_H

#include "card.h"

#include <QDialog>
#include <QTableWidgetItem>

namespace Ui {
    class CardOverview;
}

class CardOverview: public QDialog {
    Q_OBJECT

public:
    static CardOverview *getInstance(QWidget *main_window);

    explicit CardOverview(QWidget *parent = 0);
    ~CardOverview();

    void loadFromAll();
    void loadFromList(const QList<const Card *> &list);

protected:
    virtual void showEvent(QShowEvent *event);

private:
    Ui::CardOverview *ui;

    void addCard(int i, const Card *card);

private slots:
    void on_femalePlayButton_clicked();
    void on_malePlayButton_clicked();
    void on_playAudioEffectButton_clicked();
    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);
    void on_tableWidget_itemSelectionChanged();
    void askCard();
};

#endif
