// SPDX-FileCopyrightText: Copyright (C) 2013 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "ui_swiftguistd.h"

#include "config/buildconfig.h"
#include "core/context/contextapplication.h"
#include "core/context/contextaudio.h"
#include "core/context/contextnetwork.h"
#include "core/context/contextsimulator.h"
#include "core/corefacadeconfig.h"
#include "core/simulator.h"
#include "core/webdataservices.h"
#include "gui/components/aircraftcomponent.h"
#include "gui/components/atcstationcomponent.h"
#include "gui/components/cockpitcomponent.h"
#include "gui/components/dbloaddatadialog.h"
#include "gui/components/flightplancomponent.h"
#include "gui/components/infobarstatuscomponent.h"
#include "gui/components/internalscomponent.h"
#include "gui/components/interpolationcomponent.h"
#include "gui/components/logcomponent.h"
#include "gui/components/logincomponent.h"
#include "gui/components/mappingcomponent.h"
#include "gui/components/modelbrowserdialog.h"
#include "gui/components/radarcomponent.h"
#include "gui/components/settingscomponent.h"
#include "gui/components/simulatorcomponent.h"
#include "gui/components/textmessagecomponent.h"
#include "gui/components/usercomponent.h"
#include "gui/guiapplication.h"
#include "gui/guiutility.h"
#include "gui/overlaymessagesframe.h"
#include "misc/audio/notificationsounds.h"
#include "misc/icons.h"
#include "misc/logcategories.h"
#include "misc/logmessage.h"
#include "misc/threadutils.h"

#ifdef Q_OS_MACOS
#    include "input/macos/macosinpututils.h"
#endif

#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QIcon>
#include <QMessageBox>
#include <QPointer>
#include <QSize>
#include <QStackedWidget>
#include <QStringList>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <QtGlobal>

#include "swiftguistd.h"

class QCloseEvent;
class QEvent;
class QMouseEvent;
class QWidget;

namespace swift::gui
{
    class CEnableForFramelessWindow;
    class IMainWindowAccess;
} // namespace swift::gui
namespace swift::misc
{
    class CIdentifiable;
}

namespace
{
    template <typename Component>
    Component *createToolDialog(QScopedPointer<QDialog> &dialog, QPointer<Component> &component, QWidget *parent,
                                const QString &title, const QSize &size)
    {
        if (!dialog)
        {
            dialog.reset(new QDialog(parent));
            dialog->setWindowTitle(title);
            dialog->setModal(false);
            dialog->setAttribute(Qt::WA_DeleteOnClose, false);
            auto *layout = new QVBoxLayout(dialog.data());
            layout->setContentsMargins(4, 4, 4, 4);
            component = new Component(dialog.data());
            layout->addWidget(component);
            dialog->resize(size);
        }
        return component;
    }
} // namespace

using namespace swift::core;
using namespace swift::core::context;
using namespace swift::gui;
using namespace swift::gui::components;
using namespace swift::misc;
using namespace swift::misc::network;
using namespace swift::misc::aviation;
using namespace swift::misc::physical_quantities;
using namespace swift::misc::geo;
using namespace swift::misc::audio;
using namespace swift::misc::input;
using namespace swift::misc::simulation;
using namespace swift::config;

// Constructor
SwiftGuiStd::SwiftGuiStd(swift::gui::CEnableForFramelessWindow::WindowMode windowMode, QWidget *parent)
    : QMainWindow(parent, CEnableForFramelessWindow::modeToWindowFlags(windowMode)), CIdentifiable(this),
      CEnableForFramelessWindow(windowMode, true, "framelessMainWindow", this), ui(new Ui::SwiftGuiStd)
{
    // GUI
    Q_ASSERT_X(sGui, Q_FUNC_INFO, "Need sGui");
    sGui->registerMainApplicationWidget(this);
    ui->setupUi(this);
    this->setDynamicProperties(windowMode == CEnableForFramelessWindow::WindowFrameless);
    this->init();
}

SwiftGuiStd::~SwiftGuiStd() = default;

void SwiftGuiStd::mouseMoveEvent(QMouseEvent *event)
{
    if (!handleMouseMoveEvent(event)) { QMainWindow::mouseMoveEvent(event); }
}

