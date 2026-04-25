// SPDX-FileCopyrightText: Copyright (C) 2013 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include <QAction>
#include <QHBoxLayout>
#include <QPointer>
#include <QPushButton>
#include <QScopedPointer>
#include <QStackedWidget>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "swiftguistd.h"
#include "ui_swiftguistd.h"

#include "config/buildconfig.h"
#include "core/context/contextaudio.h"
#include "core/context/contextnetwork.h"
#include "core/context/contextownaircraft.h"
#include "core/context/contextsimulator.h"
#include "core/corefacade.h"
#include "core/webdataservices.h"
#include "gui/components/aircraftcomponent.h"
#include "gui/components/atcstationcomponent.h"
#include "gui/components/cockpitcomponent.h"
#include "gui/components/commandinput.h"
#include "gui/components/flightplancomponent.h"
#include "gui/components/interpolationcomponent.h"
#include "gui/components/internalscomponent.h"
#include "gui/components/logcomponent.h"
#include "gui/components/logincomponent.h"
#include "gui/components/mappingcomponent.h"
#include "gui/components/navigatordialog.h"
#include "gui/components/radarcomponent.h"
#include "gui/components/settingscomponent.h"
#include "gui/components/simulatorcomponent.h"
#include "gui/components/textmessagecomponent.h"
#include "gui/components/usercomponent.h"
#include "gui/guiapplication.h"
#include "gui/managedstatusbar.h"
#include "gui/overlaymessagesframe.h"
#include "gui/stylesheetutility.h"
#include "misc/loghandler.h"
#include "misc/logmessage.h"
#include "misc/logpattern.h"
#include "misc/identifier.h"
#include "misc/network/networkutils.h"
#include "misc/sharedstate/datalinkdbus.h"
#include "misc/simulation/simulatedaircraft.h"
#include "misc/slot.h"
#include "misc/statusmessage.h"
#include "misc/aviation/transponder.h"
#include "sound/audioutilities.h"

using namespace swift::config;
using namespace swift::core;
using namespace swift::core::context;
using namespace swift::misc;
using namespace swift::misc::aviation;
using namespace swift::misc::network;
using namespace swift::misc::input;
using namespace swift::gui;
using namespace swift::gui::components;

