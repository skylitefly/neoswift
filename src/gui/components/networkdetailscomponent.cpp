// SPDX-FileCopyrightText: Copyright (C) 2019 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "networkdetailscomponent.h"

#include <QVBoxLayout>

#include "networkselectorcomponent.h"

using namespace swift::misc::network;
using namespace swift::misc::aviation;

namespace swift::gui::components
{
    CNetworkDetailsComponent::CNetworkDetailsComponent(QWidget *parent) : QFrame(parent)
    {
        m_networkSelector = new CNetworkSelectorComponent(this);

        auto *vl = new QVBoxLayout(this);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->addWidget(m_networkSelector);

        connect(m_networkSelector, &CNetworkSelectorComponent::currentServerChanged, this,
                &CNetworkDetailsComponent::currentServerChanged);
        connect(m_networkSelector, &CNetworkSelectorComponent::requestNetworkSettings, this,
                &CNetworkDetailsComponent::requestNetworkSettings);
    }

    CNetworkDetailsComponent::~CNetworkDetailsComponent() = default;

    CLoginMode CNetworkDetailsComponent::getLoginMode() const { return m_networkSelector->getLoginMode(); }

    void CNetworkDetailsComponent::setLoginMode(CLoginMode mode) { m_networkSelector->setLoginMode(mode); }

    CServer CNetworkDetailsComponent::getCurrentServer() const { return m_networkSelector->getSelectedServer(); }

    bool CNetworkDetailsComponent::hasPartnerCallsign() const { return m_networkSelector->hasPartnerCallsign(); }

    CCallsign CNetworkDetailsComponent::getPartnerCallsign() const { return m_networkSelector->getPartnerCallsign(); }

} // namespace swift::gui::components
