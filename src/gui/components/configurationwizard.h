// SPDX-FileCopyrightText: Copyright (C) 2017 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_GUI_COMPONENTS_CONFIGURATIONWIZARD_H
#define SWIFT_GUI_COMPONENTS_CONFIGURATIONWIZARD_H

#include <QScopedPointer>
#include <QWizard>

#include "core/application/applicationsettings.h"
#include "gui/swiftguiexport.h"
#include "misc/settingscache.h"

namespace Ui
{
    class CConfigurationWizard;
}
namespace swift::gui::components
{
    /*!
     * Configure the most important settings
     */
    class SWIFT_GUI_EXPORT CConfigurationWizard : public QWizard
    {
        Q_OBJECT

    public:
        //! Page ids
        enum Pages
        {
            Welcome,
            DataLoad,
            CopyModels,
            CopySettingsAndCaches,
            NetworkDiscovery,
            SelectSimulator,    //!< checkboxes only
            SetupSimulators,    //!< per-simulator tab config
            FirstModelSet,
            XSwiftBus,
            ConfigHotkeys
        };

        //! Constructor
        explicit CConfigurationWizard(QWidget *parent = nullptr);

        //! Destructor
        ~CConfigurationWizard() override;

        //! \copydoc QWizard::nextId
        int nextId() const override;

    private:
        //! The current page has changed
        void wizardCurrentIdChanged(int id);

        //! Accepted or rejected
        void ended();

        //! Set the parent's window opacity
        void setParentOpacity(qreal opacity);

        //! Set screen geometry based on screen resolution
        void setScreenGeometry();

        QScopedPointer<Ui::CConfigurationWizard> ui;
        swift::misc::CSetting<swift::core::application::TEnabledSimulators> m_enabledSimulators { this };
    };
} // namespace swift::gui::components
#endif // SWIFT_GUI_COMPONENTS_CONFIGURATIONWIZARD_H
