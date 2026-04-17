// SPDX-FileCopyrightText: Copyright (C) 2017 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_GUI_COMPONENTS_CONFIGSIMULATORSETUPCOMPONENT_H
#define SWIFT_GUI_COMPONENTS_CONFIGSIMULATORSETUPCOMPONENT_H

#include <QFrame>
#include <QTabWidget>
#include <QWizardPage>

#include "core/application/applicationsettings.h"
#include "misc/simulation/simulatorinfo.h"

namespace swift::gui::components
{
    /*!
     * Per-simulator path configuration, one tab per enabled simulator.
     * Tabs are built dynamically from the enabled-simulators setting.
     */
    class CConfigSimulatorSetupComponent : public QFrame
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CConfigSimulatorSetupComponent(QWidget *parent = nullptr);

        //! Destructor
        ~CConfigSimulatorSetupComponent() override = default;

        //! Read enabled-simulators setting and rebuild tabs
        void initializeTabs();

        //! Save all per-simulator settings
        void saveAll();

        //! Has unsaved changes in any tab
        bool hasUnsavedChanges() const;

        //! Reset unsaved-changes flag in all tabs
        void resetUnsavedChanges();

    private:
        //! Add one tab for the given simulator
        void addSimulatorTab(const swift::misc::simulation::CSimulatorInfo &sim);

        QTabWidget *m_tabWidget = nullptr;
        swift::misc::CSetting<swift::core::application::TEnabledSimulators> m_enabledSimulators { this };
    };

    /*!
     * Wizard page for CConfigSimulatorSetupComponent
     */
    class CConfigSimulatorSetupWizardPage : public QWizardPage
    {
        Q_OBJECT

    public:
        //! Constructors
        using QWizardPage::QWizardPage;

        //! Set config component
        void setConfigComponent(CConfigSimulatorSetupComponent *config) { m_config = config; }

        //! \copydoc QWizardPage::initializePage
        void initializePage() override;

        //! \copydoc QWizardPage::validatePage
        bool validatePage() override;

    private:
        CConfigSimulatorSetupComponent *m_config = nullptr;
    };
} // namespace swift::gui::components

#endif // SWIFT_GUI_COMPONENTS_CONFIGSIMULATORSETUPCOMPONENT_H
