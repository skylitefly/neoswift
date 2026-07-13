// SPDX-FileCopyrightText: Copyright (C) 2013 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

//! \file

#ifndef STDGUI_SWIFTGUISTD_H
#define STDGUI_SWIFTGUISTD_H

#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QString>

#include "core/actionbind.h"
#include "gui/components/aircraftmodelsetvalidationdialog.h"
#include "gui/components/navigatordialog.h"
#include "gui/components/textmessagecomponenttab.h"
#include "gui/enableforframelesswindow.h"
#include "gui/guiactionbind.h"
#include "gui/mainwindowaccess.h"
#include "gui/managedstatusbar.h"
#include "gui/settings/guisettings.h"
#include "misc/audio/audiosettings.h"
#include "misc/audio/notificationsounds.h"
#include "misc/icons.h"
#include "misc/identifiable.h"
#include "misc/input/actionhotkeydefs.h"
#include "misc/loghandler.h"
#include "misc/loghistory.h"
#include "misc/network/connectionstatus.h"
#include "misc/simulation/simulatedaircraft.h"
#include "misc/statusmessage.h"
#include "misc/variant.h"

class QAction;
class QCloseEvent;
class QEvent;
class QMouseEvent;
class QTimer;
class QDialog;

namespace swift::misc::aviation
{
    class CAltitude;
    class CCallsign;
}
namespace swift::gui::components
{
    class CAircraftComponent;
    class CDbLoadDataDialog;
    class CAtcStationComponent;
    class CCockpitComponent;
    class CFlightPlanComponent;
    class CInternalsComponent;
    class CInterpolationComponent;
    class CLogComponent;
    class CLoginComponent;
    class CMappingComponent;
    class CModelBrowserDialog;
    class CRadarComponent;
    class CSettingsComponent;
    class CSimulatorComponent;
    class CTextMessageComponent;
    class CUserComponent;
} // namespace swift::gui::components
namespace Ui
{
    class SwiftGuiStd;
}