void SwiftGuiStd::mousePressEvent(QMouseEvent *event)
{
    if (!handleMousePressEvent(event)) { QMainWindow::mousePressEvent(event); }
}

void SwiftGuiStd::mouseReleaseEvent(QMouseEvent *event)
{
    m_framelessDragPosition = QPoint();
    QMainWindow::mouseReleaseEvent(event);
}

void SwiftGuiStd::performGracefulShutdown()
{
    if (!m_init) { return; }
    m_init = false;

    Q_ASSERT_X(CThreadUtils::thisIsMainThread(), Q_FUNC_INFO, "Should shutdown in main thread");

    // shut down all timers
    this->stopAllTimers(true);

    // if we have a context, we shut some things down
    if (m_contextNetworkAvailable)
    {
        if (sGui && sGui->getIContextNetwork() && sGui->getIContextNetwork()->isConnected())
        {
            if (m_contextAudioAvailable)
            {
                sGui->getIContextAudio()->disconnect(this); // break down signal / slots
            }
            sGui->getIContextNetwork()->disconnectFromNetwork();
            sGui->getIContextNetwork()->disconnect(this); // avoid any status update signals, etc.
        }
    }

    // allow some other parts to react
    const QPointer<SwiftGuiStd> myself(this);
    if (sGui) { sGui->processEventsToRefreshGui(); }
    if (!sGui || !myself) { return; } // killed in meantime?

    // tell context GUI is going down
    if (sGui->getIContextApplication()) { sGui->getIContextApplication()->unregisterApplication(identifier()); }

    // allow some other parts to react
    if (sGui) { sGui->processEventsToRefreshGui(); }
}

void SwiftGuiStd::closeEvent(QCloseEvent *event)
{
    if (sGui)
    {
        if (sGui->getIContextNetwork() && sGui->getIContextNetwork()->isConnected())
        {
            // we do not just logoff, but give the user a chance to respond
            event->ignore();
            QPointer<SwiftGuiStd> myself(this);
            QTimer::singleShot(500, this, [=] {
                if (!myself) { return; }
                myself->showLoginWindow();
            });
            return;
        }

        // save settings
        if (sGui->showCloseDialog(this, event) == QDialog::Rejected)
        {
            // already ignored
            return;
        }
    }
    this->performGracefulShutdown();
}

void SwiftGuiStd::changeEvent(QEvent *event)
{
    if (!CEnableForFramelessWindow::handleChangeEvent(event)) { QMainWindow::changeEvent(event); }
}

QAction *SwiftGuiStd::getWindowMinimizeAction(QObject *parent)
{
    const QIcon i(CIcons::changeIconBackgroundColor(this->style()->standardIcon(QStyle::SP_TitleBarMinButton),
                                                    Qt::white, QSize(16, 16)));
    auto a = new QAction(i, "Window minimized", parent);
    connect(a, &QAction::triggered, this, &SwiftGuiStd::showMinimized);
    return a;
}

QAction *SwiftGuiStd::getWindowNormalAction(QObject *parent)
{
    const QIcon i(CIcons::changeIconBackgroundColor(this->style()->standardIcon(QStyle::SP_TitleBarNormalButton),
                                                    Qt::white, QSize(16, 16)));
    auto a = new QAction(i, "Window normal", parent);
    connect(a, &QAction::triggered, this, &SwiftGuiStd::showNormal);
    return a;
}

QAction *SwiftGuiStd::getToggleWindowVisibilityAction(QObject *parent)
{
    const QIcon i(CIcons::changeIconBackgroundColor(this->style()->standardIcon(QStyle::SP_TitleBarShadeButton),
                                                    Qt::white, QSize(16, 16)));
    auto a = new QAction(i, "Toogle main window visibility", parent);
    connect(a, &QAction::triggered, this, &SwiftGuiStd::toggleWindowVisibility);
    return a;
}

QAction *SwiftGuiStd::getToggleStayOnTopAction(QObject *parent)
{
    const QIcon i(CIcons::changeIconBackgroundColor(this->style()->standardIcon(QStyle::SP_TitleBarUnshadeButton),
                                                    Qt::white, QSize(16, 16)));
    auto a = new QAction(i, "Toogle main window on top", parent);
    connect(a, &QAction::triggered, this, &SwiftGuiStd::toggleWindowStayOnTop);
    return a;
}

