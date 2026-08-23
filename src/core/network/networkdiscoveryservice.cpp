// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "core/network/networkdiscoveryservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QSslConfiguration>
#include <QSslSocket>
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

    QNetworkRequest CNetworkDiscoveryService::makeDiscoveryRequest(const CUrl &url)
    {
        QNetworkRequest request(url.toNetworkRequest());
        // Relax SSL verification for discovery — networks may use self-signed or
        // internally-issued certificates. This is a desktop client, not a browser.
        if (QSslSocket::supportsSsl())
        {
            QSslConfiguration sslConfig = request.sslConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            request.setSslConfiguration(sslConfig);
        }
        return request;
    }

    void CNetworkDiscoveryService::fetchConfigJson(const QNetworkRequest &request, const QString &logKey,
                                                   const CSlot<void(bool, const CNetworkConfig &)> &callback)
    {
        Q_ASSERT_X(sApp, Q_FUNC_INFO, "sApp must be available");
        QPointer<CNetworkDiscoveryService> myself(this);
        sApp->getFromNetwork(request, { this, [=](QNetworkReply *nwReply) {
                                           if (!myself) { return; }
                                           nwReply->deleteLater();
                                           if (nwReply->error() != QNetworkReply::NoError)
                                           {
                                               CLogMessage(myself.data()).warning(u"Discovery of '%1' failed: %2")
                                                   << logKey << nwReply->errorString();
                                               callback(false, {});
                                               return;
                                           }
                                           const QJsonDocument doc = QJsonDocument::fromJson(nwReply->readAll());
                                           if (doc.isNull() || !doc.isObject())
                                           {
                                               CLogMessage(myself.data()).warning(u"Discovery of '%1': invalid JSON")
                                                   << logKey;
                                               callback(false, {});
                                               return;
                                           }
                                           const CNetworkConfig cfg = CNetworkConfig::fromJson(doc.object());
                                           callback(true, cfg);
                                       } });
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

        m_lastDiscovery[domain] = QDateTime::currentDateTimeUtc();
        QPointer<CNetworkDiscoveryService> myself(this);

        // Try HTTPS first (with relaxed SSL), fall back to HTTP if it fails.
        const CUrl httpsUrl(QStringLiteral("https://") % domain % QStringLiteral("/.well-known/fsd-configuration.json"));
        const QNetworkRequest httpsRequest = makeDiscoveryRequest(httpsUrl);

        fetchConfigJson(httpsRequest, httpsUrl.toQString(),
                        { myself.data(), [=](bool ok, const CNetworkConfig &cfg) mutable {
                              if (!myself) { return; }
                              if (ok)
                              {
                                  callback(true, cfg);
                                  emit myself->discoveryCompleted(domain, true);
                                  return;
                              }
                              // HTTPS failed — try plain HTTP as fallback
                              const CUrl httpUrl(QStringLiteral("http://") % domain %
                                                 QStringLiteral("/.well-known/fsd-configuration.json"));
                              CLogMessage(myself.data()).info(u"HTTPS discovery of '%1' failed, trying HTTP...") << domain;
                              myself->fetchConfigJson(myself->makeDiscoveryRequest(httpUrl), httpUrl.toQString(),
                                                      { myself.data(), [=](bool ok2, const CNetworkConfig &cfg2) {
                                                            if (!myself) { return; }
                                                            if (!ok2)
                                                            {
                                                                callback(false, {});
                                                                emit myself->discoveryCompleted(domain, false);
                                                                return;
                                                            }
                                                            callback(true, cfg2);
                                                            emit myself->discoveryCompleted(domain, true);
                                                        } });
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

        // Use relaxed SSL for HTTPS URLs; try HTTP fallback if HTTPS fails.
        const QNetworkRequest request = makeDiscoveryRequest(exactUrl);

        fetchConfigJson(request, key, { myself.data(), [=](bool ok, const CNetworkConfig &cfg) mutable {
                                           if (!myself) { return; }
                                           if (ok)
                                           {
                                               callback(true, cfg);
                                               emit myself->discoveryCompleted(key, true);
                                               return;
                                           }

                                           // If the URL was HTTPS, try the HTTP equivalent as fallback.
                                           const QUrl qurl(exactUrl.toQString());
                                           if (qurl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0)
                                           {
                                               QUrl httpUrl = qurl;
                                               httpUrl.setScheme(QStringLiteral("http"));
                                               CLogMessage(myself.data()).info(u"HTTPS fetch of '%1' failed, trying HTTP...") << key;
                                               myself->fetchConfigJson(myself->makeDiscoveryRequest(CUrl(httpUrl)),
                                                                       httpUrl.toString(),
                                                                       { myself.data(), [=](bool ok2, const CNetworkConfig &cfg2) {
                                                                             if (!myself) { return; }
                                                                             if (!ok2)
                                                                             {
                                                                                 callback(false, {});
                                                                                 emit myself->discoveryCompleted(key, false);
                                                                                 return;
                                                                             }
                                                                             callback(true, cfg2);
                                                                             emit myself->discoveryCompleted(key, true);
                                                                         } });
                                               return;
                                           }

                                           callback(false, {});
                                           emit myself->discoveryCompleted(key, false);
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
            // Use relaxed SSL for the servers list fetch as well
            const QNetworkRequest serversRequest = makeDiscoveryRequest(serversUrl);
            sApp->getFromNetwork(serversRequest, { myself.data(), [=](QNetworkReply *nwReply) mutable {
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

    CServerList CNetworkDiscoveryService::parseServerList(const QJsonArray &arr, const CNetworkConfig &config)
    {
        CServerList servers;
        const CFsdSetup fsdSetup = config.toFsdSetup();

        for (const QJsonValue &val : arr)
        {
            const QJsonObject obj = val.toObject();
            if (!obj["clients_connection_allowed"].toBool(true)) { continue; }

            const QString hostname = obj["hostname_or_ip"].toString();
            const QString name = obj["name"].toString();
            const QString location = obj["location"].toString();

            if (hostname.isEmpty()) { continue; }

            // Discovered networks are configured via fsd-configuration.json.
            // Do not infer legacy VATSIM behaviour presets from the protocol value here.
            CServer transportDefaults;
            transportDefaults.setTransport(obj["transport"].toString());
            const int defaultPort =
                transportDefaults.usesWebSocket() ? (transportDefaults.usesSecureWebSocket() ? 443 : 80) : 6809;
            const int port = obj["port"].toInt(defaultPort);
            CServer server(name, location, hostname, port, CUser(), fsdSetup, {}, CServer::FSDServer);
            server.setTransport(transportDefaults.getTransport());
            server.setWebSocketPath(obj["path"].toString());
            servers.push_back(server);
        }
        return servers;
    }
} // namespace swift::core::network
