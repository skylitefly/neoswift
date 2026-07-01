// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "misc/network/networkconfig.h"

#include <algorithm>
#include <cmath>

#include <QtGlobal>

#include "misc/comparefunctions.h"
#include "misc/propertyindexref.h"
#include "misc/verify.h"

SWIFT_DEFINE_VALUEOBJECT_MIXINS(swift::misc::network, CNetworkConfig)

namespace swift::misc::network
{
    CNetworkConfig CNetworkConfig::fromJson(const QJsonObject &json)
    {
        CNetworkConfig cfg;

        cfg.m_version = json["version"].toInt(1);

        // network section
        const QJsonObject net = json["network"].toObject();
        cfg.m_networkName = net["name"].toString();
        cfg.m_networkDescription = net["description"].toString();

        // fsd section
        const QJsonObject fsd = json["fsd"].toObject();
        cfg.m_fsdProtocol = fsd["protocol"].toString(QStringLiteral("classic"));
        cfg.m_fsdChallenge = fsd["challenge"].toBool(false);
        cfg.m_fsdAuth = fsd["auth"].toString(QStringLiteral("plain"));
        if (fsd["auth_url"].isString() && !fsd["auth_url"].toString().isEmpty())
        {
            cfg.m_fsdAuthUrl.setFullUrl(fsd["auth_url"].toString());
        }
        if (fsd["servers_url"].isString() && !fsd["servers_url"].toString().isEmpty())
        {
            cfg.m_fsdServersUrl.setFullUrl(fsd["servers_url"].toString());
        }
        if (fsd["load_balancing_url"].isString() && !fsd["load_balancing_url"].toString().isEmpty())
        {
            cfg.m_fsdLoadBalancingUrl.setFullUrl(fsd["load_balancing_url"].toString());
        }
        cfg.m_fsdTextCodec = fsd["text_codec"].toString(QStringLiteral("UTF-8"));

        // nested send/receive pairs
        const QJsonObject ap = fsd["aircraft_parts"].toObject();
        cfg.m_sendAircraftParts = ap["send"].toBool(true);
        cfg.m_receiveAircraftParts = ap["receive"].toBool(true);

        const QJsonObject ip = fsd["interim_positions"].toObject();
        cfg.m_sendInterimPositions = ip["send"].toBool(false);
        cfg.m_receiveInterimPositions = ip["receive"].toBool(false);

        const QJsonObject gf = fsd["gnd_flag"].toObject();
        cfg.m_sendGndFlag = gf["send"].toBool(true);
        cfg.m_receiveGndFlag = gf["receive"].toBool(true);

        // single-direction flags
        cfg.m_sendVisualPositions = fsd["send_visual_positions"].toBool(false);
        cfg.m_sendFplIcaoEquipment = fsd["send_fpl_icao_equipment"].toBool(false);
        cfg.m_receiveEuroscopeSimData = fsd["receive_euroscope_simdata"].toBool(false);
        cfg.m_force3LetterAirlineIcao = fsd["force_3_letter_airline_icao"].toBool(false);

        const QJsonObject reconnect = fsd["reconnect"].toObject();
        cfg.m_reconnectEnabled = reconnect["enabled"].toBool(false);
        cfg.m_reconnectMaxAttempts = std::max(0, reconnect["max_attempts"].toInt(0));
        cfg.m_reconnectInitialDelaySec = std::max(0, reconnect["initial_delay_sec"].toInt(5));
        cfg.m_reconnectBackoffMultiplier = reconnect["backoff_multiplier"].toDouble(2.0);
        if (cfg.m_reconnectBackoffMultiplier < 1.0) { cfg.m_reconnectBackoffMultiplier = 1.0; }
        cfg.m_reconnectMaxDelaySec = std::max(0, reconnect["max_delay_sec"].toInt(60));
        cfg.m_reconnectAppendAttemptToCallsign = reconnect["append_attempt_to_callsign"].toBool(false);

        // data section
        const QJsonObject data = json["data"].toObject();
        if (data["network_data_url"].isString() && !data["network_data_url"].toString().isEmpty())
        {
            cfg.m_networkDataUrl.setFullUrl(data["network_data_url"].toString());
        }
        if (data["metar_url"].isString() && !data["metar_url"].toString().isEmpty())
        {
            cfg.m_metarUrl.setFullUrl(data["metar_url"].toString());
        }
        cfg.m_dataPollingIntervalSec = data["data_polling_interval_sec"].toInt(120);
        cfg.m_maxRangeNm = data["max_range_nm"].toInt(-1);

        // voice section
        const QJsonObject voice = json["voice"].toObject();
        cfg.m_voiceEnabled = voice["enabled"].toBool(false);
        if (voice["api_url"].isString() && !voice["api_url"].toString().isEmpty())
        {
            cfg.m_voiceApiUrl.setFullUrl(voice["api_url"].toString());
        }
        if (voice["map_url"].isString() && !voice["map_url"].toString().isEmpty())
        {
            cfg.m_voiceMapUrl.setFullUrl(voice["map_url"].toString());
        }

        return cfg;
    }

    int CNetworkConfig::getFsdProtocolRevision() const
    {
        if (m_fsdProtocol == QStringLiteral("vatsim-velocity")) { return 101; }
        return 9; // "classic"
    }

