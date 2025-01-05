#include "dlg_sealed_setup.h"

#include "../client/ui/tip_of_the_day.h"
#include "../settings/cache_settings.h"
#include "../game/cards/card_database_manager.h"
#include "../game/cards/card_database_model.h"

#include <QCheckBox>
#include <QDate>
#include <QDebug>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFocusEvent>
#include <QCompleter>
#include <QLineEdit>

DlgSealedSetup::DlgSealedSetup(CardDatabaseModel* inDbModel, QWidget *parent)
    : QDialog(parent)
    , dbModel(inDbModel)
{
    // Set up the grid layout
    QGridLayout* layout = new QGridLayout(this);

    SetList setList = CardDatabaseManager::getInstance()->getSetList();
    QStringList sets;
    sets.reserve(setList.size());
    for (CardSetPtr const& set : setList)
    {
        if (set->getEnabled())
        {
            const QString longName = set->getLongName();
            sets.append(longName);
            setStrings2Sets.insert(longName, set);
        }
    }

    // Populate boosterWidgets and add them to the layout
    for (int i = 0; i < boosterWidgets.size(); ++i)
    {
        boosterWidgets[i].label = new QLabel(QString("Booster %1:").arg(i + 1), this);
        boosterWidgets[i].comboBox = new QLineEdit(this);

        QCompleter* completer = new QCompleter(sets, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        boosterWidgets[i].comboBox->setCompleter(completer);

        // Add label and combo box to the layout
        layout->addWidget(boosterWidgets[i].label, i / 2, (i % 2) * 2); // Labels on even columns
        layout->addWidget(boosterWidgets[i].comboBox, i / 2, (i % 2) * 2 + 1); // Combo boxes on odd columns
    }

    auto* generateButton = new QPushButton("Generate", this);
    connect(generateButton, SIGNAL(clicked()), this, SLOT(generateSealedPool()));

    // Add the "Generate" button at the bottom
    layout->addWidget(generateButton, 3, 0, 1, 2);

    // Set dialog properties
    setLayout(layout);

    // Populate combo boxes with placeholder data
    
    //for (auto& widget : boosterWidgets) {
    //    widget.comboBox->addItems(sets);
    //}

    setWindowTitle(tr("Sealed Pool Setup"));
    setMinimumWidth(400);
    setMinimumHeight(150);
}

void DlgSealedSetup::generateSealedPool()
{
    dbModel->getSealedPool()->insert("Abrade", 10);
    CardDatabaseManager::getInstance()->notifyEnabledSetsChanged();
    close();
}