// SPDX-FileCopyrightText: Copyright (C) 2015 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "settingsnetworkserverscomponent.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QRegularExpression>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <memory>

#include "misc/network/network.h"
#include "misc/network/networklist.h"

using namespace swift::core::network;
using namespace swift::misc;
using namespace swift::misc::network;
using namespace swift::misc::network::settings;

namespace swift::gui::components
{
    CSettingsNetworkServersComponent::CSettingsNetworkServersComponent(QWidget *parent) : QFrame(parent)
    {
        // ── Table ─────────────────────────────────────────────────────────
        m_table = new QTableWidget(0, 3, this);
        m_table->setHorizontalHeaderLabels({ "Name", "Description", "Domain" });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->verticalHeader()->setVisible(false);

        // ── Buttons ───────────────────────────────────────────────────────
        m_pbAdd = new QPushButton("Add", this);
        m_pbAdd->setToolTip("Add a new network by domain name");
        m_pbDelete = new QPushButton("Delete", this);
        m_pbDelete->setToolTip("Remove the selected network");
        m_pbRefreshSelected = new QPushButton("Refresh selected", this);
        m_pbRefreshSelected->setToolTip("Re-fetch fsd-configuration for the selected network");
        m_pbRefreshAll = new QPushButton("Refresh all", this);
        m_pbRefreshAll->setToolTip("Re-fetch fsd-configuration for all networks");

        auto *btnRow = new QHBoxLayout;
        btnRow->addWidget(m_pbRefreshSelected);
        btnRow->addWidget(m_pbRefreshAll);
        btnRow->addStretch();
        btnRow->addWidget(m_pbAdd);
        btnRow->addWidget(m_pbDelete);

        // ── Group box ─────────────────────────────────────────────────────
        auto *gb = new QGroupBox("Networks", this);
        auto *gbLayout = new QVBoxLayout(gb);
        gbLayout->setContentsMargins(4, 4, 4, 4);
        gbLayout->addWidget(new QLabel(
            "Manage flight networks. Networks are discovered automatically from their domain.", gb));
        gbLayout->addWidget(m_table);
        gbLayout->addLayout(btnRow);

        auto *vl = new QVBoxLayout(this);
        vl->setContentsMargins(2, 2, 2, 2);
        vl->addWidget(gb);
        vl->addStretch();

        connect(m_pbAdd, &QPushButton::clicked, this, &CSettingsNetworkServersComponent::onAddPressed);
        connect(m_pbDelete, &QPushButton::clicked, this, &CSettingsNetworkServersComponent::onDeletePressed);
        connect(m_pbRefreshSelected, &QPushButton::clicked, this,
                &CSettingsNetworkServersComponent::onRefreshSelectedPressed);
        connect(m_pbRefreshAll, &QPushButton::clicked, this,
                &CSettingsNetworkServersComponent::onRefreshAllPressed);

        reloadTable();
    }

    CSettingsNetworkServersComponent::~CSettingsNetworkServersComponent() = default;

    void CSettingsNetworkServersComponent::reloadTable()
    {
        const CNetworkList networks = m_networks.get();
        m_table->setRowCount(networks.size());
        for (int i = 0; i < networks.size(); ++i)
        {
            const CNetwork &net = networks[i];
            const QString name = net.hasLoadedConfig() ? net.getConfig().getNetworkName() : QString();
            const QString desc = net.hasLoadedConfig() ? net.getConfig().getNetworkDescription() : QString();
            m_table->setItem(i, 0, new QTableWidgetItem(name));
            m_table->setItem(i, 1, new QTableWidgetItem(desc));
            m_table->setItem(i, 2, new QTableWidgetItem(net.getDomain()));
        }
    }

