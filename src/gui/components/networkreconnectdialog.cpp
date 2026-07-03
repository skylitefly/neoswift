// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "networkreconnectdialog.h"

#include "ui_networkreconnectdialog.h"

#include "misc/network/networklist.h"

using namespace swift::misc;
using namespace swift::misc::network;
using namespace swift::misc::network::settings;

namespace swift::gui::components
{
    CNetworkReconnectDialog::CNetworkReconnectDialog(QWidget *parent)
        : QDialog(parent), ui(new Ui::CNetworkReconnectDialog)
    {
        ui->setupUi(this);
        this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        connect(ui->bb_Dialog, &QDialogButtonBox::accepted, this, [this]() {
            m_userReconnectEnabled = ui->cb_EnableReconnect->isChecked();
            this->accept();
        });
        connect(ui->bb_Dialog, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    CNetworkReconnectDialog::~CNetworkReconnectDialog() = default;

    void CNetworkReconnectDialog::setNetwork(const CNetwork &network, bool isNewNetwork)
    {
        const CNetworkConfig &config = network.getConfig();
        const QString networkName =
            network.hasLoadedConfig() ? config.getNetworkName() : network.getDomain();

        if (isNewNetwork)
        {
            this->setWindowTitle(tr("Enable auto-reconnect?"));
            ui->lbl_Description->setText(
                tr("%1 supports restoring the connection when it is unexpectedly lost. Enable auto-reconnect?")
                    .arg(networkName));
            ui->cb_EnableReconnect->setChecked(false);
        }
        else
        {
            this->setWindowTitle(tr("Auto-reconnect settings"));
            ui->lbl_Description->setText(
                tr("Configure whether %1 should automatically try to reconnect after an unexpected disconnect.")
                    .arg(networkName));
            ui->cb_EnableReconnect->setChecked(network.getUserReconnectEnabled());
        }

        ui->val_MaxAttempts->setText(QString::number(config.getReconnectMaxAttempts()));
        ui->val_InitialDelay->setText(tr("%1 s").arg(config.getReconnectInitialDelaySec()));
        ui->val_BackoffMultiplier->setText(QString::number(config.getReconnectBackoffMultiplier()));
        ui->val_MaxDelay->setText(tr("%1 s").arg(config.getReconnectMaxDelaySec()));
        ui->val_AppendAttempt->setText(config.appendReconnectAttemptToCallsign() ? tr("Yes") : tr("No"));
    }

    bool CNetworkReconnectDialog::getUserReconnectEnabled() const { return m_userReconnectEnabled; }

    void CNetworkReconnectDialog::promptAndSave(QWidget *parent, int networkRow, CSetting<TNetworks> &networks,
                                                bool isNewNetwork)
    {
        CNetworkList list = networks.get();
        if (networkRow < 0 || networkRow >= list.size()) { return; }

        CNetworkReconnectDialog dialog(parent);
        dialog.setNetwork(list[networkRow], isNewNetwork);

        const int result = dialog.exec();
        if (!isNewNetwork && result != QDialog::Accepted) { return; }

        const bool enabled = (result == QDialog::Accepted) ? dialog.getUserReconnectEnabled() : false;

        list = networks.get();
        if (networkRow < 0 || networkRow >= list.size()) { return; }

        CNetwork network = list[networkRow];
        network.setUserReconnectEnabled(enabled);
        list[networkRow] = network;
        networks.setAndSave(list);
    }
} // namespace swift::gui::components
