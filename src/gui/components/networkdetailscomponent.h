// SPDX-FileCopyrightText: Copyright (C) 2019 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_GUI_COMPONENTS_NETWORKDETAILSCOMPONENT_H
#define SWIFT_GUI_COMPONENTS_NETWORKDETAILSCOMPONENT_H

#include <QFrame>

#include "misc/network/loginmode.h"

namespace swift::misc::aviation
{
    class CCallsign;
}
namespace swift::misc::network
{
    class CServer;
}

namespace swift::gui::components
{
    class CNetworkSelectorComponent;

    //! Thin shell delegating to CNetworkSelectorComponent.
    class CNetworkDetailsComponent : public QFrame
    {
        Q_OBJECT

    public:
        //! Ctor
        explicit CNetworkDetailsComponent(QWidget *parent = nullptr);

        //! Dtor
        ~CNetworkDetailsComponent() override;

        //! Login mode
        swift::misc::network::CLoginMode getLoginMode() const;

        //! Login mode
        void setLoginMode(swift::misc::network::CLoginMode mode);

        //! Current server
        swift::misc::network::CServer getCurrentServer() const;

        //! Pilot or Co-pilot callsign?
        bool hasPartnerCallsign() const;

        //! Pilot or Co-pilot callsign
        swift::misc::aviation::CCallsign getPartnerCallsign() const;

    signals:
        //! Request network settings
        void requestNetworkSettings();

        //! Current selected server changed
        void currentServerChanged(const swift::misc::network::CServer &server);

    private:
        CNetworkSelectorComponent *m_networkSelector = nullptr;
    };
} // namespace swift::gui::components

#endif // SWIFT_GUI_COMPONENTS_NETWORKDETAILSCOMPONENT_H