    CFsdSetup CNetworkConfig::toFsdSetup() const
    {
        CFsdSetup setup;
        setup.setTextCodec(m_fsdTextCodec);
        setup.setSendReceiveDetails(m_sendAircraftParts, m_receiveAircraftParts, m_sendGndFlag, m_receiveGndFlag,
                                    m_sendInterimPositions, m_receiveInterimPositions, m_sendVisualPositions,
                                    m_receiveEuroscopeSimData, m_sendFplIcaoEquipment);
        setup.setForce3LetterAirlineCodes(m_force3LetterAirlineIcao);
        return setup;
    }

    int CNetworkConfig::reconnectDelayMsForAttempt(int attempt) const
    {
        if (attempt <= 0) { return 0; }
        const double delaySec =
            static_cast<double>(m_reconnectInitialDelaySec) * std::pow(m_reconnectBackoffMultiplier, attempt - 1);
        const double cappedDelaySec = std::min(delaySec, static_cast<double>(m_reconnectMaxDelaySec));
        return static_cast<int>(std::round(cappedDelaySec * 1000.0));
    }

    QString CNetworkConfig::reconnectCallsignForAttempt(const QString &baseCallsign, int attempt) const
    {
        if (!m_reconnectAppendAttemptToCallsign || attempt <= 0) { return baseCallsign; }
        return baseCallsign + QString::number(attempt);
    }

    bool CNetworkConfig::isValid() const { return !m_networkName.isEmpty() && !m_fsdServersUrl.isEmpty(); }

    QString CNetworkConfig::convertToQString(bool i18n) const
    {
        Q_UNUSED(i18n);
        return QStringLiteral("%1 (%2) proto:%3 auth:%4")
            .arg(m_networkName, m_networkDescription, m_fsdProtocol, m_fsdAuth);
    }

    QVariant CNetworkConfig::propertyByIndex(CPropertyIndexRef index) const
    {
        if (index.isMyself()) { return QVariant::fromValue(*this); }
        const auto i = index.frontCasted<ColumnIndex>();
        switch (i)
        {
        case IndexNetworkName: return QVariant::fromValue(m_networkName);
        case IndexNetworkDescription: return QVariant::fromValue(m_networkDescription);
        case IndexFsdProtocol: return QVariant::fromValue(m_fsdProtocol);
        case IndexFsdChallenge: return QVariant::fromValue(m_fsdChallenge);
        case IndexFsdAuth: return QVariant::fromValue(m_fsdAuth);
        case IndexFsdServersUrl: return QVariant::fromValue(m_fsdServersUrl);
        case IndexNetworkDataUrl: return QVariant::fromValue(m_networkDataUrl);
        case IndexMetarUrl: return QVariant::fromValue(m_metarUrl);
        case IndexVoiceEnabled: return QVariant::fromValue(m_voiceEnabled);
        default: return CValueObject::propertyByIndex(index);
        }
    }

    void CNetworkConfig::setPropertyByIndex(CPropertyIndexRef index, const QVariant &variant)
    {
        if (index.isMyself())
        {
            (*this) = variant.value<CNetworkConfig>();
            return;
        }
        const auto i = index.frontCasted<ColumnIndex>();
        switch (i)
        {
        case IndexNetworkName: m_networkName = variant.value<QString>(); break;
        case IndexNetworkDescription: m_networkDescription = variant.value<QString>(); break;
        case IndexFsdProtocol: m_fsdProtocol = variant.value<QString>(); break;
        case IndexFsdChallenge: m_fsdChallenge = variant.value<bool>(); break;
        case IndexFsdAuth: m_fsdAuth = variant.value<QString>(); break;
        case IndexFsdServersUrl: m_fsdServersUrl = variant.value<CUrl>(); break;
        case IndexNetworkDataUrl: m_networkDataUrl = variant.value<CUrl>(); break;
        case IndexMetarUrl: m_metarUrl = variant.value<CUrl>(); break;
        case IndexVoiceEnabled: m_voiceEnabled = variant.value<bool>(); break;
        default: CValueObject::setPropertyByIndex(index, variant); break;
        }
    }

    int CNetworkConfig::comparePropertyByIndex(CPropertyIndexRef index, const CNetworkConfig &compareValue) const
    {
        if (index.isMyself()) { return m_networkName.compare(compareValue.m_networkName); }
        const auto i = index.frontCasted<ColumnIndex>();
        switch (i)
        {
        case IndexNetworkName: return m_networkName.compare(compareValue.m_networkName, Qt::CaseInsensitive);
        case IndexNetworkDescription:
            return m_networkDescription.compare(compareValue.m_networkDescription, Qt::CaseInsensitive);
        case IndexFsdProtocol: return m_fsdProtocol.compare(compareValue.m_fsdProtocol);
        case IndexFsdAuth: return m_fsdAuth.compare(compareValue.m_fsdAuth);
        case IndexFsdChallenge: return Compare::compare(m_fsdChallenge, compareValue.m_fsdChallenge);
        case IndexVoiceEnabled: return Compare::compare(m_voiceEnabled, compareValue.m_voiceEnabled);
        default: break;
        }
        SWIFT_VERIFY_X(false, Q_FUNC_INFO, qUtf8Printable("No comparison for index " + index.toQString()));
        return 0;
    }
} // namespace swift::misc::network
