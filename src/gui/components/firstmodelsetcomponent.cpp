// SPDX-FileCopyrightText: Copyright (C) 2018 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "gui/components/firstmodelsetcomponent.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWizard>
#include <QStringList>

#include "ui_firstmodelsetcomponent.h"

#include "config/buildconfig.h"
#include "core/modelsetbuilder.h"
#include "core/webdataservices.h"
#include "gui/components/dbdistributorcomponent.h"
#include "gui/components/dbownmodelscomponent.h"
#include "gui/components/dbownmodelsdialog.h"
#include "gui/components/dbownmodelsetcomponent.h"
#include "gui/components/dbownmodelsetdialog.h"
#include "gui/guiapplication.h"
#include "gui/models/aircraftmodellistmodel.h"
#include "gui/views/aircraftmodelview.h"
#include "gui/views/distributorview.h"
#include "misc/simulation/data/modelcaches.h"
#include "misc/directoryutils.h"
#include "misc/verify.h"

using namespace swift::config;
using namespace swift::core;
using namespace swift::misc;
using namespace swift::misc::simulation;
using namespace swift::misc::simulation::data;
using namespace swift::misc::simulation::settings;
using namespace swift::gui::components;

namespace
{
    QString firstModelSetText(const char *source)
    {
        return QCoreApplication::translate("CFirstModelSetComponent", source);
    }

    class CCreateModelSetWizard;

    class CModelScanWizardPage : public QWizardPage
    {
    public:
        using QWizardPage::QWizardPage;

        void initializePage() override;
        bool validatePage() override;
    };

    class CModelProvidersWizardPage : public QWizardPage
    {
    public:
        using QWizardPage::QWizardPage;

        void initializePage() override;
    };

    class CModelPreviewWizardPage : public QWizardPage
    {
    public:
        using QWizardPage::QWizardPage;

        void initializePage() override;
        bool validatePage() override;
    };

    class CCreateModelSetWizard : public QWizard
    {
    public:
        enum Page
        {
            SelectSimulator,
            ScanModels,
            SelectProviders,
            PreviewModelSet
        };

        explicit CCreateModelSetWizard(QWidget *parent = nullptr) : QWizard(parent)
        {
            this->setWindowTitle(firstModelSetText("New Model Set Wizard"));
            this->setWizardStyle(QWizard::ClassicStyle);

            auto *selectPage = new QWizardPage(this);
            selectPage->setTitle(firstModelSetText("Select Simulator"));
            selectPage->setSubTitle(firstModelSetText("Choose the simulator this model set is for."));
            auto *selectLayout = new QVBoxLayout(selectPage);
            m_simulatorSelector = new CSimulatorSelector(selectPage);
            m_simulatorSelector->setMode(CSimulatorSelector::RadioButtons);
            m_simulatorSelector->setRememberSelectionAndSetToLastSelection();
            selectLayout->addWidget(m_simulatorSelector);
            this->setPage(SelectSimulator, selectPage);

            auto *scanPage = new CModelScanWizardPage(this);
            scanPage->setTitle(firstModelSetText("Scan Installed Models"));
            scanPage->setSubTitle(firstModelSetText(
                "NeoSwift scans the selected simulator and continues automatically when models are found."));
            auto *scanLayout = new QVBoxLayout(scanPage);
            m_scanLabel = new QLabel(firstModelSetText("Ready to scan installed models."), scanPage);
            m_scanLabel->setWordWrap(true);
            scanLayout->addWidget(m_scanLabel);
            m_ownModels = new CDbOwnModelsComponent(scanPage);
            scanLayout->addWidget(m_ownModels);
            this->setPage(ScanModels, scanPage);

            auto *providersPage = new CModelProvidersWizardPage(this);
            providersPage->setTitle(firstModelSetText("Choose Model Providers"));
            providersPage->setSubTitle(
                firstModelSetText("Select the model providers you want to include in this model set."));
            auto *providersLayout = new QVBoxLayout(providersPage);
            m_dbOnly = new QCheckBox(firstModelSetText("Use models with DB data only"), providersPage);
            m_dbOnly->setChecked(true);
            providersLayout->addWidget(m_dbOnly);
            m_distributors = new CDbDistributorComponent(providersPage);
            m_distributors->view()->setSelectionMode(QAbstractItemView::MultiSelection);
            providersLayout->addWidget(m_distributors);
            this->setPage(SelectProviders, providersPage);

            auto *previewPage = new CModelPreviewWizardPage(this);
            previewPage->setTitle(firstModelSetText("Preview Model Set"));
            previewPage->setSubTitle(firstModelSetText("Review the model set before saving it."));
            auto *previewLayout = new QVBoxLayout(previewPage);
            m_previewLabel = new QLabel(previewPage);
            m_previewLabel->setWordWrap(true);
            previewLayout->addWidget(m_previewLabel);
            m_previewView = new swift::gui::views::CAircraftModelView(previewPage);
            m_previewView->setAircraftModelMode(swift::gui::models::CAircraftModelListModel::OwnModelSet);
            previewLayout->addWidget(m_previewView);
            this->setPage(PreviewModelSet, previewPage);

            connect(m_ownModels, &CDbOwnModelsComponent::successfullyLoadedModels, this,
                    [this](const CSimulatorInfo &simulator, int count) {
                        if (simulator != this->selectedSimulator()) { return; }
                        m_scanComplete = count > 0;
                        m_scanLabel->setText(
                            count > 0 ?
                                firstModelSetText("Model scan complete. Continuing to model providers...") :
                                firstModelSetText("Model scan completed, but no models were found."));
                        if (m_scanComplete && this->currentId() == ScanModels)
                        {
                            QTimer::singleShot(0, this, [this] {
                                if (this->currentId() == ScanModels) { this->next(); }
                            });
                        }
                    });
        }

