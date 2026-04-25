// SPDX-FileCopyrightText: Copyright (C) 2013 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "gui/components/settingscomponent.h"

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QWidget>
#include <QStringList>
#include <QToolButton>
#include <QtGlobal>

#include "ui_settingscomponent.h"

#include "config/buildconfig.h"
#include "gui/components/audionotificationcomponent.h"
#include "gui/guiapplication.h"

using namespace swift::misc;
using namespace swift::misc::network;
using namespace swift::misc::aviation;
using namespace swift::misc::audio;
using namespace swift::misc::physical_quantities;
using namespace swift::misc::input;
using namespace swift::misc::simulation;
using namespace swift::misc::simulation::settings;
using namespace swift::core;
using namespace swift::gui;
using namespace swift::config;

namespace swift::gui::components
{
    CSettingsComponent::CSettingsComponent(QWidget *parent) : QFrame(parent), ui(new Ui::CSettingsComponent)
    {
        ui->setupUi(this);

        const QStringList settingsPages {
            tr("Network Servers"),      tr("GUI"),      tr("Network"),            tr("Hotkeys"),
            tr("Audio"),                tr("Data/Caches"), tr("Simulator"),       tr("Simulator Basics"),
            tr("Simulator Messages"),   tr("Matching"), tr("Advanced")
        };
        ui->lw_SettingsNavigation->addItems(settingsPages);
        ui->lw_SettingsNavigation->setCurrentRow(0);

        const int pageCount = ui->sw_SettingsPages->count();
        for (int i = 0; i < pageCount; ++i)
        {
            QWidget *page = ui->sw_SettingsPages->widget(i);
            if (!page) { continue; }

            auto *scrollArea = new QScrollArea(this);
            scrollArea->setObjectName(page->objectName() + QStringLiteral("_ScrollArea"));
            scrollArea->setWidgetResizable(true);
            scrollArea->setFrameShape(QFrame::NoFrame);
            scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

            ui->sw_SettingsPages->removeWidget(page);
            scrollArea->setWidget(page);
            ui->sw_SettingsPages->insertWidget(i, scrollArea);
        }

        this->setCurrentIndex(0);
        ui->comp_DataLoadOverview->showVisibleDbRefreshButtons(CBuildConfig::isDebugBuild() ||
                                                               sGui->isDeveloperFlagSet());
        ui->comp_DataLoadOverview->showVisibleLoadAllButtons(false, false, false);

        connect(ui->lw_SettingsNavigation, &QListWidget::currentRowChanged, this, &CSettingsComponent::setCurrentIndex);
        connect(ui->comp_SettingsGuiGeneral, &CSettingsGuiComponent::changedWindowsOpacity, this,
                &CSettingsComponent::changedWindowsOpacity);
    }

    CSettingsComponent::~CSettingsComponent() =
        default; // declared in cpp to avoid incomplete type of Ui::CSettingsComponent

    CSpecializedSimulatorSettings CSettingsComponent::getSimulatorSettings(const CSimulatorInfo &simulator) const
    {
        return ui->comp_SettingsSimulatorBasics->getSimulatorSettings(simulator);
    }

    void CSettingsComponent::setTab(CSettingsComponent::SettingTab tab)
    {
        if (tab == SettingTabOverview)
        {
            this->setCurrentIndex(0);
            return;
        }
        this->setCurrentIndex(static_cast<int>(tab) - 1);
    }

    void CSettingsComponent::setSettingsOverviewTab() { this->setTab(SettingTabServers); }

    void CSettingsComponent::setGuiOpacity(double value) { ui->comp_SettingsGuiGeneral->setGuiOpacity(value); }

    void CSettingsComponent::setCurrentIndex(int index)
    {
        if (index < 0 || index >= ui->sw_SettingsPages->count()) { return; }
        if (ui->sw_SettingsPages->currentIndex() != index) { ui->sw_SettingsPages->setCurrentIndex(index); }
        if (ui->lw_SettingsNavigation->currentRow() != index) { ui->lw_SettingsNavigation->setCurrentRow(index); }
    }
} // namespace swift::gui::components
