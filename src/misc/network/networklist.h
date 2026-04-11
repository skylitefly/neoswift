// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_MISC_NETWORK_NETWORKLIST_H
#define SWIFT_MISC_NETWORK_NETWORKLIST_H

#include <QMetaType>
#include <QString>

#include "misc/collection.h"
#include "misc/network/network.h"
#include "misc/sequence.h"
#include "misc/swiftmiscexport.h"

SWIFT_DECLARE_SEQUENCE_MIXINS(swift::misc::network, CNetwork, CNetworkList)

namespace swift::misc::network
{
    //! List of CNetwork objects
    class SWIFT_MISC_EXPORT CNetworkList : public CSequence<CNetwork>, public mixin::MetaType<CNetworkList>
    {
    public:
        SWIFT_MISC_DECLARE_USING_MIXIN_METATYPE(CNetworkList)
        using CSequence::CSequence;

        //! Default constructor
        CNetworkList() = default;

        //! Construct from base class
        CNetworkList(const CSequence<CNetwork> &other);

        //! Find by domain (case-insensitive)
        CNetwork findByDomain(const QString &domain) const;

        //! True if a network with this domain is in the list
        bool containsDomain(const QString &domain) const;
    };
} // namespace swift::misc::network

Q_DECLARE_METATYPE(swift::misc::network::CNetworkList)
Q_DECLARE_METATYPE(swift::misc::CCollection<swift::misc::network::CNetwork>)

#endif // SWIFT_MISC_NETWORK_NETWORKLIST_H