        CSimulatorInfo selectedSimulator() const { return m_simulatorSelector->getValue(); }

        void startModelScan()
        {
            const CSimulatorInfo simulator = this->selectedSimulator();
            if (!simulator.isSingleSimulator()) { return; }

            m_scanComplete = false;
            m_scanLabel->setText(firstModelSetText("Scanning installed models..."));
            m_ownModels->setSimulator(simulator, true);
            m_ownModels->requestModelsInBackground(simulator, false);
        }

        bool hasScannedModels() const
        {
            const CSimulatorInfo simulator = this->selectedSimulator();
            return simulator.isSingleSimulator() && m_ownModels->getOwnCachedModels(simulator).size() > 0;
        }

        void prepareProvidersPage()
        {
            const CSimulatorInfo simulator = this->selectedSimulator();
            if (simulator.isSingleSimulator()) { m_distributors->filterBySimulator(simulator); }
        }

        void buildPreviewModelSet()
        {
            const CSimulatorInfo simulator = this->selectedSimulator();
            if (!simulator.isSingleSimulator())
            {
                m_previewModels.clear();
                return;
            }

            CModelSetBuilder::Builder options = CModelSetBuilder::SortByDistributors;
            const CDistributorList distributors = m_distributors->getSelectedDistributors();
            if (!distributors.isEmpty()) { options |= CModelSetBuilder::GivenDistributorsOnly; }
            if (m_dbOnly->isChecked()) { options |= CModelSetBuilder::OnlyDbData; }

            const CAircraftModelList ownModels = m_ownModels->getOwnCachedModels(simulator);
            m_previewModels = m_builder.buildModelSet(simulator, ownModels, {}, options, distributors);
            m_previewView->setCorrespondingSimulator(simulator, {});
            m_previewView->updateContainerMaybeAsync(m_previewModels);

            m_previewLabel->setText(
                firstModelSetText("This will save %1 models for %2.")
                    .arg(m_previewModels.sizeInt())
                    .arg(simulator.toQString(true)));
        }

