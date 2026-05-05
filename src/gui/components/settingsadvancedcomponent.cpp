// SPDX-FileCopyrightText: Copyright (C) 2015 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "gui/components/settingsadvancedcomponent.h"

#include "ui_settingsadvancedcomponent.h"

namespace swift::gui::components
{
    CSettingsAdvancedComponent::CSettingsAdvancedComponent(QWidget *parent)
        : QFrame(parent), ui(new Ui::CSettingsAdvancedComponent)
    {
        ui->setupUi(this);
    }

    CSettingsAdvancedComponent::~CSettingsAdvancedComponent() = default;

} // namespace swift::gui::components
