// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_CORE_NETWORK_NETWORKDISCOVERYSERVICE_H
#define SWIFT_CORE_NETWORK_NETWORKDISCOVERYSERVICE_H

#include <QDateTime>
#include <QHash>
#include <QNetworkRequest>
#include <QObject>
#include <QString>

#include "core/swiftcoreexport.h"
#include "misc/network/network.h"
#include "misc/network/networkconfig.h"
#include "misc/network/serverlist.h"
#include "misc/network/url.h"
#include "misc/slot.h"

namespace swift::core::network
{
    //! Fetches fsd-configuration.json from a network domain and populates a CNetwork object.
    class SWIFT_CORE_EXPORT CNetworkDiscoveryService : public QObject
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CNetworkDiscoveryService(QObject *parent = nullptr);

        //! Fetch the fsd-configuration.json for \a domain and invoke \a callback(success, config)
        void
        discoverNetwork(const QString &domain,
                        const swift::misc::CSlot<void(bool, const swift::misc::network::CNetworkConfig &)> &callback);

        //! Fetch the fsd-configuration.json from an exact URL and invoke \a callback(success, config)
        void discoverNetworkByUrl(
            const swift::misc::network::CUrl &exactUrl,
            const swift::misc::CSlot<void(bool, const swift::misc::network::CNetworkConfig &)> &callback);

        //! Convenience: discover config + fetch servers, update \a network, call \a callback(success, network)
        void
        discoverAndFetchAll(swift::misc::network::CNetwork network,
                            const swift::misc::CSlot<void(bool, const swift::misc::network::CNetwork &)> &callback);

        //! Parse a servers-list JSON array into CServerList, using \a config for FSD setup
        static swift::misc::network::CServerList parseServerList(const QJsonArray &arr,
                                                                 const swift::misc::network::CNetworkConfig &config);

    signals:
        //! Emitted when a discovery attempt for a domain completes
        void discoveryCompleted(const QString &domain, bool success);

    private:
        //! True if the domain was discovered within the last 60 seconds (rate-limit)
        bool isRateLimited(const QString &domain) const;

        //! Build a QNetworkRequest for \a url with SSL verification relaxed (for discovery)
        static QNetworkRequest makeDiscoveryRequest(const swift::misc::network::CUrl &url);

        //! Fetch and parse fsd-configuration.json from \a request, calling \a callback with the result
        void fetchConfigJson(const QNetworkRequest &request, const QString &logKey,
                             const swift::misc::CSlot<void(bool, const swift::misc::network::CNetworkConfig &)> &callback);

        QHash<QString, QDateTime> m_lastDiscovery;
    };
} // namespace swift::core::network

#endif // SWIFT_CORE_NETWORK_NETWORKDISCOVERYSERVICE_H