        bool saveModelSet()
        {
            const CSimulatorInfo simulator = this->selectedSimulator();
            if (!simulator.isSingleSimulator() || m_previewModels.isEmpty())
            {
                QMessageBox::warning(this, firstModelSetText("Model Set"),
                                     firstModelSetText(
                                         "The preview is empty. Choose another simulator or model provider."));
                return false;
            }

            auto &modelSets = CCentralMultiSimulatorModelSetCachesProvider::modelCachesInstance();
            modelSets.synchronizeCache(simulator);
            if (modelSets.getCachedModelsCount(simulator) > 0)
            {
                const QMessageBox::StandardButton reply =
                    QMessageBox::question(this, firstModelSetText("Model Set"),
                                          firstModelSetText("Replace the existing model set for %1?")
                                              .arg(simulator.toQString(true)),
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (reply != QMessageBox::Yes) { return false; }
            }

            const CStatusMessage status = modelSets.setCachedModels(m_previewModels, simulator);
            if (status.isFailure())
            {
                QMessageBox::warning(this, firstModelSetText("Model Set"), status.getMessage());
                return false;
            }
            return true;
        }

    private:
        CSimulatorSelector *m_simulatorSelector = nullptr;
        CDbOwnModelsComponent *m_ownModels = nullptr;
        QLabel *m_scanLabel = nullptr;
        CDbDistributorComponent *m_distributors = nullptr;
        QCheckBox *m_dbOnly = nullptr;
        QLabel *m_previewLabel = nullptr;
        swift::gui::views::CAircraftModelView *m_previewView = nullptr;
        CModelSetBuilder m_builder { this };
        CAircraftModelList m_previewModels;
        bool m_scanComplete = false;
    };

    void CModelScanWizardPage::initializePage()
    {
        static_cast<CCreateModelSetWizard *>(this->wizard())->startModelScan();
    }

    bool CModelScanWizardPage::validatePage()
    {
        auto *w = static_cast<CCreateModelSetWizard *>(this->wizard());
        if (w->hasScannedModels()) { return true; }
        QMessageBox::warning(w, firstModelSetText("Model Scan"),
                             firstModelSetText(
                                 "No models were found yet. Wait for the scan to finish or check your simulator model paths."));
        return false;
    }

    void CModelProvidersWizardPage::initializePage()
    {
        static_cast<CCreateModelSetWizard *>(this->wizard())->prepareProvidersPage();
    }

    void CModelPreviewWizardPage::initializePage()
    {
        static_cast<CCreateModelSetWizard *>(this->wizard())->buildPreviewModelSet();
    }

    bool CModelPreviewWizardPage::validatePage()
    {
        return static_cast<CCreateModelSetWizard *>(this->wizard())->saveModelSet();
    }
} // namespace

namespace swift::gui::components
{
    const QStringList &CFirstModelSetComponent::getLogCategories()
    {
        static const QStringList cats { CLogCategories::modelGui() };
        return cats;
    }

    CFirstModelSetComponent::CFirstModelSetComponent(QWidget *parent)
        : COverlayMessagesFrame(parent), ui(new Ui::CFirstModelSetComponent)
    {
        ui->setupUi(this);
        ui->fr_Info->hide();
        ui->gl_FirstModelSet->hide();

        auto *overviewLabel = new QLabel(
            firstModelSetText(
                "Create one model set for each simulator you use. The matching engine uses the saved model set for the selected simulator."),
            this);
        overviewLabel->setWordWrap(true);

        m_modelSetTable = new QTableWidget(0, 4, this);
        m_modelSetTable->setHorizontalHeaderLabels(
            { firstModelSetText("Simulator"), firstModelSetText("Models"), firstModelSetText("Updated"),
              firstModelSetText("File") });
        m_modelSetTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_modelSetTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_modelSetTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_modelSetTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
        m_modelSetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_modelSetTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_modelSetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_modelSetTable->verticalHeader()->setVisible(false);

        m_pbAddModelSet = new QPushButton(firstModelSetText("Add"), this);
        m_pbDeleteModelSet = new QPushButton(firstModelSetText("Delete"), this);
        m_pbDeleteModelSet->setEnabled(false);

        auto *buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_pbAddModelSet);
        buttonLayout->addWidget(m_pbDeleteModelSet);

