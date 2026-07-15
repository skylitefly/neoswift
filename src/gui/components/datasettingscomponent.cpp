// SPDX-FileCopyrightText: Copyright (C) 2015 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "gui/components/datasettingscomponent.h"

#include <QCheckBox>

#include "ui_datasettingscomponent.h"

#include "misc/logmessage.h"

using namespace swift::gui;
using namespace swift::misc;
using namespace swift::core::db;

namespace swift::gui::components
{
    CDataSettingsComponent::CDataSettingsComponent(QWidget *parent)
        : COverlayMessagesFrame(parent), ui(new Ui::CDataSettingsComponent)
    {
        ui->setupUi(this);
        ui->comp_GuiSettings->hideOpacity(true);
        ui->cb_LoadDbDataAtStartup->setChecked(m_loadDbDataAtStartup.get());
        connect(ui->cb_LoadDbDataAtStartup, &QCheckBox::toggled, this,
                &CDataSettingsComponent::loadDbDataAtStartupChanged);
    }

    CDataSettingsComponent::~CDataSettingsComponent() = default;

    void CDataSettingsComponent::setBackgroundUpdater(const CBackgroundDataUpdater *updater)
    {
        ui->comp_ModelSettings->setBackgroundUpdater(updater);
    }

    void CDataSettingsComponent::loadDbDataAtStartupChanged(bool enabled)
    {
        const CStatusMessage status = m_loadDbDataAtStartup.setAndSave(enabled);
        CLogMessage::preformatted(status);
    }
} // namespace swift::gui::components
