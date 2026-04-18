// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "core/network/networkdiscoveryservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QStringBuilder>

#include "core/application.h"
#include "misc/logmessage.h"
#include "misc/network/server.h"

using namespace swift::misc;
using namespace swift::misc::network;

namespace swift::core::network
{
    CNetworkDiscoveryService::CNetworkDiscoveryService(QObject *parent) : QObject(parent) {}

    bool CNetworkDiscoveryService::isRateLimited(const QString &domain) const
    {
        const auto it = m_lastDiscovery.constFind(domain);
        if (it == m_lastDiscovery.constEnd()) { return false; }
        return it.value().secsTo(QDateTime::currentDateTimeUtc()) < 60;
    }

    void CNetworkDiscoveryService::discoverNetwork(const QString &domain,
                                                   const CSlot<void(bool, const CNetworkConfig &)> &callback)
    {
        Q_ASSERT_X(sApp, Q_FUNC_INFO, "sApp must be available");

        if (isRateLimited(domain))
        {
            CLogMessage(this).info(u"Discovery of '%1' rate-limited (< 60 s)") << domain;
            callback(false, {});
            return;
        }

        const CUrl url(QStringLiteral("https://") % domain % QStringLiteral("/.well-known/fsd-configuration.json"));

        m_lastDiscovery[domain] = QDateTime::currentDateTimeUtc();
        QPointer<CNetworkDiscoveryService> myself(this);
        sApp->getFromNetwork(url, { this, [=](QNetworkReply *nwReply) {
                                       if (!myself) { return; }
                                       nwReply->deleteLater();
                                       if (nwReply->error() != QNetworkReply::NoError)
                                       {
                                           CLogMessage(myself.data()).warning(u"Discovery of '%1' failed: %2")
                                               << domain << nwReply->errorString();
                                           callback(false, {});
                                           emit myself->discoveryCompleted(domain, false);
                                           return;
                                       }
                                       const QJsonDocument doc = QJsonDocument::fromJson(nwReply->readAll());
                                       if (doc.isNull() || !doc.isObject())
                                       {
                                           CLogMessage(myself.data()).warning(u"Discovery of '%1': invalid JSON")
                                               << domain;
                                           callback(false, {});
                                           emit myself->discoveryCompleted(domain, false);
                                           return;
                                       }
                                       const CNetworkConfig cfg = CNetworkConfig::fromJson(doc.object());
                                       callback(true, cfg);
                                       emit myself->discoveryCompleted(domain, true);
                                   } });
    }

    void CNetworkDiscoveryService::discoverNetworkByUrl(const CUrl &exactUrl,
                                                        const CSlot<void(bool, const CNetworkConfig &)> &callback)
    {
        Q_ASSERT_X(sApp, Q_FUNC_INFO, "sApp must be available");
        const QString key = exactUrl.toQString();

        if (isRateLimited(key))
        {
            CLogMessage(this).info(u"Discovery of '%1' rate-limited (< 60 s)") << key;
            callback(false, {});
            return;
        }

        m_lastDiscovery[key] = QDateTime::currentDateTimeUtc();
        QPointer<CNetworkDiscoveryService> myself(this);
        sApp->getFromNetwork(exactUrl, { this, [=](QNetworkReply *nwReply) {
                                            if (!myself) { return; }
                                            nwReply->deleteLater();
                                            if (nwReply->error() != QNetworkReply::NoError)
                                            {
                                                CLogMessage(myself.data()).warning(u"Discovery of '%1' failed: %2")
                                                    << key << nwReply->errorString();
                                                callback(false, {});
                                                emit myself->discoveryCompleted(key, false);
                                                return;
                                            }
                                            const QJsonDocument doc = QJsonDocument::fromJson(nwReply->readAll());
                                            if (doc.isNull() || !doc.isObject())
                                            {
                                                CLogMessage(myself.data()).warning(u"Discovery of '%1': invalid JSON")
                                                    << key;
                                                callback(false, {});
                                                emit myself->discoveryCompleted(key, false);
                                                return;
                                            }
                                            const CNetworkConfig cfg = CNetworkConfig::fromJson(doc.object());
                                            callback(true, cfg);
                                            emit myself->discoveryCompleted(key, true);
                                        } });
    }

    void CNetworkDiscoveryService::discoverAndFetchAll(CNetwork network,
                                                       const CSlot<void(bool, const CNetwork &)> &callback)
    {
        const QString domain = network.getDomain();
        Q_ASSERT_X(!domain.isEmpty(), Q_FUNC_INFO, "domain must not be empty");

        QPointer<CNetworkDiscoveryService> myself(this);
        auto onConfig = [=](bool ok, const CNetworkConfig &cfg) mutable {
            if (!myself) { return; }
            if (!ok)
            {
                callback(false, network);
                return;
            }
            network.setConfig(cfg);

            const CUrl serversUrl = cfg.getServersUrl();
            if (serversUrl.isEmpty())
            {
                callback(true, network);
                return;
            }

            Q_ASSERT_X(sApp, Q_FUNC_INFO, "sApp must be available");
            sApp->getFromNetwork(serversUrl, { myself.data(), [=](QNetworkReply *nwReply) mutable {
                                                  if (!myself) { return; }
                                                  nwReply->deleteLater();
                                                  if (nwReply->error() != QNetworkReply::NoError)
                                                  {
                                                      CLogMessage(myself.data())
                                                              .warning(u"Fetching servers from '%1' failed: %2")
                                                          << serversUrl.toQString() << nwReply->errorString();
                                                      callback(true, network);
                                                      return;
                                                  }
                                                  const QJsonDocument doc =
                                                      QJsonDocument::fromJson(nwReply->readAll());
                                                  const QJsonArray arr =
                                                      doc.isArray() ? doc.array() : doc.object()["servers"].toArray();
                                                  network.setServers(myself->parseServerList(arr, cfg));
                                                  callback(true, network);
                                              } });
        };

        if (network.hasConfigUrl())
            discoverNetworkByUrl(CUrl(network.getConfigUrl()), { this, onConfig });
        else
            discoverNetwork(domain, { this, onConfig });
    }

    CServerList CNetworkDiscoveryService::parseServerList(const QJsonArray &arr, const CNetworkConfig &config) const
    {
        CServerList servers;
        const CFsdSetup fsdSetup = config.toFsdSetup();

        for (const QJsonValue &val : arr)
        {
            const QJsonObject obj = val.toObject();
            if (!obj["clients_connection_allowed"].toBool(true)) { continue; }

            const QString hostname = obj["hostname_or_ip"].toString();
            const int port = obj["port"].toInt(6809);
            const QString name = obj["name"].toString();
            const QString location = obj["location"].toString();

            if (hostname.isEmpty()) { continue; }

            // Discovered networks are configured via fsd-configuration.json.
            // Do not infer legacy VATSIM behaviour presets from the protocol value here.
            CServer server(name, location, hostname, port, CUser(), fsdSetup, {}, CServer::FSDServer);
            servers.push_back(server);
        }
        return servers;
    }
} // namespace swift::core::network