        ui->vl_FirstModelSetComponent->insertWidget(0, overviewLabel);
        ui->vl_FirstModelSetComponent->insertWidget(1, m_modelSetTable);
        ui->vl_FirstModelSetComponent->insertLayout(2, buttonLayout);

        connect(m_pbAddModelSet, &QPushButton::clicked, this, &CFirstModelSetComponent::addModelSet);
        connect(m_pbDeleteModelSet, &QPushButton::clicked, this, &CFirstModelSetComponent::deleteSelectedModelSet);
        connect(m_modelSetTable, &QTableWidget::itemSelectionChanged, this, [this] {
            m_pbDeleteModelSet->setEnabled(m_modelSetTable->currentRow() >= 0);
        });

        ui->comp_Distributors->view()->setSelectionMode(QAbstractItemView::MultiSelection);
        ui->comp_SimulatorSelector->setMode(CSimulatorSelector::RadioButtons);
        ui->comp_SimulatorSelector->setRememberSelectionAndSetToLastSelection();

        // we use the powerful component to access own models
        m_modelsDialog.reset(new CDbOwnModelsDialog(this));
        m_modelSetDialog.reset(new CDbOwnModelSetDialog(this));

        this->onSimulatorChanged(ui->comp_SimulatorSelector->getValue());

        bool s = connect(ui->comp_SimulatorSelector, &CSimulatorSelector::changed, this,
                         &CFirstModelSetComponent::onSimulatorChanged);
        Q_ASSERT_X(s, Q_FUNC_INFO, "Cannot connect selector signal");
        connect(&m_simulatorSettings, &CMultiSimulatorSettings::settingsChanged, this,
                &CFirstModelSetComponent::onSettingsChanged, Qt::QueuedConnection);
        Q_ASSERT_X(s, Q_FUNC_INFO, "Cannot connect settings signal");
        connect(m_modelsDialog.data(), &CDbOwnModelsDialog::successfullyLoadedModels, this,
                &CFirstModelSetComponent::onModelsLoaded, Qt::QueuedConnection);
        Q_ASSERT_X(s, Q_FUNC_INFO, "Cannot connect models signal");

        connect(ui->pb_ModelSet, &QPushButton::clicked, this, &CFirstModelSetComponent::openOwnModelSetDialog);
        connect(ui->pb_Models, &QPushButton::clicked, this, &CFirstModelSetComponent::openOwnModelsDialog);
        connect(ui->pb_ModelsTriggerReload, &QPushButton::clicked, this, &CFirstModelSetComponent::openOwnModelsDialog);
        connect(ui->pb_ChangeModelDir, &QPushButton::clicked, this, &CFirstModelSetComponent::changeModelDirectory);
        connect(ui->pb_ClearModelDir, &QPushButton::clicked, this, &CFirstModelSetComponent::changeModelDirectory);
        connect(ui->pb_CreateModelSet, &QPushButton::clicked, this, &CFirstModelSetComponent::createModelSet);

        this->reloadModelSetTable();
    }

    CFirstModelSetComponent::~CFirstModelSetComponent() = default;

    void CFirstModelSetComponent::reloadModelSetTable()
    {
        if (!m_modelSetTable) { return; }

        auto &modelSets = CCentralMultiSimulatorModelSetCachesProvider::modelCachesInstance();
        m_modelSetTable->setRowCount(0);

        for (const CSimulatorInfo &simulator : CSimulatorInfo::allSimulators().asSingleSimulatorSet())
        {
            modelSets.synchronizeCache(simulator);
            const int count = modelSets.getCachedModelsCount(simulator);
            if (count < 1) { continue; }

            const int row = m_modelSetTable->rowCount();
            m_modelSetTable->insertRow(row);

            auto *simulatorItem = new QTableWidgetItem(simulator.toQString(true));
            simulatorItem->setData(Qt::UserRole, static_cast<int>(simulator.getSimulator()));
            m_modelSetTable->setItem(row, 0, simulatorItem);
            m_modelSetTable->setItem(row, 1, new QTableWidgetItem(QString::number(count)));

            const QDateTime timestamp = modelSets.getCacheTimestamp(simulator);
            m_modelSetTable->setItem(row, 2,
                                     new QTableWidgetItem(timestamp.isValid() ?
                                                              timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm")) :
                                                              QString()));
            m_modelSetTable->setItem(row, 3, new QTableWidgetItem(modelSets.getFilename(simulator)));
        }

        m_pbDeleteModelSet->setEnabled(m_modelSetTable->currentRow() >= 0);
    }