void SwiftGuiStd::init()
{
    // POST(!) GUI init
    Q_ASSERT_X(sGui, Q_FUNC_INFO, "Missing sGui");
    Q_ASSERT_X(sGui->getWebDataServices(), Q_FUNC_INFO, "Missing web services");
    Q_ASSERT_X(sGui->supportsContexts(), Q_FUNC_INFO, "Missing contexts");

    if (m_init) { return; }

    this->setVisible(false); // hide all, so no flashing windows during init
    m_mwaStatusBar = &m_statusBar;
    m_mwaOverlayFrame = ui->fr_CentralFrameInside;
    sGui->initMainApplicationWidget(this);

    // log messages
    m_logHistoryForOverlay.setFilter(CLogPattern().withSeverityAtOrAbove(CStatusMessage::SeverityError));
    m_logHistoryForLogButtons.setFilter(CLogPattern().withSeverityAtOrAbove(SeverityWarning));
    connect(&m_logHistoryForOverlay, &CLogHistoryReplica::elementAdded, this, [this](const CStatusMessage &message) {
        //! \todo filter out validation messages at CLogPattern level
        if (!message.getCategories().contains(CLogCategories::validation()))
        {
            ui->fr_CentralFrameInside->showOverlayMessage(message);
        }
    });
    connect(&m_logHistoryForLogButtons, &CLogHistoryReplica::elementAdded, this, [this](const CStatusMessage &message) {
        if (message.getSeverity() == CStatusMessage::SeverityError) { m_statusBar.showErrorButton(); }
        else if (message.getSeverity() == CStatusMessage::SeverityWarning) { m_statusBar.showWarningButton(); }
    });
    m_logHistoryForOverlay.initialize(sApp->getDataLinkDBus());
    m_logHistoryForLogButtons.initialize(sApp->getDataLinkDBus());

    // style
    this->initStyleSheet();

    // with frameless window, we shift menu and statusbar into central widget
    // http://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
    if (this->isFrameless())
    {
        // wrap menu in layout, add button to menu bar and insert on top
        QHBoxLayout *menuBarLayout = this->addFramelessCloseButton(ui->mb_MainMenuBar);
        ui->vl_CentralWidgetOutside->insertLayout(0, menuBarLayout, 0);

        // move the status bar into the frame
        // (otherwise it is dangling outside the frame as it belongs to the window)
        ui->sb_MainStatusBar->setParent(ui->wi_CentralWidgetOutside);
        ui->vl_CentralWidgetOutside->addWidget(ui->sb_MainStatusBar, 0);

        // grip
        this->addFramelessSizeGripToStatusBar(ui->sb_MainStatusBar);
    }

    // timers
    m_timerContextWatchdog.setObjectName(this->objectName().append(":m_timerContextWatchdog"));

    // info bar and status bar
    m_statusBar.initStatusBar(ui->sb_MainStatusBar);
    connect(&m_statusBar, &CManagedStatusBar::requestLogPage, this, &SwiftGuiStd::displayLog);

    // navigator
    m_navigator->addAction(this->getToggleWindowVisibilityAction(m_navigator.data()));
    m_navigator->addAction(this->getWindowNormalAction(m_navigator.data()));
    m_navigator->addAction(this->getWindowMinimizeAction(m_navigator.data()));
    m_navigator->addAction(this->getToggleStayOnTopAction(m_navigator.data()));
    m_navigator->buildNavigator(1);

    // wire GUI signals
    this->initGuiSignals();

    // signal / slots contexts / timers
    Q_ASSERT_X(sGui->getIContextNetwork(), Q_FUNC_INFO, "Missing network context");
    Q_ASSERT_X(sGui->getIContextSimulator(), Q_FUNC_INFO, "Missing simulator context");

    bool s = connect(sGui->getIContextNetwork(), &IContextNetwork::connectionStatusChanged, this,
                     &SwiftGuiStd::onConnectionStatusChanged, Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(sGui->getIContextNetwork(), &IContextNetwork::kicked, this, &SwiftGuiStd::onKickedFromNetwork,
                Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(sGui->getIContextSimulator(), &IContextSimulator::validatedModelSet, this,
                &SwiftGuiStd::onValidatedModelSet, Qt::QueuedConnection);
    Q_ASSERT(s);
    s = connect(&m_timerContextWatchdog, &QTimer::timeout, this, &SwiftGuiStd::handleTimerBasedUpdates);
    Q_ASSERT(s);

    if (sGui->getIContextAudio())
    {
        s = connect(sGui->getIContextAudio(), &IContextAudio::voiceClientFailure, this,
                    &SwiftGuiStd::onAudioClientFailure, Qt::QueuedConnection);
        Q_ASSERT(s);
    }
    if (sGui->getCContextAudioBase())
    {
        connect(sGui->getCContextAudioBase(), &CContextAudioBase::changedOutputMute, this,
                [this](bool muted) {
                    ui->pb_SoundMute->setChecked(muted);
                    this->updateStatusInfoTooltip();
                },
                Qt::QueuedConnection);
    }
    if (sGui->getIContextOwnAircraft())
    {
        connect(sGui->getIContextOwnAircraft(), &IContextOwnAircraft::changedAircraftCockpit, this,
                [this](const swift::misc::simulation::CSimulatedAircraft &aircraft, const CIdentifier &) {
                    const CTransponder transponder = aircraft.getTransponder();
                    const bool ident = transponder.getTransponderMode() == CTransponder::StateIdent;
                    ui->pb_CockpitModeC->setText(transponder.getModeAsShortString());
                    ui->pb_CockpitModeC->setToolTip(transponder.toQString());
                    ui->pb_CockpitIdent->setChecked(ident);
                    ui->pb_CockpitIdent->setText(ident ? tr("Ident") : tr("Ident"));
                },
                Qt::QueuedConnection);
        const CTransponder transponder = sGui->getIContextOwnAircraft()->getOwnAircraft().getTransponder();
        ui->pb_CockpitModeC->setText(transponder.getModeAsShortString());
        ui->pb_CockpitModeC->setToolTip(transponder.toQString());
    }
    Q_UNUSED(s)

    // check if DB data have been loaded
    // only check once, so data can be loaded and
    connectOnce(sGui->getWebDataServices(), &CWebDataServices::sharedInfoObjectsRead, this,
                &SwiftGuiStd::checkDbDataLoaded, Qt::QueuedConnection);

    // start timers, update timers will be started when network is connected
    m_timerContextWatchdog.start(2500);

    // init availability
    this->setContextAvailability();

    // data
    this->initialContextDataReads();

    // complete menu
    this->initMenus();

    // do this as last statement, so it can be used as flag
    // whether init has been completed
    this->setVisible(true);

    // more checks
    QPointer<SwiftGuiStd> myself(this);
    QTimer::singleShot(5000, this, [=] {
        if (!myself) { return; }
        this->verifyPrerequisites();
    });

    // trigger version check
    sGui->triggerNewVersionCheck(10 * 1000);

    // done
    m_init = true;
}

void SwiftGuiStd::initStyleSheet()
{
    if (!sGui || sGui->isShuttingDown()) { return; }
    const QString s = sGui->getStyleSheetUtility().styles({ CStyleSheetUtility::fileNameFonts(),
                                                            CStyleSheetUtility::fileNameStandardWidget(),
                                                            CStyleSheetUtility::fileNameSwiftStandardGui() });
    this->setStyleSheet(""); //! \todo KB 2018-07 without clearing the stylesheet I see a crash here for the 2nd update
    this->setStyleSheet(s);
}

void SwiftGuiStd::initGuiSignals()
{
    // primary controls
    ui->comp_AtcStations->setCompactMode(true);
    ui->comp_TextMessages->showRecipientSelector(false);
    ui->comp_TextMessages->showSettings(false);
    ui->comp_TextMessages->showTextMessageEntry(false);
    ui->comp_InfoBarStatus->setVisible(false);
    ui->sp_MainOperations->setSizes({ 190, 540 });

    connect(ui->pb_Connect, &QPushButton::released, this, &SwiftGuiStd::loginRequested);
    connect(ui->pb_CockpitModeC, &QPushButton::released, this, []() {
        if (!sGui || !sGui->getIContextOwnAircraft()) { return; }
        CTransponder transponder = sGui->getIContextOwnAircraft()->getOwnAircraft().getTransponder();
        transponder.toggleTransponderMode();
        sGui->getIContextOwnAircraft()->setTransponderMode(transponder.getTransponderMode());
    });
    connect(ui->pb_CockpitIdent, &QPushButton::released, this, [this]() {
        this->ensureCockpitComponent()->setSelectedTransponderModeStateIdent();
    });
    connect(ui->pb_SoundMaxVolume, &QPushButton::pressed, this, []() {
        if (sGui && sGui->getCContextAudioBase()) { sGui->getCContextAudioBase()->setMasterOutputVolume(100); }
    });
    connect(ui->pb_SoundMute, &QPushButton::released, this, [this]() {
        if (!sGui || !sGui->getCContextAudioBase()) { return; }
        const bool mute = sGui->getCContextAudioBase()->isOutputMuted();
        sGui->getCContextAudioBase()->setOutputMute(!mute);
    });
    connect(ui->pb_Audio, &QPushButton::released, this, [this]() {
        this->ensureCockpitComponent()->showAudio();
        this->showToolDialog(m_cockpitDialog.data());
    });
    connect(ui->pb_FlightPlan, &QPushButton::released, this, &SwiftGuiStd::showFlightPlanWindow);
    connect(ui->pb_Settings, &QPushButton::released, this, &SwiftGuiStd::showSettingsWindow);
    connect(ui->tb_StatusInfo, &QToolButton::released, this, &SwiftGuiStd::showLogWindow);
    this->updateStatusInfoTooltip();

    ui->lep_CommandLineInput->setIdentifier(this->identifier());
    connect(ui->lep_CommandLineInput, &CCommandInput::commandEntered, sGui->getCoreFacade(),
            &CCoreFacade::parseCommandLine);
    connect(ui->lep_CommandLineInput, &CCommandInput::textEntered, ui->comp_TextMessages,
            &CTextMessageComponent::handleGlobalCommandLineText);

    // text component
    connect(ui->comp_TextMessages, &CTextMessageComponent::textMessageTabSelected, this,
            &SwiftGuiStd::focusInTextMessageEntryField, Qt::QueuedConnection);

    // audio
    connect(ui->comp_AtcStations, &CAtcStationComponent::requestAudioWidget, this, [this]() {
        this->ensureCockpitComponent()->showAudio();
        this->showToolDialog(m_cockpitDialog.data());
    });

    // menu
    connect(ui->menu_TestLocationsEDDF, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsEDDM, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsEDNX, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsEDRY, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_TestLocationsLOWW, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);

    connect(ui->menu_WindowToggleNavigator, &QAction::triggered, m_navigator.data(),
            &CNavigatorDialog::toggleNavigatorVisibility);
    connect(ui->menu_WindowFont, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_WindowMinimize, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_WindowToggleOnTop, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_InternalsPage, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_AtcDetails, &QAction::triggered, this, &SwiftGuiStd::showAtcDetailsWindow);
    connect(ui->menu_Cockpit, &QAction::triggered, this, &SwiftGuiStd::showCockpitWindow);
    connect(ui->menu_Aircraft, &QAction::triggered, this, &SwiftGuiStd::showAircraftWindow);
    connect(ui->menu_Users, &QAction::triggered, this, &SwiftGuiStd::showUsersWindow);
    connect(ui->menu_Simulator, &QAction::triggered, this, &SwiftGuiStd::showSimulatorWindow);
    connect(ui->menu_FlightPlan, &QAction::triggered, this, &SwiftGuiStd::showFlightPlanWindow);
    connect(ui->menu_Mapping, &QAction::triggered, this, &SwiftGuiStd::showMappingWindow);
    connect(ui->menu_Interpolation, &QAction::triggered, this, &SwiftGuiStd::showInterpolationWindow);
    connect(ui->menu_Radar, &QAction::triggered, this, &SwiftGuiStd::showRadarWindow);
    connect(ui->menu_Log, &QAction::triggered, this, &SwiftGuiStd::showLogWindow);
    connect(ui->menu_SettingsPage, &QAction::triggered, this, &SwiftGuiStd::showSettingsWindow);
    connect(ui->menu_AutoPublish, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_ToggleIncognito, &QAction::triggered, this, &SwiftGuiStd::onMenuClicked);
    connect(ui->menu_ModelBrowser, &QAction::triggered, this, &SwiftGuiStd::startModelBrowser, Qt::QueuedConnection);
    connect(ui->menu_AfvMap, &QAction::triggered, this, &SwiftGuiStd::startAFVMap, Qt::QueuedConnection);

    connect(m_navigator.data(), &CNavigatorDialog::navigatorClosed, this, &SwiftGuiStd::onNavigatorClosed,
            Qt::QueuedConnection);
    m_navigator->setMainWindow(this);

    // styles
    connect(sGui, &CGuiApplication::styleSheetsChanged, this, &SwiftGuiStd::onStyleSheetsChanged, Qt::QueuedConnection);

    // text messages
    connect(ui->comp_AtcStations, &CAtcStationComponent::requestTextMessageWidget, ui->comp_TextMessages,
            &CTextMessageComponent::showCorrespondingTab,
            Qt::QueuedConnection);

    // command line / text messages
    // here we display SUP messages and such in a central window
    ui->fr_CentralFrameInside->activateTextMessages(true);
    connect(ui->comp_TextMessages, &CTextMessageComponent::displayInInfoWindow, this,
            &SwiftGuiStd::onShowOverlayVariant, Qt::QueuedConnection);
    connect(ui->comp_AtcStations, &CAtcStationComponent::requestTextMessageEntryTab, this,
            &SwiftGuiStd::onShowOverlayInlineTextMessageTab, Qt::QueuedConnection);
    connect(ui->comp_AtcStations, &CAtcStationComponent::requestTextMessageEntryCallsign, this,
            &SwiftGuiStd::onShowOverlayInlineTextMessageCallsign, Qt::QueuedConnection);

    // on top
    connect(sGui, &CGuiApplication::alwaysOnTop, this, &SwiftGuiStd::onToggledWindowsOnTop, Qt::QueuedConnection);
}

void SwiftGuiStd::initialContextDataReads()
{
    this->setContextAvailability();
    if (!m_coreAvailable)
    {
        CLogMessage(this).error(u"No initial data read as network context is not available");
        return;
    }

    this->reloadOwnAircraft(); // init read, independent of traffic network
    CLogMessage(this).info(u"Initial data read");
}

void SwiftGuiStd::stopAllTimers(bool disconnectSignalSlots)
{
    m_timerContextWatchdog.stop();
    if (!disconnectSignalSlots) { return; }
    this->disconnect(&m_timerContextWatchdog);
}