    void CSettingsNetworkServersComponent::onAddPressed()
    {
        bool ok = false;
        QString domain = QInputDialog::getText(this, "Add Network",
                                               "Enter network domain name\n(e.g. skylitefly.com):",
                                               QLineEdit::Normal, QString(), &ok);
        if (!ok || domain.trimmed().isEmpty()) { return; }

        // Normalize: lower-case, strip scheme, strip trailing slashes
        domain = domain.trimmed().toLower();
        domain.remove(QRegularExpression(QStringLiteral("^https?://")));
        while (domain.endsWith('/')) { domain.chop(1); }

        if (domain.isEmpty())
        {
            QMessageBox::warning(this, "Add Network", "Invalid domain name.");
            return;
        }

        if (m_networks.get().containsDomain(domain))
        {
            QMessageBox::information(this, "Add Network",
                                     QString("'%1' is already in your network list.").arg(domain));
            return;
        }

        m_pbAdd->setEnabled(false);
        m_pbAdd->setText("…");

        CNetwork network(domain);
        QPointer<CSettingsNetworkServersComponent> myself(this);
        m_discoveryService.discoverAndFetchAll(
            network, { this, [=](bool success, const CNetwork &discovered) mutable {
                if (!myself) { return; }
                m_pbAdd->setEnabled(true);
                m_pbAdd->setText("Add");

                if (!success)
                {
                    QMessageBox::warning(this, "Add Network",
                                         QString("Discovery of '%1' failed.\n"
                                                 "Make sure the domain serves\n"
                                                 "https://%1/.well-known/fsd-configuration.json\n"
                                                 "with correct CORS headers.")
                                             .arg(domain));
                    return;
                }

                CNetworkList networks = m_networks.get();
                networks.push_back(discovered);
                m_networks.set(networks);
            } });
    }

    void CSettingsNetworkServersComponent::onDeletePressed()
    {
        const int row = m_table->currentRow();
        CNetworkList networks = m_networks.get();
        if (row < 0 || row >= networks.size()) { return; }

        const CNetwork &net = networks[row];
        const int res = QMessageBox::question(this, "Delete Network",
                                              QString("Remove '%1' from the network list?").arg(net.getDomain()),
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (res != QMessageBox::Yes) { return; }

        networks.erase(networks.begin() + row);
        m_networks.set(networks);
    }

    void CSettingsNetworkServersComponent::onRefreshSelectedPressed()
    {
        const int row = m_table->currentRow();
        const CNetworkList networks = m_networks.get();
        if (row < 0 || row >= networks.size()) { return; }

        m_pbRefreshSelected->setEnabled(false);
        m_pbRefreshAll->setEnabled(false);

        QPointer<CSettingsNetworkServersComponent> myself(this);
        m_discoveryService.discoverAndFetchAll(
            networks[row], { this, [=](bool success, const CNetwork &discovered) mutable {
                if (!myself) { return; }
                m_pbRefreshSelected->setEnabled(true);
                m_pbRefreshAll->setEnabled(true);
                if (!success) { return; }
                CNetworkList updated = m_networks.get();
                if (row < updated.size())
                {
                    updated[row] = discovered;
                    m_networks.set(updated);
                }
            } });
    }

    void CSettingsNetworkServersComponent::onRefreshAllPressed()
    {
        const CNetworkList networks = m_networks.get();
        if (networks.isEmpty()) { return; }

        m_pbRefreshSelected->setEnabled(false);
        m_pbRefreshAll->setEnabled(false);

        auto pending = std::make_shared<int>(networks.size());
        QPointer<CSettingsNetworkServersComponent> myself(this);

        for (int i = 0; i < networks.size(); ++i)
        {
            m_discoveryService.discoverAndFetchAll(
                networks[i], { this, [=](bool success, const CNetwork &discovered) mutable {
                    if (!myself) { return; }
                    if (success)
                    {
                        CNetworkList updated = m_networks.get();
                        if (i < updated.size())
                        {
                            updated[i] = discovered;
                            m_networks.set(updated);
                        }
                    }
                    if (--*pending <= 0)
                    {
                        m_pbRefreshSelected->setEnabled(true);
                        m_pbRefreshAll->setEnabled(true);
                    }
                } });
        }
    }

} // namespace swift::gui::components