void SwiftGuiStd::setSettingsPage(int settingsTabIndex)
{
    CSettingsComponent *settings = this->ensureSettingsComponent();
    if (settingsTabIndex < 0) { return; }
    settings->setTab(static_cast<CSettingsComponent::SettingTab>(settingsTabIndex));
}

void SwiftGuiStd::loginRequested()
{
    if (!sGui || sGui->isShuttingDown() || !sGui->getIContextNetwork()) { return; }
    if (sGui->getIContextNetwork()->isConnected())
    {
        CStatusMessage msg = sGui->getIContextNetwork()->disconnectFromNetwork();
        msg.addCategories(this);
        CLogMessage::preformatted(msg);
        return;
    }

    this->ensureLoginComponent();
    this->showLoginWindow();
}

void SwiftGuiStd::showLoginWindow()
{
    if (!sGui || sGui->isShuttingDown() || !sGui->getIContextNetwork()) { return; }
    this->ensureLoginComponent();
    m_loginComponent->refreshFromContexts();
    this->showToolDialog(m_loginDialog.data());
}

void SwiftGuiStd::onKickedFromNetwork(const QString &kickMessage)
{
    this->updateGuiStatusInformation();

    const QString msgText = kickMessage.isEmpty() ? QStringLiteral("You have been kicked from the network") :
                                                    QStringLiteral("You have been kicked: '%1'").arg(kickMessage);
    CLogMessage(this).error(msgText);
    // this->displayInOverlayWindow(CStatusMessage(this, CStatusMessage::SeverityError, msgText));
}

void SwiftGuiStd::onConnectionStatusChanged(const CConnectionStatus &from, const CConnectionStatus &to)
{
    Q_UNUSED(from)
    this->updateGuiStatusInformation();
    this->updateStatusInfoTooltip();
    if (to.isConnected())
    {
        ui->pb_Connect->setText(tr("Disconnect"));
        ui->pb_Connect->setChecked(true);
    }
    else
    {
        ui->pb_Connect->setText(tr("Connect"));
        ui->pb_Connect->setChecked(false);
    }

    // sounds
    switch (to.getConnectionStatus())
    {
    case CConnectionStatus::Connected: this->playNotifcationSound(CNotificationSounds::NotificationLogin); break;
    case CConnectionStatus::Disconnected: this->playNotifcationSound(CNotificationSounds::NotificationLogoff); break;
    default: break;
    }
}

void SwiftGuiStd::handleTimerBasedUpdates()
{
    this->setContextAvailability();
    this->updateGuiStatusInformation();
    this->updateStatusInfoTooltip();

    // own aircraft
    this->reloadOwnAircraft();
}

void SwiftGuiStd::setContextAvailability()
{
    const bool corePreviouslyAvailable = m_coreAvailable;
    const bool isShuttingDown = !sGui || sGui->isShuttingDown();
    if (!isShuttingDown && sGui->getIContextApplication() && !sGui->getIContextApplication()->isEmptyObject())
    {
        // ping to check if core is still alive
        m_coreAvailable = this->isMyIdentifier(sGui->getIContextApplication()->registerApplication(identifier()));
    }
    else { m_coreAvailable = false; }
    if (isShuttingDown) { return; }
    if (m_coreAvailable && m_coreFailures > 0) { m_coreFailures--; }
    else if (!m_coreAvailable && m_coreFailures < MaxCoreFailures) { m_coreFailures++; }
    else if (!m_coreAvailable && !m_displayingDBusReconnect) { this->displayDBusReconnectDialog(); }
    m_contextNetworkAvailable =
        m_coreAvailable && sGui->getIContextNetwork() && !sGui->getIContextNetwork()->isEmptyObject();
    m_contextAudioAvailable = m_coreAvailable && sGui->getIContextAudio() && !sGui->getIContextAudio()->isEmptyObject();

    // react to a change in core's availability
    if (m_coreAvailable != corePreviouslyAvailable)
    {
        if (m_coreAvailable)
        {
            // core has just become available (startup)
            // this HERE is called with and without DBus
            sGui->getIContextApplication()->synchronizeLocalSettings();
        }
    }
}

