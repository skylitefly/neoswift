// SPDX-FileCopyrightText: Copyright (C) 2017 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "configsimulatorcomponent.h"

#include "ui_configsimulatorcomponent.h"

#include "config/buildconfig.h"
#include "gui/guiutility.h"
#include "misc/logmessage.h"
#include "misc/simulation/fscommon/fsdirectories.h"

using namespace swift::misc;
using namespace swift::misc::simulation;
using namespace swift::misc::simulation::fscommon;
using namespace swift::config;

namespace swift::gui::components
{
    CConfigSimulatorComponent::CConfigSimulatorComponent(QWidget *parent)
        : QFrame(parent), ui(new Ui::CConfigSimulatorComponent)
    {
        ui->setupUi(this);
        this->preselectSimulators();
    }

    CConfigSimulatorComponent::~CConfigSimulatorComponent() = default;

    void CConfigSimulatorComponent::save()
    {
        const QStringList sims = this->selectedSimsToPluginIds();
        const CStatusMessage msg = m_enabledSimulators.setAndSave(sims);
        CLogMessage::preformatted(msg);
    }

    bool CConfigSimulatorComponent::hasUnsavedChanges() const { return false; }

    void CConfigSimulatorComponent::resetUnsavedChanges() {}

    void CConfigSimulatorComponent::preselectSimulators()
    {
        CSimulatorInfo sims = m_enabledSimulators.get();
        if (!sims.isAnySimulator())
        {
            CSimulatorInfo simWithModels = m_modelSets.simulatorsWithModels();
            sims = simWithModels;
        }

        // no x64 check as we would allow to config 32bit with launcher x64 and vice versa
        const bool p3d =
            (sims.isP3D() || !CFsDirectories::p3dDir().isEmpty()) && CBuildConfig::isCompiledWithP3DSupport();
        const bool fsx =
            (sims.isFSX() || !CFsDirectories::fsxDir().isEmpty()) && CBuildConfig::isCompiledWithFsxSupport();
        const bool fs9 =
            (sims.isFS9() || !CFsDirectories::fs9Dir().isEmpty()) && CBuildConfig::isCompiledWithFs9Support();
        const bool msfs =
            (sims.isMSFS() || !CFsDirectories::msfsDir().isEmpty()) && CBuildConfig::isCompiledWithMSFSSupport();
        const bool msfs2024 = (sims.isMSFS2024() || !CFsDirectories::msfs2024Dir().isEmpty()) &&
                              CBuildConfig::isCompiledWithMSFS2024Support();
        const bool xp = sims.isXPlane() && CBuildConfig::isCompiledWithXPlaneSupport();
        const bool fg = sims.isFG() && CBuildConfig::isCompiledWithFGSupport();

        ui->cb_P3D->setChecked(p3d);
        ui->cb_FSX->setChecked(fsx);
        ui->cb_FS9->setChecked(fs9);
        ui->cb_XP->setChecked(xp);
        ui->cb_FG->setChecked(fg);
        ui->cb_MSFS->setChecked(msfs);
        ui->cb_MSFS2024->setChecked(msfs2024);

        ui->cb_P3D->setEnabled(CBuildConfig::isCompiledWithP3DSupport());
        ui->cb_FSX->setEnabled(CBuildConfig::isCompiledWithFsxSupport());
        ui->cb_FS9->setEnabled(CBuildConfig::isCompiledWithFs9Support());
        ui->cb_XP->setEnabled(CBuildConfig::isCompiledWithXPlaneSupport());
        ui->cb_FG->setEnabled(CBuildConfig::isCompiledWithFGSupport());
        ui->cb_MSFS->setEnabled(CBuildConfig::isCompiledWithMSFSSupport());
        ui->cb_MSFS2024->setEnabled(CBuildConfig::isCompiledWithMSFS2024Support());

        CGuiUtility::checkBoxReadOnly(ui->cb_P3D, !CBuildConfig::isCompiledWithP3DSupport());
        CGuiUtility::checkBoxReadOnly(ui->cb_FSX, !CBuildConfig::isCompiledWithFsxSupport());
        CGuiUtility::checkBoxReadOnly(ui->cb_FS9, !CBuildConfig::isCompiledWithFs9Support());
        CGuiUtility::checkBoxReadOnly(ui->cb_XP, !CBuildConfig::isCompiledWithXPlaneSupport());
        CGuiUtility::checkBoxReadOnly(ui->cb_FG, !CBuildConfig::isCompiledWithFGSupport());
        CGuiUtility::checkBoxReadOnly(ui->cb_MSFS, !CBuildConfig::isCompiledWithMSFSSupport());
        CGuiUtility::checkBoxReadOnly(ui->cb_MSFS2024, !CBuildConfig::isCompiledWithMSFS2024Support());

    }

    QStringList CConfigSimulatorComponent::selectedSimsToPluginIds()
    {
        QStringList ids;

        // have to match full canonical ids from swift-plugin-simulators.xml
        if (ui->cb_FS9->isChecked()) { ids << CSimulatorPluginInfo::fs9PluginIdentifier(); }
        if (ui->cb_FSX->isChecked()) { ids << CSimulatorPluginInfo::fsxPluginIdentifier(); }
        if (ui->cb_P3D->isChecked()) { ids << CSimulatorPluginInfo::p3dPluginIdentifier(); }
        if (ui->cb_XP->isChecked()) { ids << CSimulatorPluginInfo::xplanePluginIdentifier(); }
        if (ui->cb_FG->isChecked()) { ids << CSimulatorPluginInfo::fgPluginIdentifier(); }
        if (ui->cb_MSFS->isChecked()) { ids << CSimulatorPluginInfo::msfsPluginIdentifier(); }
        if (ui->cb_MSFS2024->isChecked()) { ids << CSimulatorPluginInfo::msfs2024PluginIdentifier(); }

        return ids;
    }

    void CConfigSimulatorWizardPage::initializePage() { m_config->resetUnsavedChanges(); }

    bool CConfigSimulatorWizardPage::validatePage()
    {
        Q_ASSERT_X(m_config, Q_FUNC_INFO, "Missing config");
        m_config->save();
        return true;
    }
} // namespace swift::gui::components