//! swift GUI
class SwiftGuiStd :
    public QMainWindow,
    public swift::misc::CIdentifiable,
    public swift::gui::CEnableForFramelessWindow,
    public swift::gui::IMainWindowAccess
{
    Q_OBJECT
    Q_INTERFACES(swift::gui::IMainWindowAccess)

public:
    //! Constructor
    SwiftGuiStd(WindowMode windowMode, QWidget *parent = nullptr);

    //! Destructor
    ~SwiftGuiStd() override;

protected:
    //! \name QMainWindow events
    //! @{

    //! \copydoc QMainWindow::mouseMoveEvent
    void mouseMoveEvent(QMouseEvent *event) override;

    //! \copydoc QMainWindow::mousePressEvent
    void mousePressEvent(QMouseEvent *event) override;

    //! \copydoc QMainWindow::mouseReleaseEvent
    void mouseReleaseEvent(QMouseEvent *event) override;

    //! \copydoc QMainWindow::closeEvent
    void closeEvent(QCloseEvent *event) override;

    //! \copydoc QMainWindow::changeEvent
    void changeEvent(QEvent *event) override;
    //! @}

    //! Get a minimize action which minimizes the window

    //! @{
    //! Actions for navigator
    QAction *getWindowMinimizeAction(QObject *parent);
    QAction *getWindowNormalAction(QObject *parent);
    QAction *getToggleWindowVisibilityAction(QObject *parent);
    QAction *getToggleStayOnTopAction(QObject *parent);
    //! @}

private:
    QScopedPointer<Ui::SwiftGuiStd> ui;
    QScopedPointer<swift::gui::components::CNavigatorDialog> m_navigator {
        new swift::gui::components::CNavigatorDialog()
    }; //!< navigator dialog bar, if I pass the parent, the dialog is always centered over the parent
    QScopedPointer<swift::gui::components::CDbLoadDataDialog> m_dbLoadDialog; //!< load DB data, lazy init UI component
    QScopedPointer<swift::gui::components::CModelBrowserDialog> m_modelBrower; //!< model browser
    QScopedPointer<swift::gui::components::CAircraftModelSetValidationDialog>
        m_validationDialog; //!< aircraft model validation dialog
    QScopedPointer<QDialog> m_loginDialog;
    QScopedPointer<QDialog> m_atcDetailsDialog;
    QScopedPointer<QDialog> m_cockpitDialog;
    QScopedPointer<QDialog> m_aircraftDialog;
    QScopedPointer<QDialog> m_usersDialog;
    QScopedPointer<QDialog> m_simulatorDialog;
    QScopedPointer<QDialog> m_flightPlanDialog;
    QScopedPointer<QDialog> m_mappingDialog;
    QScopedPointer<QDialog> m_interpolationDialog;
    QScopedPointer<QDialog> m_radarDialog;
    QScopedPointer<QDialog> m_logDialog;
    QScopedPointer<QDialog> m_settingsDialog;
    QScopedPointer<QDialog> m_internalsDialog;
    QPointer<swift::gui::components::CLoginComponent> m_loginComponent;
    QPointer<swift::gui::components::CAtcStationComponent> m_atcDetailsComponent;
    QPointer<swift::gui::components::CCockpitComponent> m_cockpitComponent;
    QPointer<swift::gui::components::CAircraftComponent> m_aircraftComponent;
    QPointer<swift::gui::components::CUserComponent> m_userComponent;
    QPointer<swift::gui::components::CSimulatorComponent> m_simulatorComponent;
    QPointer<swift::gui::components::CFlightPlanComponent> m_flightPlanComponent;
    QPointer<swift::gui::components::CMappingComponent> m_mappingComponent;
    QPointer<swift::gui::components::CInterpolationComponent> m_interpolationComponent;
    QPointer<swift::gui::components::CRadarComponent> m_radarComponent;
    QPointer<swift::gui::components::CLogComponent> m_logComponent;
    QPointer<swift::gui::components::CSettingsComponent> m_settingsComponent;
    QPointer<swift::gui::components::CInternalsComponent> m_internalsComponent;
    swift::core::CActionBind m_actionPtt { swift::misc::input::pttHotkeyAction(),
                                           swift::misc::CIcons::StandardIconRadio16, this, &SwiftGuiStd::onPttChanged };
    swift::core::CActionBindings m_menuHotkeyHandlers;
    swift::gui::CManagedStatusBar m_statusBar;
    swift::misc::CLogHistoryReplica m_logHistoryForLogButtons { this };
    swift::misc::CLogHistoryReplica m_logHistoryForOverlay { this };
    swift::misc::CSetting<swift::misc::audio::TSettings> m_audioSettings { this };
    swift::misc::CSetting<swift::gui::settings::TLoadDbDataAtStartup> m_loadDbDataAtStartup { this };

    // contexts
    static constexpr int MaxCoreFailures = 5; //!< Failures counted before reconnecting
    int m_coreFailures = 0; //!< failed access to core
    bool m_init = false;
    bool m_coreAvailable = false; //!< core already available?
    bool m_contextNetworkAvailable = false; //!< network context available?
    bool m_contextAudioAvailable = false; //!< audio context available?
    bool m_pttActive = false; //!< PTT hotkey currently active
    bool m_displayingDBusReconnect = false; //!< currently displaying reconnect dialog
    bool m_dbDataLoading = false; //!< DB or shared data loading in progress
    QTimer m_timerContextWatchdog; //!< core available?
    swift::misc::simulation::CSimulatedAircraft m_ownAircraft; //!< own aircraft's state

    //! GUI status update
    void updateGuiStatusInformation();

    //! Compact tooltip with hidden diagnostic status details.
    QString buildStatusInfoTooltip() const;

    //! Refresh compact status info affordance.
    void updateStatusInfoTooltip();

    //! Set style sheet
    void initStyleSheet();

    //! 1st data reads
    void initialContextDataReads();

    //! Init data (post GUI init)
    void init();

    //! Init GUI signals
    void initGuiSignals();

    //! Init dynamic menus
    void initMenus();

    //! Graceful shutdown
    void performGracefulShutdown();

    //! Audio device lists
    void setAudioDeviceLists();

    //! Context and DBus availability, used by watchdog
    void setContextAvailability();

    //! Position of own plane for testing
    //! \param wgsLatitude  WGS latitude
    //! \param wgsLongitude WGS longitude
    //! \param altitude
    void setTestPosition(const QString &wgsLatitude, const QString &wgsLongitude,
                         const swift::misc::aviation::CAltitude &altitude,
                         const swift::misc::aviation::CAltitude &pressureAltitude);

    //! Stop all timers
    //! \param disconnectSignalSlots also disconnect signal/slots
    void stopAllTimers(bool disconnectSignalSlots);

    //! Play notifcation sound
    void playNotifcationSound(swift::misc::audio::CNotificationSounds::NotificationFlag notification) const;

    //! Display log
    void displayLog();

    //! Display network settings
    void displayNetworkSettings();

    //! Display a reconnect dialog
    void displayDBusReconnectDialog();

    //! PTT changed
    void onPttChanged(bool enabled);

    //
    // Data receiving related funtions
    //

    //! Reload own aircraft
    bool reloadOwnAircraft();

    //! Connection status changed
    //! \param from old status
    //! \param to   new status
    void onConnectionStatusChanged(const swift::misc::network::CConnectionStatus &from,
                                   const swift::misc::network::CConnectionStatus &to);

    //
    // GUI related functions
    //

    //! Display the settings page
    void setSettingsPage(int settingsTabIndex = -1);

    //! Login requested
    void loginRequested();

    //! Show login/logoff component without applying the main Connect button shortcut behavior.
    void showLoginWindow();

    //! Menu item clicked
    void onMenuClicked();

    //! Tool windows
    void showAtcDetailsWindow();
    void showCockpitWindow();
    void showAircraftWindow();
    void showUsersWindow();
    void showSimulatorWindow();
    void showFlightPlanWindow();
    void showMappingWindow();
    void showInterpolationWindow();
    void showRadarWindow();
    void showLogWindow();
    void showSettingsWindow();
    void showInternalsWindow();
    void showToolDialog(QDialog *dialog);

    //! Lazy component accessors
    swift::gui::components::CLoginComponent *ensureLoginComponent();
    swift::gui::components::CAtcStationComponent *ensureAtcDetailsComponent();
    swift::gui::components::CCockpitComponent *ensureCockpitComponent();
    swift::gui::components::CAircraftComponent *ensureAircraftComponent();
    swift::gui::components::CUserComponent *ensureUserComponent();
    swift::gui::components::CSimulatorComponent *ensureSimulatorComponent();
    swift::gui::components::CFlightPlanComponent *ensureFlightPlanComponent();
    swift::gui::components::CMappingComponent *ensureMappingComponent();
    swift::gui::components::CInterpolationComponent *ensureInterpolationComponent();
    swift::gui::components::CRadarComponent *ensureRadarComponent();
    swift::gui::components::CLogComponent *ensureLogComponent();
    swift::gui::components::CSettingsComponent *ensureSettingsComponent();
    swift::gui::components::CInternalsComponent *ensureInternalsComponent();

    //! Kicked from network
    void onKickedFromNetwork(const QString &kickMessage);

    //! Update timer
    void handleTimerBasedUpdates();

    //! Change opacity 0-100
    void onChangedWindowOpacity(int opacity = -1);

    //! Toggle if windows stays on top
    //! \remark mostly used with navigator
    void toggleWindowStayOnTop();

    //! Toggle window visibility
    //! \remark mostly used with navigator
    void toggleWindowVisibility();

    //! Style sheet has been changed
    void onStyleSheetsChanged();

    //! Toggle window on top
    void onToggledWindowsOnTop(bool onTop);

    //! Reported issue with the client
    void onAudioClientFailure(const swift::misc::CStatusMessage &msg);

    //! Focus in main entry window
    void focusInMainEntryField();

    //! Focus in the text message entry field
    void focusInTextMessageEntryField();

    //! Show window minimized
    void showMinimized();

    //! Show window normal
    void showNormal();

    //! Navigator dialog has been closed
    void onNavigatorClosed();

    //! Checks if model set is available
    void verifyPrerequisites();

    //! Model set haas been verfied
    void onValidatedModelSet(const swift::misc::simulation::CSimulatorInfo &simulator,
                             const swift::misc::simulation::CAircraftModelList &valid,
                             const swift::misc::simulation::CAircraftModelList &invalid, bool stopped,
                             const swift::misc::CStatusMessageList &msgs);

    //! Display validation dialog
    void displayValidationDialog();

    //! Ckeck if the DB data have been loaded
    void checkDbDataLoaded();

    //! Start the model browser
    bool startModelBrowser();

    //! Start AFV map
    bool startAFVMap();

    //! @{
    //! Request overlay inline text message
    void onShowOverlayVariant(const swift::misc::CVariant &variant, std::chrono::milliseconds duration);
    void onShowOverlayInlineTextMessageTab(swift::gui::components::TextMessageTab tab);
    void onShowOverlayInlineTextMessageCallsign(const swift::misc::aviation::CCallsign &callsign);
    //! @}
};

#endif // STDGUI_SWIFTGUISTD_H