void SwiftGuiStd::updateGuiStatusInformation()
{
    if (m_coreAvailable)
    {
        static const QString dBusTimestamp("%1 %2");
        static const QString local("local");
        const QString now = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");
        const bool dBus = sGui->getCoreFacadeConfig().requiresDBusConnection();
        ui->comp_InfoBarStatus->setDBusStatus(dBus && m_coreAvailable);
        ui->comp_InfoBarStatus->setDBusTooltip(
            dBus ? dBusTimestamp.arg(now, sGui->getCoreFacadeConfig().getDBusAddress()) : local);
    }
    else
    {
        static const QString unavailable("unavailable");
        ui->comp_InfoBarStatus->setDBusStatus(false);
        ui->comp_InfoBarStatus->setDBusTooltip(unavailable);
    }
    this->updateStatusInfoTooltip();
}

QString SwiftGuiStd::buildStatusInfoTooltip() const
{
    QStringList lines;

    if (!sGui || sGui->isShuttingDown())
    {
        lines << tr("Application: shutting down");
        return lines.join(u'\n');
    }

    const IContextNetwork *network = sGui->getIContextNetwork();
    if (network && !network->isEmptyObject())
    {
        const QString server = network->isConnected() ? network->getConnectedServer().getName() : QString();
        lines << (server.isEmpty() ? tr("Network: disconnected") : tr("Network: connected (%1)").arg(server));
    }
    else { lines << tr("Network: unavailable"); }

    const IContextSimulator *simulator = sGui->getIContextSimulator();
    if (simulator && !simulator->isEmptyObject())
    {
        const ISimulator::SimulatorStatus status = simulator->getSimulatorStatus();
        const QString simulatorName = simulator->getSimulatorPluginInfo().getDescription();
        const QString statusText = ISimulator::statusToString(status);
        lines << (simulatorName.isEmpty() ? tr("Simulator: %1").arg(statusText) :
                                            tr("Simulator: %1 (%2)").arg(statusText, simulatorName));
        lines << tr("Mapper: %1 models").arg(simulator->getModelSetCount());
    }
    else
    {
        lines << tr("Simulator: unavailable");
        lines << tr("Mapper: unavailable");
    }

    if (sGui->getCContextAudioBase())
    {
        const bool started = sGui->getCContextAudioBase()->isAudioStarted();
        const bool muted = sGui->getCContextAudioBase()->isOutputMuted();
        lines << tr("PTT: %1").arg(m_pttActive ? tr("active") : tr("idle"));
        lines << tr("Audio: %1").arg(!started ? tr("stopped") : (muted ? tr("muted") : tr("ready")));
    }
    else
    {
        lines << tr("PTT: unavailable");
        lines << tr("Audio: unavailable");
    }

    if (sGui->getIContextApplication())
    {
        lines << tr("DBus: %1")
                     .arg(sGui->getIContextApplication()->isUsingImplementingObject() ? tr("connected") : tr("local"));
    }
    else { lines << tr("DBus: unavailable"); }

    return lines.join(u'\n');
}

void SwiftGuiStd::updateStatusInfoTooltip()
{
    if (!ui || !ui->tb_StatusInfo) { return; }

    bool warning = false;
    bool error = false;
    if (!sGui || sGui->isShuttingDown()) { error = true; }
    else
    {
        const bool networkConnected = sGui->getIContextNetwork() && !sGui->getIContextNetwork()->isEmptyObject() &&
                                      sGui->getIContextNetwork()->isConnected();
        const bool simulatorAvailable = sGui->getIContextSimulator() && !sGui->getIContextSimulator()->isEmptyObject();
        const bool audioReady = sGui->getCContextAudioBase() && sGui->getCContextAudioBase()->isAudioStarted() &&
                                !sGui->getCContextAudioBase()->isOutputMuted();
        warning = !networkConnected || !simulatorAvailable || !audioReady;
    }

    ui->tb_StatusInfo->setToolTip(this->buildStatusInfoTooltip());
    ui->tb_StatusInfo->setProperty(
        "statusRole", error ? QStringLiteral("error") : (warning ? QStringLiteral("warning") : QStringLiteral("info")));
    CGuiUtility::forceStyleSheetUpdate(ui->tb_StatusInfo);
}

