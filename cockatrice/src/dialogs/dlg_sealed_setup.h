#ifndef DLG_SEALEDSETUP_H
#define DLG_SEALEDSETUP_H

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>

#include "../game/cards/card_database.h"

#include <array>

class QLabel;
class QPushButton;
class QCheckBox;
class CardDatabaseModel;

class DlgSealedSetup : public QDialog
{
    Q_OBJECT
public:
    explicit DlgSealedSetup(CardDatabaseModel* inDbModel, QWidget *parent = nullptr);

private slots:
    void generateSealedPool();

private:
    void addCardToSealedPool(CardInfoPtr card, const QString& debugSlot) const;

    struct PerBoosterWidgets
    {
        QLabel* label = nullptr;
        QComboBox* comboBox = nullptr;
    };
    std::array<PerBoosterWidgets, 6> boosterWidgets;

    CardDatabaseModel* dbModel = nullptr;
    QMap<QString, CardSetPtr> setStrings2Sets;

};

struct BoosterCardList
{
    QList<CardInfoPtr> allCards;
    QMap<const QString, QList<CardInfoPtr>> cardsPerColors;
};

#endif