    void CFirstModelSetComponent::addModelSet()
    {
        CCreateModelSetWizard wizard(this->mainWindow());
        const auto result = static_cast<QDialog::DialogCode>(wizard.exec());
        if (result == QDialog::Accepted) { this->reloadModelSetTable(); }
    }

    void CFirstModelSetComponent::deleteSelectedModelSet()
    {
        const CSimulatorInfo simulator = this->selectedModelSetSimulator();
        if (!simulator.isSingleSimulator()) { return; }

        const QMessageBox::StandardButton reply =
            QMessageBox::question(this->mainWindow(), firstModelSetText("Delete Model Set"),
                                  firstModelSetText("Delete the model set for %1?").arg(simulator.toQString(true)),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) { return; }

        CCentralMultiSimulatorModelSetCachesProvider::modelCachesInstance().clearCachedModels(simulator);
        this->reloadModelSetTable();
    }

    CSimulatorInfo CFirstModelSetComponent::selectedModelSetSimulator() const
    {
        if (!m_modelSetTable) { return {}; }
        const int row = m_modelSetTable->currentRow();
        if (row < 0) { return {}; }
        const QTableWidgetItem *item = m_modelSetTable->item(row, 0);
        if (!item) { return {}; }
        const int simulator = item->data(Qt::UserRole).toInt();
        return CSimulatorInfo(CSimulatorInfo::Simulator(static_cast<CSimulatorInfo::SimulatorFlag>(simulator)));
    }

    void CFirstModelSetComponent::onSimulatorChanged(const CSimulatorInfo &simulator)
    {
        if (!simulator.isSingleSimulator())
        {
            //! \fixme KB 2019-01 reported by RR, sometimes happening and leading to ASSERT/CTD, avoiding the
            //! "crash" for better infos
            if (CBuildConfig::isLocalDeveloperDebugBuild())
            {
                SWIFT_VERIFY_X(false, Q_FUNC_INFO, "Need single simulator");
            }
            CLogMessage(this).error(u"Changing to non-single simulator %1 ignored") << simulator.toQString();
            return;
        }

        Q_ASSERT_X(m_modelsDialog, Q_FUNC_INFO, "No models dialog");
        m_modelsDialog->setSimulator(simulator);

        Q_ASSERT_X(m_modelSetDialog, Q_FUNC_INFO, "No model set dialog");
        m_modelSetDialog->setSimulator(simulator);

        // distributor component
        ui->comp_Distributors->filterBySimulator(simulator);

        const QStringList dirs = m_simulatorSettings.getModelDirectoriesOrDefault(simulator);
        ui->le_ModelDirectories->setText(dirs.join(", "));

        // kind of hack, but simplest solution
        // we us the loader of the components directly,
        // avoid to fully init a loader logic here
        static const QString modelsNo("No models so far");
        const int modelsCount = this->modelLoader()->getCachedModelsCount(simulator);
        if (modelsCount > 0)
        {
            static const QString modelsInfo("%1 included %2 DB key %3");
            const CAircraftModelList modelsInCache = this->modelLoader()->getCachedModels(simulator);
            const int modelsIncluded = modelsInCache.countByMode(CAircraftModel::Include);
            const int modelsDbKey = modelsInCache.countWithValidDbKey(true);
            ui->le_ModelsInfo->setText(modelsInfo.arg(this->modelLoader()->getCacheCountAndTimestamp(simulator))
                                           .arg(modelsIncluded)
                                           .arg(modelsDbKey));
        }
        else { ui->le_ModelsInfo->setText(modelsNo); }

        ui->pb_CreateModelSet->setEnabled(modelsCount > 0);

        static const QString modelsSetNo("Model set is empty");
        const int modelsSetCount = m_modelSetDialog->modelSetComponent()->getModelSetCount();
        ui->le_ModelSetInfo->setText(
            modelsSetCount > 0 ? m_modelSetDialog->modelSetComponent()->getModelCacheCountAndTimestamp() : modelsSetNo);
    }

