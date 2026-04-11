// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_MISC_NETWORK_DATA_LASTNETWORK_H
#define SWIFT_MISC_NETWORK_DATA_LASTNETWORK_H

#include "misc/network/network.h"

#include "misc/datacache.h"

namespace swift::misc::network::data
{
    //! Trait for the last (most recently) used network, including its cached config and server list
    struct TLastNetwork : public TDataTrait<CNetwork>
    {
        //! \copydoc swift::core::TDataTrait::key
        static const char *key() { return "lastnetwork"; }

        //! \copydoc swift::core::TDataTrait::isPinned
        static constexpr bool isPinned() { return true; }

        //! \copydoc swift::core::TDataTrait::humanReadable
        static const QString &humanReadable()
        {
            static const QString name("Last used network");
            return name;
        }
    };
} // namespace swift::misc::network::data

#endif // SWIFT_MISC_NETWORK_DATA_LASTNETWORK_H
