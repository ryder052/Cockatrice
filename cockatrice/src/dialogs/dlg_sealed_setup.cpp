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
#include <random>

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

void DlgSealedSetup::addCardToSealedPool(CardInfoPtr card) const
{
    (*dbModel->getSealedPool())[card->getName()]++;
}

void DlgSealedSetup::generateSealedPool()
{
    for (int boosterIdx = 0; boosterIdx < boosterWidgets.size(); ++boosterIdx)
    {
        const QString boosterName = boosterWidgets[boosterIdx].comboBox->text();

        auto setIt = setStrings2Sets.find(boosterName);
        if (setIt == setStrings2Sets.end())
        {
            continue;
        }

        // Construct card pools
        QMap<QString, BoosterCardList> cardsPerRarity;
        for (auto&& card : **setIt)
        {
            QList<CardInfoPerSet> printingsInSet = card->getSets()[(*setIt)->getShortName()];

            const QString rarity = printingsInSet[0].getProperty("rarity");
            BoosterCardList& targetList = cardsPerRarity[rarity];

            const QString colors = card->getProperty(Mtg::Colors);
            targetList.allCards.push_back(card);
            targetList.cardsPerColors[colors].push_back(card);
            for (int colorIdx = 0; colorIdx < colors.length(); ++colorIdx)
            {
                targetList.cardsPerColors[QString(colors[colorIdx])].push_back(card);
            }
        }

        // Remove tokens
        cardsPerRarity.remove("<NULL>");

        // Generate
        std::array<int, 624> seed_data;
        std::random_device r;
        std::generate_n(seed_data.data(), seed_data.size(), std::ref(r));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 randomEngine(seq);

        QList<std::pair<QString, int>> slotsPerRarity = { {"common", 14} };
        for (auto&& [rarity, count] : slotsPerRarity)
        {
            if (!cardsPerRarity.contains(rarity))
            {
                continue;
            }

            const BoosterCardList& list = cardsPerRarity[rarity];

            QList<QChar> mustInclude = { 'W', 'U', 'B', 'R', 'G' };
            mustInclude.erase(mustInclude.begin() + (randomEngine() % 5));

            for (QChar color : mustInclude)
            {
                const QList<CardInfoPtr>& colorList = list.cardsPerColors[QString(color)];
                if (colorList.isEmpty())
                {
                    continue;
                }

                CardInfoPtr chosenCard = colorList[randomEngine() % colorList.size()];
                addCardToSealedPool(chosenCard);
            }

            count -= mustInclude.size();

            for (int randomCardIdx = 0; randomCardIdx < count; ++randomCardIdx)
            {
                CardInfoPtr chosenCard = list.allCards[randomEngine() % list.allCards.size()];
                addCardToSealedPool(chosenCard);
            }
        }
    }

    CardDatabaseManager::getInstance()->notifyEnabledSetsChanged();
    close();
}