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
    struct PerBoosterWidgets
    {
        QLabel* label = nullptr;
        QLineEdit* comboBox = nullptr;
    };
    std::array<PerBoosterWidgets, 6> boosterWidgets;

    CardDatabaseModel* dbModel = nullptr;
    QMap<QString, CardSetPtr> setStrings2Sets;

};

#endif