    void CFirstModelSetComponent::onSettingsChanged(const CSimulatorInfo &simulator)
    {
        const CSimulatorInfo currentSimulator = ui->comp_SimulatorSelector->getValue();
        if (simulator != currentSimulator) { return; } // ignore changes not for my selected simulator
        this->onSimulatorChanged(simulator);
    }

    void CFirstModelSetComponent::onModelsLoaded(const CSimulatorInfo &simulator, int count)
    {
        Q_UNUSED(count);
        const CSimulatorInfo currentSimulator = ui->comp_SimulatorSelector->getValue();
        if (simulator != currentSimulator) { return; } // ignore changes not for my selected simulator
        this->onSimulatorChanged(simulator);
    }

    void CFirstModelSetComponent::triggerSettingsChanged(const CSimulatorInfo &simulator)
    {
        if (!sGui || sGui->isShuttingDown()) { return; }
        QPointer<CFirstModelSetComponent> myself(this);
        QTimer::singleShot(0, this, [=] {
            if (!myself || !sGui || sGui->isShuttingDown()) { return; }
            myself->onSettingsChanged(simulator);
        });
    }

    const CDbOwnModelsComponent *CFirstModelSetComponent::modelsComponent() const
    {
        Q_ASSERT_X(m_modelsDialog, Q_FUNC_INFO, "No models dialog");
        Q_ASSERT_X(m_modelsDialog->modelsComponent(), Q_FUNC_INFO, "No models component");
        return m_modelsDialog->modelsComponent();
    }

    const CDbOwnModelSetComponent *CFirstModelSetComponent::modelSetComponent() const
    {
        Q_ASSERT_X(m_modelSetDialog, Q_FUNC_INFO, "No model set dialog");
        Q_ASSERT_X(m_modelSetDialog->modelSetComponent(), Q_FUNC_INFO, "No model set component");
        return m_modelSetDialog->modelSetComponent();
    }

    IAircraftModelLoader *CFirstModelSetComponent::modelLoader() const
    {
        Q_ASSERT_X(m_modelsDialog->modelsComponent()->modelLoader(), Q_FUNC_INFO, "No model loader");
        return m_modelsDialog->modelsComponent()->modelLoader();
    }

    void CFirstModelSetComponent::openOwnModelsDialog()
    {
        if (!m_modelsDialog) { return; }
        if (!sGui || sGui->isShuttingDown() || !sGui->getWebDataServices()) { return; }
        const bool reload = (QObject::sender() == ui->pb_ModelsTriggerReload);

        const CSimulatorInfo simulator = ui->comp_SimulatorSelector->getValue();
        m_modelsDialog->setSimulator(simulator);

        if (reload)
        {
            if (!sGui->getWebDataServices()->hasDbModelData())
            {
                const QMessageBox::StandardButton reply = QMessageBox::warning(
                    this->mainWindow(), tr("DB data"),
                    tr("No DB data, models cannot be consolidated. Load anyway?"),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply != QMessageBox::Yes) { return; }
            }

            bool loadOnlyIfNotEmpty = true;
            if (m_modelsDialog->getOwnModelsCount() > 0)
            {
                const QMessageBox::StandardButton reply =
                    QMessageBox::warning(this->mainWindow(), tr("Model loading"),
                                         tr("Reload the models?\nThe existing cache data will we overridden."),
                                         QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) { loadOnlyIfNotEmpty = false; }
            }
            m_modelsDialog->requestModelsInBackground(simulator, loadOnlyIfNotEmpty);
        }
        m_modelsDialog->exec();

        // force UI update
        this->triggerSettingsChanged(simulator);
    }

