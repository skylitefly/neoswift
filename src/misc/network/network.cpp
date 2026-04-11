// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "misc/network/network.h"

#include <QStringBuilder>
#include <QtGlobal>

#include "misc/comparefunctions.h"
#include "misc/propertyindexref.h"
#include "misc/verify.h"

SWIFT_DEFINE_VALUEOBJECT_MIXINS(swift::misc::network, CNetwork)

namespace swift::misc::network
{
    CNetwork::CNetwork(const QString &domain) : m_domain(domain) {}

    void CNetwork::setConfig(const CNetworkConfig &config)
    {
        m_config = config;
        m_isConfigLoaded = true;
        m_configFetchedAt = QDateTime::currentDateTimeUtc();
    }

    QString CNetwork::getWellKnownUrl() const
    {
        return QStringLiteral("https://") % m_domain % QStringLiteral("/.well-known/fsd-configuration.json");
    }

    bool CNetwork::isConfigStale(int maxAgeMinutes) const
    {
        if (!m_isConfigLoaded) { return true; }
        if (!m_configFetchedAt.isValid()) { return true; }
        return m_configFetchedAt.secsTo(QDateTime::currentDateTimeUtc()) > maxAgeMinutes * 60;
    }

    QString CNetwork::convertToQString(bool i18n) const
    {
        Q_UNUSED(i18n);
        if (m_isConfigLoaded) { return QStringLiteral("%1 (%2)").arg(m_domain, m_config.getNetworkName()); }
        return m_domain;
    }

    QVariant CNetwork::propertyByIndex(CPropertyIndexRef index) const
    {
        if (index.isMyself()) { return QVariant::fromValue(*this); }
        const auto i = index.frontCasted<ColumnIndex>();
        switch (i)
        {
        case IndexDomain: return QVariant::fromValue(m_domain);
        case IndexNetworkName: return QVariant::fromValue(m_config.getNetworkName());
        case IndexUser: return m_user.propertyByIndex(index.copyFrontRemoved());
        case IndexIsConfigLoaded: return QVariant::fromValue(m_isConfigLoaded);
        default: return CValueObject::propertyByIndex(index);
        }
    }

    void CNetwork::setPropertyByIndex(CPropertyIndexRef index, const QVariant &variant)
    {
        if (index.isMyself())
        {
            (*this) = variant.value<CNetwork>();
            return;
        }
        const auto i = index.frontCasted<ColumnIndex>();
        switch (i)
        {
        case IndexDomain: m_domain = variant.value<QString>(); break;
        case IndexUser: m_user.setPropertyByIndex(index.copyFrontRemoved(), variant); break;
        default: CValueObject::setPropertyByIndex(index, variant); break;
        }
    }

    int CNetwork::comparePropertyByIndex(CPropertyIndexRef index, const CNetwork &compareValue) const
    {
        if (index.isMyself()) { return m_domain.compare(compareValue.m_domain); }
        const auto i = index.frontCasted<ColumnIndex>();
        switch (i)
        {
        case IndexDomain: return m_domain.compare(compareValue.m_domain, Qt::CaseInsensitive);
        case IndexNetworkName:
            return m_config.getNetworkName().compare(compareValue.m_config.getNetworkName(), Qt::CaseInsensitive);
        case IndexIsConfigLoaded: return Compare::compare(m_isConfigLoaded, compareValue.m_isConfigLoaded);
        default: break;
        }
        SWIFT_VERIFY_X(false, Q_FUNC_INFO, qUtf8Printable("No comparison for index " + index.toQString()));
        return 0;
    }
} // namespace swift::misc::network
