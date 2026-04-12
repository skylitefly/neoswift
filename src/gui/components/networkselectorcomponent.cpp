// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "networkselectorcomponent.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>

#include "gui/components/serverlistselector.h"
#include "gui/uppercasevalidator.h"

using namespace swift::core::network;
using namespace swift::misc;
using namespace swift::misc::aviation;
using namespace swift::misc::network;
using namespace swift::misc::network::data;
using namespace swift::misc::network::settings;

namespace swift::gui::components
{
    CNetworkSelectorComponent::CNetworkSelectorComponent(QWidget *parent) : QFrame(parent)
    {
        // ── Network selection row ─────────────────────────────────────────
        auto *rowNetwork = new QHBoxLayout;
        m_cbNetwork = new QComboBox(this);
        m_cbNetwork->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_cbNetwork->setToolTip("Select the flight network to connect to");

        m_pbNetworkSettings = new QPushButton("Settings", this);
        m_pbNetworkSettings->setToolTip("Open network settings");

        rowNetwork->addWidget(new QLabel("Network:", this));
        rowNetwork->addWidget(m_cbNetwork);
        rowNetwork->addWidget(m_pbNetworkSettings);

        // ── Server selection row ──────────────────────────────────────────
        auto *rowServer = new QHBoxLayout;
        m_serverSelector = new CServerListSelector(this);
        m_serverSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_pbRefresh = new QPushButton("\u21bb", this);
        m_pbRefresh->setToolTip("Re-fetch network configuration and server list");
        m_pbRefresh->setFixedWidth(30);

        rowServer->addWidget(new QLabel("Server:", this));
        rowServer->addWidget(m_serverSelector);
        rowServer->addWidget(m_pbRefresh);

        // ── Options row ───────────────────────────────────────────────────
        auto *rowOptions = new QHBoxLayout;
        m_cbObserver = new QCheckBox("Observer", this);
        m_cbObserver->setToolTip("Connect as observer (read-only, not visible to others)");

        auto *lblPartner = new QLabel("Co-pilot:", this);
        m_lePartnerCallsign = new QLineEdit(this);
        m_lePartnerCallsign->setMaxLength(10);
        m_lePartnerCallsign->setPlaceholderText("callsign");
        m_lePartnerCallsign->setToolTip("Optional: callsign of your co-pilot / partner");

        constexpr int MaxLen = 10;
        auto *ucv = new CUpperCaseValidator(0, MaxLen, m_lePartnerCallsign);
        ucv->setAllowedCharacters09AZ();
        m_lePartnerCallsign->setValidator(ucv);

        rowOptions->addWidget(m_cbObserver);
        rowOptions->addSpacing(8);
        rowOptions->addWidget(lblPartner);
        rowOptions->addWidget(m_lePartnerCallsign);
        rowOptions->addStretch();

        // ── Assemble ──────────────────────────────────────────────────────
        auto *vl = new QVBoxLayout(this);
        vl->setContentsMargins(2, 2, 2, 2);
        vl->setSpacing(4);
        vl->addLayout(rowNetwork);
        vl->addLayout(rowServer);
        vl->addLayout(rowOptions);

        // ── Connections ───────────────────────────────────────────────────
        connect(m_cbNetwork, qOverload<int>(&QComboBox::currentIndexChanged), this,
                &CNetworkSelectorComponent::onNetworkIndexChanged);
        connect(m_pbNetworkSettings, &QPushButton::pressed, this, &CNetworkSelectorComponent::requestNetworkSettings);
        connect(m_pbRefresh, &QPushButton::pressed, this, &CNetworkSelectorComponent::onRefreshServersPressed);
        connect(m_serverSelector, &CServerListSelector::serverChanged, this,
                &CNetworkSelectorComponent::currentServerChanged);

        // Populate from persisted settings
        this->reloadNetworks();
    }

    CNetwork CNetworkSelectorComponent::getSelectedNetwork() const
    {
        const int idx = m_cbNetwork->currentIndex();
        if (idx < 0) { return {}; }
        const CNetworkList networks = m_networks.get();
        if (idx >= networks.size()) { return {}; }
        return networks[idx];
    }

