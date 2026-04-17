// SPDX-FileCopyrightText: Copyright (C) 2017 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "configurationwizard.h"

#include <QPointer>
#include <QTimer>

#include "ui_configurationwizard.h"

#include "gui/guiapplication.h"
#include "gui/guiutility.h"
#include "misc/applicationinfolist.h"
#include "misc/directoryutils.h"
#include "misc/math/mathutils.h"
#include "misc/simulation/simulatorplugininfo.h"

using namespace swift::misc;
using namespace swift::misc::math;
using namespace swift::misc::simulation;

namespace swift::gui::components
{
    CConfigurationWizard::CConfigurationWizard(QWidget *parent) : QWizard(parent), ui(new Ui::CConfigurationWizard)
    {
        ui->setupUi(this);
        this->setWindowFlags(windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint |
                             Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

        ui->wp_CopyModels->setConfigComponent(ui->comp_CopyModels);
        ui->wp_CopySettingsAndCaches->setConfigComponent(ui->comp_CopySettingsAndCachesComponent);
        ui->wp_Simulator->setConfigComponent(ui->comp_Simulator);
        ui->wp_SimulatorSetup->setConfigComponent(ui->comp_SimulatorSetup);
        ui->wp_SimulatorSpecific->setConfigComponent(ui->comp_InstallXSwiftBus, ui->comp_InstallFsxTerrainProbe);
        ui->wp_DataLoad->setConfigComponent(ui->comp_DataLoad);
        ui->wp_Hotkeys->setConfigComponent(ui->comp_Hotkeys);
        ui->wp_Legal->setConfigComponent(ui->comp_LegalInformation);
        ui->comp_Hotkeys->registerDummyPttEntry();
        this->setButtonText(CustomButton1, "skip");

        // no other versions, skip copy pages
        // disabled afetr discussion with RP as it is confusing
        // if (!CApplicationInfoList::hasOtherSwiftDataDirectories()) { this->setStartId(SelectSimulator); }

        ui->tb_SimulatorSpecific->setCurrentWidget(ui->comp_InstallXSwiftBus);

        // Qt's internal title/subtitle QLabels have no objectName, so stylesheets and
        // findChild() both miss them. ClassicStyle also gives title and subtitle the
        // same point size by default. Switch to rich-text titles so inline HTML font
        // sizing is actually honored by QLabel.
        this->setTitleFormat(Qt::RichText);
        this->setSubTitleFormat(Qt::RichText);
        for (int id : this->pageIds())
        {
            QWizardPage *p = this->page(id);
            if (!p) { continue; }
            p->setTitle(QStringLiteral("<span style=\"font-size:14pt; font-weight:700;\">%1</span>")
                            .arg(p->title().toHtmlEscaped()));
            p->setSubTitle(QStringLiteral("<span style=\"font-size:9pt; font-weight:400;\">%1</span>")
                               .arg(p->subTitle().toHtmlEscaped()));
        }


        // Silently trigger background data loading — no page visit needed
        QTimer::singleShot(500, this, [this] { ui->comp_DataLoad->loadAllFromShared(); });

        const QList<int> ids = this->pageIds();
        const auto mm = std::minmax_element(ids.begin(), ids.end());
        m_maxId = *mm.second;
        m_minId = *mm.first;

        connect(this, &QWizard::currentIdChanged, this, &CConfigurationWizard::wizardCurrentIdChanged);
        connect(this, &QWizard::customButtonClicked, this, &CConfigurationWizard::clickedCustomButton);
        connect(this, &QWizard::rejected, this, &CConfigurationWizard::ended);
        connect(this, &QWizard::accepted, this, &CConfigurationWizard::ended);

        Q_ASSERT_X(sGui, Q_FUNC_INFO, "missing sGui");
        const QPointer<CConfigurationWizard> myself(this);
        connect(this, &QWizard::helpRequested, sGui, [=] {
            if (!myself) { return; }
            if (!sGui || sGui->isShuttingDown()) { return; }
            sGui->showHelp("install");
        });

        this->setScreenGeometry();
        CGuiUtility::setWizardButtonWidths(this);
    }

    CConfigurationWizard::~CConfigurationWizard() = default;

    bool CConfigurationWizard::lastStepSkipped() const { return m_skipped; }

    int CConfigurationWizard::nextId() const
    {
        const int id = currentId();
        const bool hasOtherVersions = CApplicationInfoList::hasOtherSwiftDataDirectories();

        switch (id)
        {
        case Legal:
            // DataLoad is handled silently; skip import pages when no previous version exists
            return hasOtherVersions ? CopyModels : SelectSimulator;

        case DataLoad:
            // Should not be reached in normal flow, but handle gracefully
            return hasOtherVersions ? CopyModels : SelectSimulator;

        case CopyModels:
            return CopySettingsAndCaches;

        case FirstModelSet:
        {
            // Skip simulator-specific installs when the user does not use X-Plane or FSX/P3D
            const QStringList enabled = m_enabledSimulators.get();
            const bool needsPlugin =
                enabled.contains(CSimulatorPluginInfo::xplanePluginIdentifier()) ||
                enabled.contains(CSimulatorPluginInfo::fsxPluginIdentifier()) ||
                enabled.contains(CSimulatorPluginInfo::p3dPluginIdentifier());
            return needsPlugin ? XSwiftBus : ConfigHotkeys;
        }

        default:
            return QWizard::nextId();
        }
    }

    bool CConfigurationWizard::lastWizardStepSkipped(const QWizard *standardWizard)
    {
        const auto *wizard = qobject_cast<const CConfigurationWizard *>(standardWizard);
        return wizard && wizard->lastStepSkipped();
    }

    void CConfigurationWizard::wizardCurrentIdChanged(int id)
    {
        const int previousId = m_previousId;
        const bool backward = id < previousId;
        const bool skipped = m_skipped;
        m_previousId = id; // update
        m_skipped = false; // reset
        Q_UNUSED(skipped);
        Q_UNUSED(backward);

        this->setParentOpacity(0.5);
        const QWizardPage *page = this->currentPage();
        Q_UNUSED(page);

        this->setOption(HaveCustomButton1, id != m_maxId);
    }

    void CConfigurationWizard::clickedCustomButton(int which)
    {
        if (which == static_cast<int>(CustomButton1))
        {
            m_skipped = true;
            this->next();
        }
        else { m_skipped = false; }
    }

    void CConfigurationWizard::ended() { this->setParentOpacity(1.0); }

    void CConfigurationWizard::setParentOpacity(qreal opacity)
    {
        QWidget *parent = this->parentWidget();
        if (!parent) { return; }
        if (CMathUtils::epsilonEqual(parent->windowOpacity(), opacity)) { return; }
        parent->setWindowOpacity(opacity);
    }

    void CConfigurationWizard::setScreenGeometry()
    {
        if (!sGui) { return; }
        const QRect g = CGuiApplication::currentScreenGeometry();

        // 1280/720 on 4k hires
        // 1920/1280 on non hires 1920 displays
        const int gw = g.width();
        const int gh = g.height();
        const int calcW = qRound(gw * 0.8);
        const int calcH = qRound(gh * 0.9); // normally critical as buttons are hidden

        // do not get too huge
        const int w = qMin(900, calcW);
        const int h = qMin(750, calcH);
        this->resize(w, h);
    }
} // namespace swift::gui::components
