// SPDX-FileCopyrightText: Copyright (C) 2015 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_GUI_COMPONENTS_SETTINGSNETWORKSERVERSCOMPONENT_H
#define SWIFT_GUI_COMPONENTS_SETTINGSNETWORKSERVERSCOMPONENT_H

#include <QFrame>
#include <QPushButton>
#include <QTableWidget>

#include "core/network/networkdiscoveryservice.h"
#include "gui/swiftguiexport.h"
#include "misc/network/settings/networksettings.h"
#include "misc/settingscache.h"

namespace swift::gui::components
{
    //! Settings page for managing flight networks.
    //! Shows all configured networks in a table and allows adding / removing them.
    class SWIFT_GUI_EXPORT CSettingsNetworkServersComponent : public QFrame
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CSettingsNetworkServersComponent(QWidget *parent = nullptr);

        //! Destructor
        ~CSettingsNetworkServersComponent() override;

    private:
        void reloadTable();
        void onAddPressed();
        void onAddByUrlPressed();
        void onDeletePressed();
        void onEditUserPressed();
        void onRefreshSelectedPressed();
        void onRefreshAllPressed();
        bool editNetworkUser(int row, bool required);

        QTableWidget *m_table = nullptr;
        QPushButton *m_pbAdd = nullptr;
        QPushButton *m_pbDelete = nullptr;
        QPushButton *m_pbEditUser = nullptr;
        QPushButton *m_pbRefreshSelected = nullptr;
        QPushButton *m_pbRefreshAll = nullptr;

        swift::core::network::CNetworkDiscoveryService m_discoveryService;
        swift::misc::CSetting<swift::misc::network::settings::TNetworks> m_networks {
            this, &CSettingsNetworkServersComponent::reloadTable
        };
    };
} // namespace swift::gui::components

#endif // SWIFT_GUI_COMPONENTS_SETTINGSNETWORKSERVERSCOMPONENT_H
