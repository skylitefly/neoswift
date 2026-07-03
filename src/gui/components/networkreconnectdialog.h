// SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_GUI_COMPONENTS_NETWORKRECONNECTDIALOG_H
#define SWIFT_GUI_COMPONENTS_NETWORKRECONNECTDIALOG_H

#include <QDialog>
#include <QScopedPointer>

#include "gui/swiftguiexport.h"
#include "misc/network/network.h"
#include "misc/network/settings/networksettings.h"
#include "misc/settingscache.h"

namespace Ui
{
    class CNetworkReconnectDialog;
}

namespace swift::gui::components
{
    //! Dialog to configure per-network automatic reconnect preference
    class SWIFT_GUI_EXPORT CNetworkReconnectDialog : public QDialog
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CNetworkReconnectDialog(QWidget *parent = nullptr);

        //! Destructor
        ~CNetworkReconnectDialog() override;

        //! Initialize for \a network; \a isNewNetwork changes title and description text
        void setNetwork(const swift::misc::network::CNetwork &network, bool isNewNetwork);

        //! Checkbox value after the dialog is accepted
        bool getUserReconnectEnabled() const;

        //! Show dialog and persist preference for the network at \a networkRow
        //! When \a isNewNetwork is true, cancel saves disabled preference without rolling back the network
        static void promptAndSave(QWidget *parent, int networkRow,
                                  swift::misc::CSetting<swift::misc::network::settings::TNetworks> &networks,
                                  bool isNewNetwork);

    private:
        QScopedPointer<Ui::CNetworkReconnectDialog> ui;
        bool m_userReconnectEnabled = false;
    };
} // namespace swift::gui::components

#endif // SWIFT_GUI_COMPONENTS_NETWORKRECONNECTDIALOG_H
