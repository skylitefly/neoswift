// SPDX-FileCopyrightText: Copyright (C) 2018 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_CORE_DATA_NETWORKSETUP_H
#define SWIFT_CORE_DATA_NETWORKSETUP_H

#include <QObject>
#include <QString>

#include "core/data/vatsimsetup.h"
#include "core/swiftcoreexport.h"
#include "core/vatsim/vatsimsettings.h"
#include "misc/datacache.h"
#include "misc/network/data/lastnetwork.h"
#include "misc/network/data/lastserver.h"
#include "misc/network/network.h"
#include "misc/network/networklist.h"
#include "misc/network/serverlist.h"
#include "misc/network/settings/networksettings.h"
#include "misc/network/settings/serversettings.h"
#include "misc/settingscache.h"
#include "misc/statusmessage.h"

namespace swift::core::data
{
    //! Remembering the last servers and ecosystem.
    class SWIFT_CORE_EXPORT CNetworkSetup : public QObject
    {
        Q_OBJECT

    public:
        //! Default constructor
        CNetworkSetup() = default;

        //! Destructor.
        ~CNetworkSetup() override = default;

        //! Last VATSIM server (VATSIM only)
        swift::misc::network::CServer getLastVatsimServer() const;

        //! Set value of last VATSIM server
        swift::misc::CStatusMessage setLastVatsimServer(const swift::misc::network::CServer &server);

        //! Last server (all networks)
        swift::misc::network::CServer getLastServer() const;

        //! Set value of last server
        swift::misc::CStatusMessage setLastServer(const swift::misc::network::CServer &server);

        //! Last used eco system
        swift::misc::network::CEcosystem getLastEcosystem() const;

        //! The other servers
        swift::misc::network::CServerList getOtherServers() const;

        //! The other servers plus test servers
        swift::misc::network::CServerList getOtherServersPlusPredefinedServers() const;

        //! Last used with VATSIM?
        bool wasLastUsedWithVatsim() const;

        //! Used with an other server (i.e. non VATSIM)
        bool wasLastUsedWithOtherServer() const;

        // ---- new generic network API ----

        //! All known networks (persisted list)
        swift::misc::network::CNetworkList getNetworks() const;

        //! Persist the list of known networks
        swift::misc::CStatusMessage setNetworks(const swift::misc::network::CNetworkList &networks);

        //! The last used network (with cached config and servers)
        swift::misc::network::CNetwork getLastNetwork() const;

        //! Persist the last used network
        swift::misc::CStatusMessage setLastNetwork(const swift::misc::network::CNetwork &network);

    signals:
        //! Setup changed
        void setupChanged();

    private:
        //! Settings have been changed
        void onSettingsChanged();

        swift::misc::CSettingReadOnly<swift::misc::network::settings::TTrafficServers> m_otherTrafficNetworkServers {
            this, &CNetworkSetup::onSettingsChanged
        };
        swift::misc::CData<swift::misc::network::data::TLastServer> m_lastServer {
            this, &CNetworkSetup::onSettingsChanged
        }; //!< recently used server (VATSIM, other)
        swift::misc::CData<swift::core::data::TVatsimLastServer> m_lastVatsimServer {
            this, &CNetworkSetup::onSettingsChanged
        }; //!< recently used VATSIM server

        // new generic network storage
        swift::misc::CSetting<swift::misc::network::settings::TNetworks> m_networks {
            this, &CNetworkSetup::onSettingsChanged
        };
        swift::misc::CData<swift::misc::network::data::TLastNetwork> m_lastNetwork {
            this, &CNetworkSetup::onSettingsChanged
        };
    };
} // namespace swift::core::data

#endif // SWIFT_CORE_DATA_NETWORKSETUP_H
