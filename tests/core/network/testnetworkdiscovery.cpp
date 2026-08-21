// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include "core/network/networkdiscoveryservice.h"
#include "misc/network/networkconfig.h"
#include "misc/network/server.h"
#include "misc/network/serverlist.h"

using namespace swift::core::network;
using namespace swift::misc::network;

class CTestNetworkDiscovery : public QObject
{
    Q_OBJECT

private slots:
    void parsesServerTransports()
    {
        const QJsonArray input {
            QJsonObject { { "name", "TCP" }, { "hostname_or_ip", "tcp.example.test" } },
            QJsonObject { { "name", "WSS" }, { "hostname_or_ip", "wss.example.test" }, { "transport", "websocket" } },
            QJsonObject { { "name", "WS" },
                          { "hostname_or_ip", "ws.example.test" },
                          { "transport", "ws" },
                          { "path", "custom-fsd" },
                          { "port", 8080 } },
            QJsonObject { { "name", "DISABLED" },
                          { "hostname_or_ip", "disabled.example.test" },
                          { "clients_connection_allowed", false } },
        };

        const CServerList servers = CNetworkDiscoveryService::parseServerList(input, CNetworkConfig {});
        QCOMPARE(servers.size(), 3);

        QCOMPARE(servers[0].getTransport(), QStringLiteral("tcp"));
        QCOMPARE(servers[0].getPort(), 6809);
        QVERIFY(!servers[0].usesWebSocket());

        QCOMPARE(servers[1].getTransport(), QStringLiteral("wss"));
        QCOMPARE(servers[1].getPort(), 443);
        QCOMPARE(servers[1].getWebSocketPath(), QStringLiteral("/fsd"));
        QVERIFY(servers[1].usesSecureWebSocket());

        QCOMPARE(servers[2].getTransport(), QStringLiteral("ws"));
        QCOMPARE(servers[2].getPort(), 8080);
        QCOMPARE(servers[2].getWebSocketPath(), QStringLiteral("/custom-fsd"));
        QVERIFY(servers[2].usesWebSocket());
        QVERIFY(!servers[2].usesSecureWebSocket());
    }

    void unknownTransportFallsBackToTcp()
    {
        CServer server;
        server.setTransport(QStringLiteral("future-transport"));
        QCOMPARE(server.getTransport(), QStringLiteral("tcp"));
    }
};

QTEST_APPLESS_MAIN(CTestNetworkDiscovery)

#include "testnetworkdiscovery.moc"
