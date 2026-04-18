// SPDX-FileCopyrightText: Copyright (C) 2017 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "configsimulatorsetupcomponent.h"

#include <QLabel>
#include <QVBoxLayout>

#include "config/buildconfig.h"
#include "gui/components/settingssimulatorbasicscomponent.h"
#include "misc/simulation/simulatorinfo.h"
#include "misc/simulation/simulatorplugininfo.h"

using namespace swift::config;
using namespace swift::misc::simulation;

namespace swift::gui::components
{
    CConfigSimulatorSetupComponent::CConfigSimulatorSetupComponent(QWidget *parent) : QFrame(parent)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        m_tabWidget = new QTabWidget(this);
        layout->addWidget(m_tabWidget);
        setLayout(layout);
    }

    void CConfigSimulatorSetupComponent::initializeTabs()
    {
        // Remove existing tabs
        while (m_tabWidget->count() > 0) { m_tabWidget->removeTab(0); }

        const QStringList enabledIds = m_enabledSimulators.get();

        // Build a CSimulatorInfo from the enabled plugin id list
        // The order matches the plan: FSX, FS9, P3D, MSFS, MSFS2024, XPlane, FG
        struct SimEntry
        {
            CSimulatorInfo sim;
            bool compiled;
            QString canonicalId;
        };

        const QList<SimEntry> order = {
            { CSimulatorInfo(CSimulatorInfo::FSX), CBuildConfig::isCompiledWithFsxSupport(), CSimulatorPluginInfo::fsxPluginIdentifier() },
            { CSimulatorInfo(CSimulatorInfo::FS9), CBuildConfig::isCompiledWithFs9Support(), CSimulatorPluginInfo::fs9PluginIdentifier() },
            { CSimulatorInfo(CSimulatorInfo::P3D), CBuildConfig::isCompiledWithP3DSupport(), CSimulatorPluginInfo::p3dPluginIdentifier() },
            { CSimulatorInfo(CSimulatorInfo::MSFS), CBuildConfig::isCompiledWithMSFSSupport(), CSimulatorPluginInfo::msfsPluginIdentifier() },
            { CSimulatorInfo(CSimulatorInfo::MSFS2024), CBuildConfig::isCompiledWithMSFS2024Support(), CSimulatorPluginInfo::msfs2024PluginIdentifier() },
            { CSimulatorInfo(CSimulatorInfo::XPLANE), CBuildConfig::isCompiledWithXPlaneSupport(), CSimulatorPluginInfo::xplanePluginIdentifier() },
            { CSimulatorInfo(CSimulatorInfo::FG), CBuildConfig::isCompiledWithFGSupport(), CSimulatorPluginInfo::fgPluginIdentifier() },
        };

        int tabsAdded = 0;
        for (const SimEntry &entry : order)
        {
            if (!entry.compiled) { continue; }
            if (enabledIds.contains(entry.canonicalId))
            {
                addSimulatorTab(entry.sim);
                tabsAdded++;
            }
        }

        if (tabsAdded == 0)
        {
            auto *label = new QLabel(tr("No simulators selected. Please go back and select at least one simulator."),
                                     m_tabWidget);
            label->setWordWrap(true);
            label->setAlignment(Qt::AlignCenter);
            m_tabWidget->addTab(label, tr("No simulators"));
        }
    }

    void CConfigSimulatorSetupComponent::saveAll()
    {
        for (int i = 0; i < m_tabWidget->count(); ++i)
        {
            auto *comp = qobject_cast<CSettingsSimulatorBasicsComponent *>(m_tabWidget->widget(i));
            if (comp) { comp->save(); }
        }
    }

    bool CConfigSimulatorSetupComponent::hasUnsavedChanges() const
    {
        for (int i = 0; i < m_tabWidget->count(); ++i)
        {
            const auto *comp = qobject_cast<const CSettingsSimulatorBasicsComponent *>(m_tabWidget->widget(i));
            if (comp && comp->hasUnsavedChanges()) { return true; }
        }
        return false;
    }

    void CConfigSimulatorSetupComponent::resetUnsavedChanges()
    {
        for (int i = 0; i < m_tabWidget->count(); ++i)
        {
            auto *comp = qobject_cast<CSettingsSimulatorBasicsComponent *>(m_tabWidget->widget(i));
            if (comp) { comp->resetUnsavedChanges(); }
        }
    }

    void CConfigSimulatorSetupComponent::addSimulatorTab(const CSimulatorInfo &sim)
    {
        auto *comp = new CSettingsSimulatorBasicsComponent(m_tabWidget);
        comp->hideSelector(false); // false = hide the selector widget
        comp->setSimulator(sim);
        m_tabWidget->addTab(comp, sim.toQString(false));
    }

    void CConfigSimulatorSetupWizardPage::initializePage()
    {
        Q_ASSERT_X(m_config, Q_FUNC_INFO, "Missing config");
        m_config->resetUnsavedChanges();
        m_config->initializeTabs();
    }

    bool CConfigSimulatorSetupWizardPage::validatePage()
    {
        Q_ASSERT_X(m_config, Q_FUNC_INFO, "Missing config");
        m_config->saveAll();
        return true;
    }
} // namespace swift::gui::components
