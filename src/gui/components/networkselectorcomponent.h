// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_GUI_COMPONENTS_NETWORKSELECTORCOMPONENT_H
#define SWIFT_GUI_COMPONENTS_NETWORKSELECTORCOMPONENT_H

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>

#include "core/network/networkdiscoveryservice.h"
#include "gui/swiftguiexport.h"
#include "misc/aviation/callsign.h"
#include "misc/datacache.h"
#include "misc/network/data/lastnetwork.h"
#include "misc/network/loginmode.h"
#include "misc/network/network.h"
#include "misc/network/networklist.h"
#include "misc/network/server.h"
#include "misc/network/settings/networksettings.h"
#include "misc/settingscache.h"

namespace swift::gui::components
{
    class CServerListSelector;

    //! Widget for selecting the active flight network and server.
    //!
    //! Replaces the old VATSIM / Other-server two-tab layout.
    //! Networks are stored in TNetworks settings; new networks are added via
    //! the auto-discovery mechanism (/.well-known/fsd-configuration.json).
    class SWIFT_GUI_EXPORT CNetworkSelectorComponent : public QFrame
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CNetworkSelectorComponent(QWidget *parent = nullptr);

        //! Currently selected network
        swift::misc::network::CNetwork getSelectedNetwork() const;

        //! Currently selected server
        swift::misc::network::CServer getSelectedServer() const;

        //! Login mode (pilot / observer)
        swift::misc::network::CLoginMode getLoginMode() const;

        //! Set login mode
        void setLoginMode(swift::misc::network::CLoginMode mode);

        //! True if a co-pilot/partner callsign is filled in
        bool hasPartnerCallsign() const;

        //! Partner callsign (empty if not set)
        swift::misc::aviation::CCallsign getPartnerCallsign() const;

    signals:
        //! Emitted whenever the selected server changes (network switch or server switch)
        void currentServerChanged(const swift::misc::network::CServer &server);

        //! Emitted when the user wants to open network settings
        void requestNetworkSettings();

    private:
        //! Rebuild the network combo from TNetworks
        void reloadNetworks();

        //! Called when the user selects a different network in the combo
        void onNetworkIndexChanged(int index);

        //! "Refresh servers" button pressed
        void onRefreshServersPressed();

        //! Fill the server selector from the given network's cached server list
        void populateServers(const swift::misc::network::CNetwork &network);

        QComboBox *m_cbNetwork = nullptr;
        QPushButton *m_pbNetworkSettings = nullptr;
        CServerListSelector *m_serverSelector = nullptr;
        QPushButton *m_pbRefresh = nullptr;
        QCheckBox *m_cbObserver = nullptr;
        QLineEdit *m_lePartnerCallsign = nullptr;

        swift::core::network::CNetworkDiscoveryService m_discoveryService;
        swift::misc::CSetting<swift::misc::network::settings::TNetworks> m_networks {
            this, &CNetworkSelectorComponent::reloadNetworks
        };
        swift::misc::CData<swift::misc::network::data::TLastNetwork> m_lastNetwork { this };
    };
} // namespace swift::gui::components

#endif // SWIFT_GUI_COMPONENTS_NETWORKSELECTORCOMPONENT_H
