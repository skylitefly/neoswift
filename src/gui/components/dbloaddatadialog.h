// SPDX-FileCopyrightText: Copyright (C) 2018 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef SWIFT_GUI_COMPONENTS_DBLOADDATADIALOG_H
#define SWIFT_GUI_COMPONENTS_DBLOADDATADIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include <QUrl>

#include "gui/swiftguiexport.h"
#include "misc/network/entityflags.h"

namespace Ui
{
    class CDbLoadDataDialog;
}
namespace swift::gui::components
{
    /*!
     * Load new data from the database as a progress dialog.
     */
    class SWIFT_GUI_EXPORT CDbLoadDataDialog : public QDialog
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CDbLoadDataDialog(QWidget *parent = nullptr);

        //! Destructor
        ~CDbLoadDataDialog() override;

        //! Start loading newer or empty entities.
        bool newerOrEmptyEntitiesDetected(swift::misc::network::CEntityFlags::Entity loadEntities);

    private:
        //! Data are/have been read
        void onDataRead(swift::misc::network::CEntityFlags::Entity entity,
                        swift::misc::network::CEntityFlags::ReadState state, int number, const QUrl &url);

        //! Dialog rejected
        void onRejected();

        QScopedPointer<Ui::CDbLoadDataDialog> ui;
        swift::misc::network::CEntityFlags::Entity m_pendingEntities = swift::misc::network::CEntityFlags::NoEntity;
        int m_pendingEntitiesCount = -1;
    };
} // namespace swift::gui::components

#endif // SWIFT_GUI_COMPONENTS_DBLOADDATADIALOG_H