void SwiftGuiStd::onChangedWindowOpacity(int opacity)
{
    qreal o = opacity / 100.0;
    o = o < 0.3 ? 0.3 : o;
    o = o > 1.0 ? 1.0 : o;
    QWidget::setWindowOpacity(o);
    if (m_settingsComponent) { m_settingsComponent->setGuiOpacity(o * 100.0); }
}

void SwiftGuiStd::toggleWindowStayOnTop()
{
    if (sGui) { sGui->toggleStayOnTop(); }
}

void SwiftGuiStd::toggleWindowVisibility()
{
    if (this->isVisible())
    {
        this->hide();
        return;
    }
    this->show();
}

void SwiftGuiStd::onStyleSheetsChanged() { this->initStyleSheet(); }

void SwiftGuiStd::onToggledWindowsOnTop(bool onTop)
{
    if (onTop)
    {
        // here we could automatically display the navigator
        // if (m_navigator) { m_navigator->showNavigator(true); }
        this->show();
    }
}

void SwiftGuiStd::onAudioClientFailure(const CStatusMessage &msg)
{
    if (msg.isEmpty()) { return; }
    if (!sGui || sGui->isShuttingDown()) { return; }

    ui->fr_CentralFrameInside->showOverlayHTMLMessage(msg);
}

void SwiftGuiStd::focusInMainEntryField() { ui->lep_CommandLineInput->setFocus(); }

void SwiftGuiStd::focusInTextMessageEntryField() { ui->comp_TextMessages->focusTextEntry(); }

void SwiftGuiStd::showMinimized() { this->showMinimizedModeChecked(); }

void SwiftGuiStd::showNormal() { this->showNormalModeChecked(); }

void SwiftGuiStd::onNavigatorClosed()
{
    if (!sGui || sGui->isShuttingDown()) { return; }
    this->show();
}

void SwiftGuiStd::verifyPrerequisites()
{
    if (!sGui || sGui->isShuttingDown()) { return; }

    CStatusMessageList msgs;
    if (!sGui->supportsContexts() || !sGui->getIContextSimulator())
    {
        msgs.push_back(CStatusMessage(this).error(u"No simulator context"));
    }
    else { msgs.push_back(sGui->getIContextSimulator()->verifyPrerequisites()); }

#ifdef Q_OS_MACOS
    if (!swift::input::CMacOSInputUtils::hasAccess())
    {
        // A log message about missing permissions is already emitted when initializing the keyboard.
        // But this happens way before initializing the GUI. Hence do the check here again to show an error message
        // to the user
        msgs.push_back(
            CLogMessage(this).error(tr("Cannot access the keyboard. Is \"Input Monitoring\" for swift enabled?")));
    }
#endif

    if (msgs.hasWarningOrErrorMessages())
    {
        if (msgs.size() > 1) { this->displayInOverlayWindow(msgs); }
        else { this->displayInOverlayWindow(msgs.front()); }
    }
}

void SwiftGuiStd::onValidatedModelSet(const CSimulatorInfo &simulator, const CAircraftModelList &valid,
                                      const CAircraftModelList &invalid, bool stopped, const CStatusMessageList &msgs)
{
    using namespace std::chrono_literals;
    // will NOT be called if no errors and setting is "only on errors"
    if (!sGui || sGui->isShuttingDown()) { return; }
    if (QApplication::activeModalWidget())
    {
        // avoid too many "deadlocking" dialogs, display warning instead
        if (invalid.isEmpty()) { return; }
        const CStatusMessage m =
            CLogMessage(this).validationWarning(
                u"Model set validation has found %1 invalid models for '%2', check the model validation")
            << invalid.size() << simulator.toQString(true);
        this->displayInOverlayWindow(m, 5s);
        return;
    }

    this->displayValidationDialog();
    m_validationDialog->validatedModelSet(simulator, valid, invalid, stopped, msgs);
}

void SwiftGuiStd::displayValidationDialog()
{
    if (!sGui || sGui->isShuttingDown()) { return; }
    if (!m_validationDialog) { m_validationDialog.reset(new CAircraftModelSetValidationDialog(this)); }
    m_validationDialog->show();
}

