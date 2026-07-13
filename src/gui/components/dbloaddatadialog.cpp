// SPDX-FileCopyrightText: Copyright (C) 2018 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "gui/components/dbloaddatadialog.h"

#include <QPointer>
#include <QTimer>

#include "ui_dbloaddatadialog.h"

#include "core/webdataservices.h"
#include "gui/guiapplication.h"
#include "misc/logmessage.h"

using namespace swift::misc;
using namespace swift::misc::network;
using namespace swift::core;

namespace swift::gui::components
{
    CDbLoadDataDialog::CDbLoadDataDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CDbLoadDataDialog)
    {
        Q_ASSERT_X(sGui, Q_FUNC_INFO, "Need sGui");
        ui->setupUi(this);
        this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        connect(sGui->getWebDataServices(), &CWebDataServices::dataRead, this, &CDbLoadDataDialog::onDataRead,
                Qt::QueuedConnection);
        connect(this, &CDbLoadDataDialog::rejected, this, &CDbLoadDataDialog::onRejected);
    }

    CDbLoadDataDialog::~CDbLoadDataDialog() = default;

    bool CDbLoadDataDialog::newerOrEmptyEntitiesDetected(CEntityFlags::Entity loadEntities)
    {
        if (m_pendingEntities != CEntityFlags::NoEntity) { return false; } // already loading
        if (loadEntities == CEntityFlags::NoEntity) { return false; }

        m_pendingEntities = sGui->getWebDataServices()->triggerLoadingDirectlyFromSharedFiles(loadEntities, false);
        const int pending = CEntityFlags::numberOfEntities(m_pendingEntities);
        m_pendingEntitiesCount = sGui->getWebDataServices()->getDbInfoObjectsCount(loadEntities);
        ui->pb_Loading->setMaximum(m_pendingEntitiesCount > 0 ? m_pendingEntitiesCount : qMax(pending, 1));
        ui->pb_Loading->setValue(0);
        ui->lbl_Info->setText(tr("Please wait"));
        this->show();
        CGuiApplication::modalWindowToFront();

        if (pending < 1)
        {
            ui->pb_Loading->setValue(ui->pb_Loading->maximum());
            ui->lbl_Info->setText(tr("Done!"));
            QTimer::singleShot(1000, this, &CDbLoadDataDialog::accept);
        }
        return true;
    }

    void CDbLoadDataDialog::onDataRead(CEntityFlags::Entity entity, CEntityFlags::ReadState state, int number,
                                       const QUrl &url)
    {
        if (m_pendingEntities == CEntityFlags::NoEntity) { return; } // not triggered from here
        if (!m_pendingEntities.testFlag(CEntityFlags::entityToEntityFlag(entity))) { return; }

        const QString e = CEntityFlags::entitiesToString(entity);
        if (!CEntityFlags::isFinishedReadStateOrFailure(state)) { return; }
        if (state == CEntityFlags::ReadFailed)
        {
            CLogMessage(this).warning(u"Read failed for %1 from '%2'") << e << url.toString();
        }

        m_pendingEntities &= ~entity;
        const int pending = CEntityFlags::numberOfEntities(m_pendingEntities);
        const int max = ui->pb_Loading->maximum();
        if (m_pendingEntitiesCount < 0) { ui->pb_Loading->setValue(max - pending); }
        else
        {
            m_pendingEntitiesCount -= number;
            ui->pb_Loading->setValue(max - m_pendingEntitiesCount);
        }
        if (pending < 1)
        {
            m_pendingEntitiesCount = -1;
            ui->pb_Loading->setValue(ui->pb_Loading->maximum());
            ui->lbl_Info->setText(tr("Done!"));
            QPointer<CDbLoadDataDialog> myself(this);
            QTimer::singleShot(1000, this, [=] {
                if (!myself) { return; }
                myself->accept();
            });
        }
    }

    void CDbLoadDataDialog::onRejected()
    {
        m_pendingEntities = CEntityFlags::NoEntity;
        m_pendingEntitiesCount = -1;
        ui->pb_Loading->setVisible(false);
    }
} // namespace swift::gui::components
