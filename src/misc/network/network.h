// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_MISC_NETWORK_NETWORK_H
#define SWIFT_MISC_NETWORK_NETWORK_H

#include <QDateTime>
#include <QMetaType>
#include <QString>

#include "misc/metaclass.h"
#include "misc/network/networkconfig.h"
#include "misc/network/serverlist.h"
#include "misc/network/user.h"
#include "misc/propertyindexref.h"
#include "misc/swiftmiscexport.h"
#include "misc/valueobject.h"

SWIFT_DECLARE_VALUEOBJECT_MIXINS(swift::misc::network, CNetwork)

namespace swift::misc::network
{
    //! A flight network identified by a domain name.
    //! Holds the cached configuration and server list obtained via well-known discovery.
    class SWIFT_MISC_EXPORT CNetwork : public CValueObject<CNetwork>
    {
    public:
        //! Properties by index
        enum ColumnIndex
        {
            IndexDomain = CPropertyIndexRef::GlobalIndexCNetwork,
            IndexNetworkName,
            IndexUser,
            IndexIsConfigLoaded
        };

        //! Default constructor
        CNetwork() = default;

        //! Construct with a domain name
        explicit CNetwork(const QString &domain);

        //! Domain (e.g. "skylitefly.com")
        const QString &getDomain() const { return m_domain; }

        //! Set domain
        void setDomain(const QString &domain) { m_domain = domain; }

        //! User credentials for this network
        const CUser &getUser() const { return m_user; }

        //! Set user credentials
        void setUser(const CUser &user) { m_user = user; }

        //! Cached network configuration (populated after discovery)
        const CNetworkConfig &getConfig() const { return m_config; }

        //! Store a freshly-fetched configuration
        void setConfig(const CNetworkConfig &config);

        //! Cached server list (populated after discovery)
        const CServerList &getServers() const { return m_servers; }

        //! Store a freshly-fetched server list
        void setServers(const CServerList &servers) { m_servers = servers; }

        //! True if a config has been loaded (discovery completed at least once)
        bool hasLoadedConfig() const { return m_isConfigLoaded; }

        //! URL at which the well-known config JSON is served
        QString getWellKnownUrl() const;

        //! True if the cached config is older than \a maxAgeMinutes
        bool isConfigStale(int maxAgeMinutes = 60) const;

        //! \copydoc swift::misc::mixin::Index::propertyByIndex
        QVariant propertyByIndex(CPropertyIndexRef index) const;

        //! \copydoc swift::misc::mixin::Index::setPropertyByIndex
        void setPropertyByIndex(CPropertyIndexRef index, const QVariant &variant);

        //! \copydoc swift::misc::mixin::Index::comparePropertyByIndex
        int comparePropertyByIndex(CPropertyIndexRef index, const CNetwork &compareValue) const;

        //! \copydoc swift::misc::mixin::String::toQString
        QString convertToQString(bool i18n = false) const;

    private:
        QString m_domain;
        CUser m_user;
        CNetworkConfig m_config;
        CServerList m_servers;
        // m_configFetchedAt and m_isConfigLoaded are runtime-only: not serialized to JSON
        QDateTime m_configFetchedAt;
        bool m_isConfigLoaded = false;

        SWIFT_METACLASS(
            CNetwork,
            SWIFT_METAMEMBER(domain),
            SWIFT_METAMEMBER(user),
            SWIFT_METAMEMBER(config),
            SWIFT_METAMEMBER(servers),
            SWIFT_METAMEMBER(configFetchedAt, 0, DisabledForJson | DisabledForComparison),
            SWIFT_METAMEMBER(isConfigLoaded, 0, DisabledForJson | DisabledForComparison));
    };
} // namespace swift::misc::network

Q_DECLARE_METATYPE(swift::misc::network::CNetwork)

#endif // SWIFT_MISC_NETWORK_NETWORK_H