void SwiftGuiStd::checkDbDataLoaded()
{
    if (!sGui || sGui->isShuttingDown()) { return; }
    if (!m_loadDbDataAtStartup.get()) { return; }
    Q_ASSERT_X(sGui->hasWebDataServices(), Q_FUNC_INFO, "Missing web services");
    Q_ASSERT_X(CThreadUtils::thisIsMainThread(), Q_FUNC_INFO, "Wrong thread, needs to run in main thread");
    const CEntityFlags::Entity loadEntities =
        sGui->getWebDataServices()->getSynchronizedEntitiesWithNewerSharedFileOrEmpty(!m_dbDataLoading);
    if (loadEntities == CEntityFlags::NoEntity)
    {
        m_dbDataLoading = false;
        return;
    }

    if (!m_dbLoadDialog) { m_dbLoadDialog.reset(new CDbLoadDataDialog(this)); }
    m_dbLoadDialog->newerOrEmptyEntitiesDetected(loadEntities);
}

void SwiftGuiStd::playNotifcationSound(CNotificationSounds::NotificationFlag notification) const
{
    if (!m_contextAudioAvailable) { return; }
    if (!m_audioSettings.get().isNotificationFlagSet(notification)) { return; }
    if (!sGui || sGui->isShuttingDown()) { return; }
    sGui->getCContextAudioBase()->playNotification(notification, true);
}

void SwiftGuiStd::displayLog() { this->showLogWindow(); }

void SwiftGuiStd::displayNetworkSettings()
{
    if (!sApp || sApp->isShuttingDown()) { return; }
    this->setSettingsPage(CSettingsComponent::SettingTabServers);
    this->showSettingsWindow();
}

