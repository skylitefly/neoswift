// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_MISC_NETWORK_SETTINGS_NETWORKSETTINGS_H
#define SWIFT_MISC_NETWORK_SETTINGS_NETWORKSETTINGS_H

#include "misc/network/networklist.h"
#include "misc/settingscache.h"

namespace swift::misc::network::settings
{
    //! Persisted list of known networks (one entry per domain the user has added)
    struct TNetworks : public TSettingTrait<CNetworkList>
    {
        //! \copydoc swift::misc::TSettingTrait::key
        static const char *key() { return "network/networks"; }

        //! \copydoc swift::misc::TSettingTrait::humanReadable
        static const QString &humanReadable()
        {
            static const QString name("Known networks");
            return name;
        }

        //! \copydoc swift::misc::TSettingTrait::defaultValue
        static const swift::misc::network::CNetworkList &defaultValue()
        {
            static const CNetworkList dv;
            return dv;
        }
    };
} // namespace swift::misc::network::settings

#endif // SWIFT_MISC_NETWORK_SETTINGS_NETWORKSETTINGS_H
