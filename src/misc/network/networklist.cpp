// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "misc/network/networklist.h"

SWIFT_DEFINE_SEQUENCE_MIXINS(swift::misc::network, CNetwork, CNetworkList)

namespace swift::misc::network
{
    CNetworkList::CNetworkList(const CSequence<CNetwork> &other) : CSequence<CNetwork>(other) {}

    CNetwork CNetworkList::findByDomain(const QString &domain) const
    {
        for (const CNetwork &n : *this)
        {
            if (n.getDomain().compare(domain, Qt::CaseInsensitive) == 0) { return n; }
        }
        return {};
    }

    bool CNetworkList::containsDomain(const QString &domain) const
    {
        return std::any_of(cbegin(), cend(),
                           [&](const CNetwork &n) { return n.getDomain().compare(domain, Qt::CaseInsensitive) == 0; });
    }

    bool CNetworkList::containsConfigUrl(const QString &url) const
    {
        return std::any_of(cbegin(), cend(),
                           [&](const CNetwork &n) { return n.getConfigUrl().compare(url, Qt::CaseInsensitive) == 0; });
    }
} // namespace swift::misc::network