void SwiftGuiStd::showToolDialog(QDialog *dialog)
{
    if (!dialog) { return; }
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void SwiftGuiStd::showAtcDetailsWindow()
{
    this->ensureAtcDetailsComponent();
    this->showToolDialog(m_atcDetailsDialog.data());
}

void SwiftGuiStd::showCockpitWindow()
{
    this->ensureCockpitComponent();
    this->showToolDialog(m_cockpitDialog.data());
}

void SwiftGuiStd::showAircraftWindow()
{
    this->ensureAircraftComponent();
    this->showToolDialog(m_aircraftDialog.data());
}

void SwiftGuiStd::showUsersWindow()
{
    this->ensureUserComponent();
    this->showToolDialog(m_usersDialog.data());
}

void SwiftGuiStd::showSimulatorWindow()
{
    this->ensureSimulatorComponent();
    this->showToolDialog(m_simulatorDialog.data());
}

void SwiftGuiStd::showFlightPlanWindow()
{
    this->ensureFlightPlanComponent();
    this->showToolDialog(m_flightPlanDialog.data());
}

void SwiftGuiStd::showMappingWindow()
{
    this->ensureMappingComponent();
    this->showToolDialog(m_mappingDialog.data());
}

void SwiftGuiStd::showInterpolationWindow()
{
    this->ensureInterpolationComponent();
    this->showToolDialog(m_interpolationDialog.data());
}

void SwiftGuiStd::showRadarWindow()
{
    this->ensureRadarComponent();
    this->showToolDialog(m_radarDialog.data());
}

void SwiftGuiStd::showLogWindow()
{
    this->ensureLogComponent();
    this->showToolDialog(m_logDialog.data());
}

void SwiftGuiStd::showSettingsWindow()
{
    this->ensureSettingsComponent();
    this->showToolDialog(m_settingsDialog.data());
}

void SwiftGuiStd::showInternalsWindow()
{
    this->ensureInternalsComponent();
    this->showToolDialog(m_internalsDialog.data());
}

CLoginComponent *SwiftGuiStd::ensureLoginComponent()
{
    const bool created = !m_loginDialog;
    CLoginComponent *component =
        createToolDialog(m_loginDialog, m_loginComponent, this, tr("Connect"), QSize(600, 440));
    if (!created) { return component; }
    connect(component, &CLoginComponent::loginOrLogoffCancelled, m_loginDialog.data(), &QDialog::hide,
            Qt::UniqueConnection);
    connect(component, &CLoginComponent::loginOrLogoffSuccessful, m_loginDialog.data(), &QDialog::hide,
            Qt::UniqueConnection);
    connect(component, &CLoginComponent::loginOrLogoffSuccessful, this,
            [this]() { this->ensureFlightPlanComponent()->loginDataSet(); });
    connect(component, &CLoginComponent::loginDataChangedDigest, this,
            [this]() { this->ensureFlightPlanComponent()->loginDataSet(); });
    connect(component, &CLoginComponent::requestNetworkSettings, this, &SwiftGuiStd::displayNetworkSettings,
            Qt::UniqueConnection);
    connect(component, &CLoginComponent::requestLoginPage, this, &SwiftGuiStd::showLoginWindow, Qt::UniqueConnection);
    return component;
}

CAtcStationComponent *SwiftGuiStd::ensureAtcDetailsComponent()
{
    const bool created = !m_atcDetailsDialog;
    CAtcStationComponent *component = createToolDialog(m_atcDetailsDialog, m_atcDetailsComponent, this,
                                                       tr("Controllers / ATIS / METAR"), QSize(760, 520));
    if (created) { component->setCompactMode(false); }
    return component;
}

CCockpitComponent *SwiftGuiStd::ensureCockpitComponent()
{
    const bool created = !m_cockpitDialog;
    CCockpitComponent *component =
        createToolDialog(m_cockpitDialog, m_cockpitComponent, this, tr("Cockpit"), QSize(600, 420));
    if (!created) { return component; }
    connect(component, &CCockpitComponent::requestTextMessageEntryTab, this,
            &SwiftGuiStd::onShowOverlayInlineTextMessageTab, Qt::UniqueConnection);
    connect(component, &CCockpitComponent::requestTextMessageEntryCallsign, this,
            &SwiftGuiStd::onShowOverlayInlineTextMessageCallsign, Qt::UniqueConnection);
    return component;
}

CAircraftComponent *SwiftGuiStd::ensureAircraftComponent()
{
    const bool created = !m_aircraftDialog;
    CAircraftComponent *component =
        createToolDialog(m_aircraftDialog, m_aircraftComponent, this, tr("Aircraft"), QSize(760, 520));
    if (!created) { return component; }
    connect(component, &CAircraftComponent::requestTextMessageWidget, ui->comp_TextMessages,
            &CTextMessageComponent::showCorrespondingTab, Qt::UniqueConnection);
    return component;
}

CUserComponent *SwiftGuiStd::ensureUserComponent()
{
    const bool created = !m_usersDialog;
    CUserComponent *component = createToolDialog(m_usersDialog, m_userComponent, this, tr("Users"), QSize(640, 460));
    if (!created) { return component; }
    connect(component, &CUserComponent::requestTextMessageWidget, ui->comp_TextMessages,
            &CTextMessageComponent::showCorrespondingTab, Qt::UniqueConnection);
    return component;
}

CSimulatorComponent *SwiftGuiStd::ensureSimulatorComponent()
{
    return createToolDialog(m_simulatorDialog, m_simulatorComponent, this, tr("Simulator"), QSize(760, 520));
}

CFlightPlanComponent *SwiftGuiStd::ensureFlightPlanComponent()
{
    return createToolDialog(m_flightPlanDialog, m_flightPlanComponent, this, tr("Flight Plan"), QSize(800, 600));
}

CMappingComponent *SwiftGuiStd::ensureMappingComponent()
{
    const bool created = !m_mappingDialog;
    CMappingComponent *component =
        createToolDialog(m_mappingDialog, m_mappingComponent, this, tr("Models"), QSize(960, 620));
    if (!created) { return component; }
    connect(component, &CMappingComponent::requestTextMessageWidget, ui->comp_TextMessages,
            &CTextMessageComponent::showCorrespondingTab, Qt::UniqueConnection);
    connect(component, &CMappingComponent::requestValidationDialog, this, &SwiftGuiStd::displayValidationDialog,
            Qt::UniqueConnection);
    return component;
}

CInterpolationComponent *SwiftGuiStd::ensureInterpolationComponent()
{
    return createToolDialog(m_interpolationDialog, m_interpolationComponent, this, tr("Interpolation"),
                            QSize(760, 520));
}

CRadarComponent *SwiftGuiStd::ensureRadarComponent()
{
    return createToolDialog(m_radarDialog, m_radarComponent, this, tr("Radar"), QSize(760, 520));
}

CLogComponent *SwiftGuiStd::ensureLogComponent()
{
    const bool created = !m_logDialog;
    CLogComponent *component = createToolDialog(m_logDialog, m_logComponent, this, tr("Log"), QSize(760, 520));
    m_mwaLogComponent = component;
    if (created) { component->showFilterDialog(); }
    return component;
}

CSettingsComponent *SwiftGuiStd::ensureSettingsComponent()
{
    const bool created = !m_settingsDialog;
    CSettingsComponent *component =
        createToolDialog(m_settingsDialog, m_settingsComponent, this, tr("Settings"), QSize(850, 400));
    if (!created) { return component; }
    connect(component, &CSettingsComponent::changedWindowsOpacity, this, &SwiftGuiStd::onChangedWindowOpacity,
            Qt::UniqueConnection);
    return component;
}

CInternalsComponent *SwiftGuiStd::ensureInternalsComponent()
{
    return createToolDialog(m_internalsDialog, m_internalsComponent, this, tr("Internals"), QSize(760, 520));
}

void SwiftGuiStd::onPttChanged(bool enabled)
{
    m_pttActive = enabled;
    this->updateStatusInfoTooltip();
    if (!sGui || !sGui->getCContextAudioBase()) { return; }

    // based on user request still play with AFV
    sGui->getCContextAudioBase()->playNotification(
        enabled ? CNotificationSounds::PTTClickKeyDown : CNotificationSounds::PTTClickKeyUp, true);
}

void SwiftGuiStd::displayDBusReconnectDialog()
{
    if (m_displayingDBusReconnect) { return; }
    if (!sGui || sGui->isShuttingDown()) { return; }
    if (!sGui->getCoreFacade()) { return; }
    if (!sGui->getCoreFacadeConfig().requiresDBusConnection()) { return; }
    m_displayingDBusReconnect = true;
    const QString dBusAddress = sGui->getCoreFacade()->getDBusAddress();
    const QString informativeText = tr("Do you want to try to reconnect? 'Abort' will close the GUI.\n\nDBus: '%1'");
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(tr("neoswift core not reachable!"));
    msgBox.setInformativeText(informativeText.arg(dBusAddress));
    msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Abort);
    msgBox.setDefaultButton(QMessageBox::Retry);
    const int ret = msgBox.exec();
    if (ret == QMessageBox::Abort)
    {
        m_coreFailures = 0;
        sGui->gracefulShutdown();
        CGuiApplication::exit(EXIT_FAILURE);
        return;
    }

    m_displayingDBusReconnect = false;
    CStatusMessage msg = sGui->getCoreFacade()->tryToReconnectWithDBus();
    if (msg.isSuccess()) { m_coreFailures = 0; }
    msg.clampSeverity(CStatusMessage::SeverityWarning);
    CLogMessage::preformatted(msg);
}