    void CFirstModelSetComponent::openOwnModelSetDialog()
    {
        const CSimulatorInfo simulator = ui->comp_SimulatorSelector->getValue();
        m_modelSetDialog->setSimulator(simulator);
        m_modelSetDialog->enableButtons(false, false);
        m_modelSetDialog->exec();

        // force UI update
        this->triggerSettingsChanged(simulator);
    }

    void CFirstModelSetComponent::changeModelDirectory()
    {
        using namespace std::chrono_literals;

        if (!sGui || sGui->isShuttingDown()) { return; }
        const CSimulatorInfo simulator = ui->comp_SimulatorSelector->getValue();
        CSpecializedSimulatorSettings settings = m_simulatorSettings.getSpecializedSettings(simulator);
        const bool clear = (QObject::sender() == ui->pb_ClearModelDir);

        if (clear) { settings.clearModelDirectories(); }
        else
        {
            const QString dirOld = settings.getFirstModelDirectoryOrDefault();
            const QString newDir =
                QFileDialog::getExistingDirectory(this->mainWindow(), tr("Open model directory"), dirOld,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (newDir.isEmpty() || CDirectoryUtils::isSameExistingDirectory(dirOld, newDir)) { return; }
            settings.addModelDirectory(newDir);
        }

        const CStatusMessage msg = m_simulatorSettings.setAndSaveSettings(settings, simulator);
        if (msg.isSuccess()) { this->triggerSettingsChanged(simulator); }
        else { this->showOverlayMessage(msg, 4s); }
    }

    void CFirstModelSetComponent::createModelSet()
    {
        using namespace std::chrono_literals;
        const CSimulatorInfo simulator = ui->comp_SimulatorSelector->getValue();
        const int modelsCount = this->modelLoader()->getCachedModelsCount(simulator);
        if (modelsCount < 1)
        {
            static const CStatusMessage msg =
                CStatusMessage(this).validationError(u"No models indexed so far. Try 'reload'!");
            this->showOverlayMessage(msg, 4s);
            return;
        }

        bool useAllModels = false;
        if (!ui->comp_Distributors->hasSelectedDistributors())
        {
            const QMessageBox::StandardButton reply =
                QMessageBox::question(this->mainWindow(), tr("Models"),
                                      tr("No distributors selected, use all models?"),
                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) { useAllModels = true; }
            else
            {
                static const CStatusMessage msg = CStatusMessage(this).validationError(u"No distributors selected");
                this->showOverlayMessage(msg, 4s);
                return;
            }
        }
        CAircraftModelList modelsForSet = this->modelLoader()->getCachedModels(simulator);
        if (!useAllModels)
        {
            const CDistributorList distributors = ui->comp_Distributors->getSelectedDistributors();
            modelsForSet = modelsForSet.findByDistributors(distributors);
        }

        if (ui->cb_DbDataOnly->isChecked()) { modelsForSet.removeObjectsWithoutDbKey(); }
        if (modelsForSet.isEmpty())
        {
            this->showOverlayHTMLMessage(tr("Selection yielded no result!"));
            return;
        }

        // just in case, paranoia
        if (!m_modelSetDialog || !m_modelSetDialog->modelSetComponent())
        {
            this->showOverlayHTMLMessage("No model set dialog, cannot continue");
            return;
        }

        const int modelsSetCount = m_modelSetDialog->modelSetComponent()->getModelSetCount();
        if (modelsSetCount > 0)
        {
            QMessageBox::StandardButton override = QMessageBox::question(
                this, "Override", "Override existing model set?", QMessageBox::Yes | QMessageBox::No);
            if (override != QMessageBox::Yes) { return; }
        }

        m_modelSetDialog->modelSetComponent()->setModelSet(modelsForSet, simulator);
        ui->pb_ModelSet->click();
    }

    QWidget *CFirstModelSetComponent::mainWindow()
    {
        QWidget *pw = CGuiApplication::mainApplicationWidget();
        return pw ? pw : this;
    }

    bool CFirstModelSetWizardPage::validatePage() { return true; }
} // namespace swift::gui::components
