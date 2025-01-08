#include "dlg_sealed_setup.h"

#include "../client/ui/tip_of_the_day.h"
#include "../settings/cache_settings.h"
#include "../game/cards/card_database_manager.h"
#include "../game/cards/card_database_model.h"
#include "../game/game_specific_terms.h"

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
        boosterWidgets[i].comboBox = new QComboBox(this);

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
    for (auto& widget : boosterWidgets) {
        widget.comboBox->addItems(sets);
    }

    setWindowTitle(tr("Sealed Pool Setup"));
    setMinimumWidth(400);
    setMinimumHeight(150);
}

void DlgSealedSetup::addCardToSealedPool(CardInfoPtr card, const QString& debugSlot) const
{
    (*dbModel->getSealedPool())[card->getName()]++;

    QList<CardInfoPerSet> printingsInSet = card->getSets()["FDN"];
    QString rarity;
    for (auto&& printing : printingsInSet)
    {
        rarity = printing.getProperty("rarity");
        if (!rarity.isEmpty())
        {
            break;
        }
    }
    if (!rarity.isEmpty())
    {
        qDebug() << QString("%1: %2 (%3)").arg(debugSlot).arg(card->getName()).arg(rarity);
    }
}

void DlgSealedSetup::generateSealedPool()
{
    std::random_device randomEngine;

    for (int boosterIdx = 0; boosterIdx < boosterWidgets.size(); ++boosterIdx)
    {
        const QString boosterName = boosterWidgets[boosterIdx].comboBox->currentText();

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
            QString rarity;
            for(auto&& printing : printingsInSet)
            {
                rarity = printing.getProperty("rarity");
                if (!rarity.isEmpty())
                {
                    break;
                }
            }
            if (rarity.isEmpty())
            {
                continue;
            }

            BoosterCardList& targetList = cardsPerRarity[rarity];

            const QString colors = card->getProperty(Mtg::Colors);
            targetList.allCards.push_back(card);
            targetList.cardsPerColors[colors].push_back(card);
            for (int colorIdx = 0; colorIdx < colors.length(); ++colorIdx)
            {
                targetList.cardsPerColors[QString(colors[colorIdx])].push_back(card);
            }
        }

        // Generate

        // rarity-based slots
        QList<std::pair<QString, int>> slotsPerRarity;
        {
            slotsPerRarity.emplace_back("common", 7);
            slotsPerRarity.emplace_back("uncommon", 3);

            bool hasMythic = (randomEngine() % 7 == 0);
            if (hasMythic)
            {
                slotsPerRarity.emplace_back("mythic", 1);
            }
            else
            {
                slotsPerRarity.emplace_back("rare", 1);
            }
        }

        QList<QChar> mustInclude = { 'W', 'U', 'B', 'R', 'G' };
        mustInclude.erase(mustInclude.begin() + (randomEngine() % 5));

        for (auto&& [rarity, count] : slotsPerRarity)
        {
            if (!cardsPerRarity.contains(rarity))
            {
                continue;
            }

            const BoosterCardList& list = cardsPerRarity[rarity];
            if (rarity == "common")
            {
                // Color balance
                for (QChar color : mustInclude)
                {
                    const QList<CardInfoPtr>& colorList = list.cardsPerColors[QString(color)];
                    if (colorList.isEmpty())
                    {
                        continue;
                    }

                    CardInfoPtr chosenCard = colorList[randomEngine() % colorList.size()];
                    addCardToSealedPool(chosenCard, "colorAligned");
                }

                count -= mustInclude.size();
            }

            for (int randomCardIdx = 0; randomCardIdx < count; ++randomCardIdx)
            {
                CardInfoPtr chosenCard = list.allCards[randomEngine() % list.allCards.size()];
                addCardToSealedPool(chosenCard, rarity + "_random");
            }
        }

        // assume basic land in land slot / skip

        // wildcards
        const int wildcardSlots = 2;
        for (int wildcardIdx = 0; wildcardIdx < wildcardSlots; ++wildcardIdx)
        {
            BoosterCardList* list = nullptr;

            int rarityRoll = randomEngine() % 1000;

            static constexpr int common_chance = 300;
            static constexpr int uncommon_chance = 500;
            static constexpr int rare_chance = 175;
            static constexpr int mythic_chance = 25;
            static_assert(common_chance + uncommon_chance + rare_chance + mythic_chance == 1000);

            if (rarityRoll < (common_chance))
            {
                list = &cardsPerRarity["common"];
            }
            else if (rarityRoll < (common_chance + uncommon_chance))
            {
                list = &cardsPerRarity["uncommon"];
            }
            else if (rarityRoll < (common_chance + uncommon_chance + rare_chance))
            {
                list = &cardsPerRarity["rare"];
            }
            else
            {
                list = &cardsPerRarity["mythic"];
            }
            CardInfoPtr chosenCard = list->allCards[randomEngine() % list->allCards.size()];
            addCardToSealedPool(chosenCard, "wildcard");
        }
    }

    // add infinite basics
    (*dbModel->getSealedPool())["Plains"] = -1;
    (*dbModel->getSealedPool())["Island"] = -1;
    (*dbModel->getSealedPool())["Swamp"] = -1;
    (*dbModel->getSealedPool())["Mountain"] = -1;
    (*dbModel->getSealedPool())["Forest"] = -1;

    CardDatabaseManager::getInstance()->notifyEnabledSetsChanged();
    close();
}