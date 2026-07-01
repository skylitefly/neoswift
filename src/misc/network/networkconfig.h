// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_MISC_NETWORK_NETWORKCONFIG_H
#define SWIFT_MISC_NETWORK_NETWORKCONFIG_H

#include <QJsonObject>
#include <QMetaType>
#include <QString>

#include "misc/metaclass.h"
#include "misc/network/fsdsetup.h"
#include "misc/network/url.h"
#include "misc/propertyindexref.h"
#include "misc/swiftmiscexport.h"
#include "misc/valueobject.h"

SWIFT_DECLARE_VALUEOBJECT_MIXINS(swift::misc::network, CNetworkConfig)

namespace swift::misc::network
{
    //! Network configuration loaded from fsd-configuration.json (.well-known discovery)
    class SWIFT_MISC_EXPORT CNetworkConfig : public CValueObject<CNetworkConfig>
    {
    public:
        //! Properties by index
        enum ColumnIndex
        {
            IndexNetworkName = CPropertyIndexRef::GlobalIndexCNetworkConfig,
            IndexNetworkDescription,
            IndexFsdProtocol,
            IndexFsdChallenge,
            IndexFsdAuth,
            IndexFsdServersUrl,
            IndexNetworkDataUrl,
            IndexMetarUrl,
            IndexVoiceEnabled
        };

        //! Default constructor
        CNetworkConfig() = default;

        //! Parse from the JSON object representing fsd-configuration.json
        static CNetworkConfig fromJson(const QJsonObject &json);

        //! Network display name
        const QString &getNetworkName() const { return m_networkName; }

        //! Network description
        const QString &getNetworkDescription() const { return m_networkDescription; }

        //! FSD protocol string ("classic" | "vatsim-velocity")
        const QString &getFsdProtocol() const { return m_fsdProtocol; }

        //! FSD protocol revision number: "classic" -> 9, "vatsim-velocity" -> 101
        int getFsdProtocolRevision() const;

        //! Whether this protocol uses VATSIM-style ATIS messages instead of private-message fallback
        bool usesVatsimAtisMessages() const { return m_fsdProtocol == QStringLiteral("vatsim-velocity"); }

        //! Whether VATSIMAuth challenge/response handshake is enabled (independent of auth mode)
        bool useChallenge() const { return m_fsdChallenge; }

        //! Auth mode string ("plain" | "jwt")
        const QString &getFsdAuth() const { return m_fsdAuth; }

        //! Whether JWT token auth is used (plain password otherwise)
        bool useJwt() const { return m_fsdAuth == QStringLiteral("jwt"); }

        //! URL to exchange CID+password for a JWT token (only relevant when useJwt())
        const CUrl &getAuthUrl() const { return m_fsdAuthUrl; }

        //! URL to fetch the server list JSON
        const CUrl &getServersUrl() const { return m_fsdServersUrl; }

        //! Load-balancing redirect URL (optional)
        const CUrl &getLoadBalancingUrl() const { return m_fsdLoadBalancingUrl; }

        //! Text codec for FSD messages
        const QString &getTextCodec() const { return m_fsdTextCodec; }

        //! URL to fetch network-data (pilots/controllers)
        const CUrl &getNetworkDataUrl() const { return m_networkDataUrl; }

        //! METAR endpoint URL
        const CUrl &getMetarUrl() const { return m_metarUrl; }

        //! Data polling interval in seconds
        int getDataPollingIntervalSec() const { return m_dataPollingIntervalSec; }

        //! Maximum visibility range in NM (-1 = unlimited)
        int getMaxRangeNm() const { return m_maxRangeNm; }

        //! Whether voice (AFV) is enabled for this network
        bool isVoiceEnabled() const { return m_voiceEnabled; }

        //! AFV API server URL
        const CUrl &getVoiceApiUrl() const { return m_voiceApiUrl; }

        //! AFV map URL
        const CUrl &getVoiceMapUrl() const { return m_voiceMapUrl; }

        //! Whether unexpected FSD disconnects should be retried
        bool isReconnectEnabled() const { return m_reconnectEnabled && m_reconnectMaxAttempts > 0; }

        //! Maximum number of reconnect attempts
        int getReconnectMaxAttempts() const { return m_reconnectMaxAttempts; }

        //! Initial reconnect delay in seconds
        int getReconnectInitialDelaySec() const { return m_reconnectInitialDelaySec; }

        //! Reconnect delay multiplier for subsequent attempts
        double getReconnectBackoffMultiplier() const { return m_reconnectBackoffMultiplier; }

        //! Maximum reconnect delay in seconds
        int getReconnectMaxDelaySec() const { return m_reconnectMaxDelaySec; }

        //! Whether reconnect attempts append the attempt number to the callsign
        bool appendReconnectAttemptToCallsign() const { return m_reconnectAppendAttemptToCallsign; }

        //! Delay before a reconnect attempt, in milliseconds
        int reconnectDelayMsForAttempt(int attempt) const;