    CServer CNetworkSelectorComponent::getSelectedServer() const { return m_serverSelector->currentServer(); }

    CLoginMode CNetworkSelectorComponent::getLoginMode() const
    {
        return m_cbObserver->isChecked() ? CLoginMode::Observer : CLoginMode::Pilot;
    }

    void CNetworkSelectorComponent::setLoginMode(CLoginMode mode)
    {
        m_cbObserver->setChecked(mode == CLoginMode::Observer);
    }

    bool CNetworkSelectorComponent::hasPartnerCallsign() const { return !m_lePartnerCallsign->text().isEmpty(); }

    CCallsign CNetworkSelectorComponent::getPartnerCallsign() const
    {
        if (m_lePartnerCallsign->text().isEmpty()) { return {}; }
        return { m_lePartnerCallsign->text(), CCallsign::Aircraft };
    }

    void CNetworkSelectorComponent::reloadNetworks()
    {
        const CNetworkList networks = m_networks.get();
        const QString lastDomain = m_lastNetwork.get().getDomain();

        // Rebuild combo, suppress onNetworkIndexChanged during rebuild
        QSignalBlocker blocker(m_cbNetwork);
        m_cbNetwork->clear();
        int preSelectIdx = -1;
        for (int i = 0; i < networks.size(); ++i)
        {
            const CNetwork &net = networks[i];
            const QString label = net.hasLoadedConfig() ? net.getConfig().getNetworkName() : net.getDomain();
            m_cbNetwork->addItem(label);
            if (net.getDomain() == lastDomain) { preSelectIdx = i; }
        }

        if (preSelectIdx >= 0) { m_cbNetwork->setCurrentIndex(preSelectIdx); }
        else if (!networks.isEmpty()) { m_cbNetwork->setCurrentIndex(0); }

        // Update server list for whatever is now selected
        const int idx = m_cbNetwork->currentIndex();
        if (idx >= 0 && idx < networks.size())
        {
            const CNetwork &selected = networks[idx];
            this->populateServers(selected);

            // Keep the last-network cache aligned with the currently selected network object.
            // The cache is consumed by the FSD/audio contexts during connect.
            m_lastNetwork.set(selected);
        }
        else { m_serverSelector->setServers({}); }
    }

    void CNetworkSelectorComponent::onNetworkIndexChanged(int index)
    {
        const CNetworkList networks = m_networks.get();
        if (index < 0 || index >= networks.size()) { return; }

        const CNetwork &net = networks[index];
        this->populateServers(net);
        m_lastNetwork.set(net);
        emit this->currentServerChanged(m_serverSelector->currentServer());
    }

    void CNetworkSelectorComponent::onRefreshServersPressed()
    {
        const int idx = m_cbNetwork->currentIndex();
        const CNetworkList networks = m_networks.get();
        if (idx < 0 || idx >= networks.size()) { return; }

        m_pbRefresh->setEnabled(false);
        m_pbRefresh->setText("…");

        QPointer<CNetworkSelectorComponent> myself(this);
        m_discoveryService.discoverAndFetchAll(networks[idx],
                                               { this, [=](bool success, const CNetwork &discovered) mutable {
                                                    if (!myself) { return; }
                                                    m_pbRefresh->setEnabled(true);
                                                    m_pbRefresh->setText("\u21bb");
                                                    if (!success) { return; }

                                                    CNetworkList updated = m_networks.get();
                                                    const int currentIdx = m_cbNetwork->currentIndex();
                                                    if (currentIdx >= 0 && currentIdx < updated.size())
                                                    {
                                                        updated[currentIdx] = discovered;
                                                        m_networks.set(updated);
                                                        this->populateServers(discovered);
                                                        m_lastNetwork.set(discovered);
                                                    }
                                                } });
    }

    void CNetworkSelectorComponent::populateServers(const CNetwork &network)
    {
        const CServerList servers = network.getServers();
        m_serverSelector->setServers(servers);
        // Try to preselect the last used server name for this network
        if (!servers.isEmpty()) { m_serverSelector->preSelect(servers.front().getName()); }
    }
} // namespace swift::gui::components