void SwiftGuiStd::onShowOverlayVariant(const CVariant &variant, std::chrono::milliseconds duration)
{
    if (!sGui || sGui->isShuttingDown()) { return; }
    ui->fr_CentralFrameInside->showOverlayVariant(variant, duration);
}

void SwiftGuiStd::onShowOverlayInlineTextMessageTab(components::TextMessageTab tab)
{
    if (!sGui || sGui->isShuttingDown()) { return; }
    ui->comp_TextMessages->setTab(tab);
    ui->comp_TextMessages->focusTextEntry();
}

void SwiftGuiStd::onShowOverlayInlineTextMessageCallsign(const CCallsign &callsign)
{
    if (!sGui || sGui->isShuttingDown()) { return; }
    ui->comp_TextMessages->showCorrespondingTab(callsign);
    ui->comp_TextMessages->focusTextEntry();
}

bool SwiftGuiStd::startModelBrowser()
{
    if (!m_modelBrower) { m_modelBrower.reset(new CModelBrowserDialog(this)); }
    m_modelBrower->setModal(false);
    m_modelBrower->show();
    m_modelBrower->raise();
    m_modelBrower->activateWindow();
    return true;
}

bool SwiftGuiStd::startAFVMap()
{
    if (sGui && !sGui->isShuttingDown()) { sGui->openUrl(sGui->getGlobalSetup().getAfvMapUrl()); }

    return true;
}