        //! Callsign to use for a reconnect attempt
        QString reconnectCallsignForAttempt(const QString &baseCallsign, int attempt) const;

        //! Build a CFsdSetup from the boolean flags in this config
        CFsdSetup toFsdSetup() const;

        //! True if the minimum required fields are populated
        bool isValid() const;

        //! \copydoc swift::misc::mixin::Index::propertyByIndex
        QVariant propertyByIndex(CPropertyIndexRef index) const;

        //! \copydoc swift::misc::mixin::Index::setPropertyByIndex
        void setPropertyByIndex(CPropertyIndexRef index, const QVariant &variant);

        //! \copydoc swift::misc::mixin::Index::comparePropertyByIndex
        int comparePropertyByIndex(CPropertyIndexRef index, const CNetworkConfig &compareValue) const;

        //! \copydoc swift::misc::mixin::String::toQString
        QString convertToQString(bool i18n = false) const;

    private:
        // --- top-level ---
        QString m_networkName;
        QString m_networkDescription;
        int m_version = 1;

        // --- fsd ---
        QString m_fsdProtocol = QStringLiteral("classic");
        bool m_fsdChallenge = false;
        QString m_fsdAuth = QStringLiteral("plain");
        CUrl m_fsdAuthUrl;
        CUrl m_fsdServersUrl;
        CUrl m_fsdLoadBalancingUrl;
        QString m_fsdTextCodec = QStringLiteral("UTF-8");

        // fsd flags – send/receive pairs
        bool m_sendAircraftParts = true;
        bool m_receiveAircraftParts = true;
        bool m_sendInterimPositions = false;
        bool m_receiveInterimPositions = false;
        bool m_sendGndFlag = true;
        bool m_receiveGndFlag = true;

        // fsd flags – single direction
        bool m_sendVisualPositions = false;
        bool m_sendFplIcaoEquipment = false;
        bool m_receiveEuroscopeSimData = false;
        bool m_force3LetterAirlineIcao = false;

        // fsd reconnect
        bool m_reconnectEnabled = false;
        int m_reconnectMaxAttempts = 0;
        int m_reconnectInitialDelaySec = 5;
        double m_reconnectBackoffMultiplier = 2.0;
        int m_reconnectMaxDelaySec = 60;
        bool m_reconnectAppendAttemptToCallsign = false;

        // --- data ---
        CUrl m_networkDataUrl;
        CUrl m_metarUrl;
        int m_dataPollingIntervalSec = 120;
        int m_maxRangeNm = -1;

        // --- voice ---
        bool m_voiceEnabled = false;
        CUrl m_voiceApiUrl;
        CUrl m_voiceMapUrl;

        SWIFT_METACLASS(
            CNetworkConfig,
            SWIFT_METAMEMBER(networkName),
            SWIFT_METAMEMBER(networkDescription),
            SWIFT_METAMEMBER(version),
            SWIFT_METAMEMBER(fsdProtocol),
            SWIFT_METAMEMBER(fsdChallenge),
            SWIFT_METAMEMBER(fsdAuth),
            SWIFT_METAMEMBER(fsdAuthUrl),
            SWIFT_METAMEMBER(fsdServersUrl),
            SWIFT_METAMEMBER(fsdLoadBalancingUrl),
            SWIFT_METAMEMBER(fsdTextCodec),
            SWIFT_METAMEMBER(sendAircraftParts),
            SWIFT_METAMEMBER(receiveAircraftParts),
            SWIFT_METAMEMBER(sendInterimPositions),
            SWIFT_METAMEMBER(receiveInterimPositions),
            SWIFT_METAMEMBER(sendGndFlag),
            SWIFT_METAMEMBER(receiveGndFlag),
            SWIFT_METAMEMBER(sendVisualPositions),
            SWIFT_METAMEMBER(sendFplIcaoEquipment),
            SWIFT_METAMEMBER(receiveEuroscopeSimData),
            SWIFT_METAMEMBER(force3LetterAirlineIcao),
            SWIFT_METAMEMBER(reconnectEnabled),
            SWIFT_METAMEMBER(reconnectMaxAttempts),
            SWIFT_METAMEMBER(reconnectInitialDelaySec),
            SWIFT_METAMEMBER(reconnectBackoffMultiplier),
            SWIFT_METAMEMBER(reconnectMaxDelaySec),
            SWIFT_METAMEMBER(reconnectAppendAttemptToCallsign),
            SWIFT_METAMEMBER(networkDataUrl),
            SWIFT_METAMEMBER(metarUrl),
            SWIFT_METAMEMBER(dataPollingIntervalSec),
            SWIFT_METAMEMBER(maxRangeNm),
            SWIFT_METAMEMBER(voiceEnabled),
            SWIFT_METAMEMBER(voiceApiUrl),
            SWIFT_METAMEMBER(voiceMapUrl));
    };
} // namespace swift::misc::network

Q_DECLARE_METATYPE(swift::misc::network::CNetworkConfig)

#endif // SWIFT_MISC_NETWORK_NETWORKCONFIG_H
