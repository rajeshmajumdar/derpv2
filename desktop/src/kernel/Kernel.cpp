#include "Kernel.h"
#include "../interfaces/DBaseModule.h"
#include "LoginDialog.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QKeyEvent>

Kernel::Kernel(QWidget *parent) : QMainWindow(parent) {
  m_uiManager = new UIManager(this);
  m_networkManager = new NetworkManager(this);
  m_serverManager = new ServerManager(this);

  m_uiManager->setupShell();

  QSettings settings("derp", "AppConfig");
  m_appRole = settings.value("role").toString();

  connect(m_networkManager, &NetworkManager::accessDenied, this, &Kernel::handleAccessDenied);

  connect(m_serverManager, &ServerManager::logReceived, this, &Kernel::log);

  connect(m_serverManager, &ServerManager::serverFault, this, [this](QString error) {
    log("[Kernel]PANIC: " + error);
    QMessageBox::critical(this, "Server Error", error);
    qApp->quit();
  });

  if (m_appRole.isEmpty()) {
    SetupWizard wizard(m_serverManager, this);
    if (wizard.exec() != QDialog::Accepted) {
      qApp->quit();
      return;
    }

    m_appRole = settings.value("role").toString();

    this->setWindowState(Qt::WindowMaximized);
    this->show();

    if (m_appRole == "staff") {
      if (m_serverManager->isRunning()) {
        m_serverManager->stopServer();
      }
      initStaffConnection();
    } else {
      m_apiBaseUrl = "http://127.0.0.1:5000";
      m_networkManager->setApiBaseUrl(m_apiBaseUrl);
      startAuthFlow();
    }
  } else if (m_appRole == "master") {
    connect(m_serverManager, &ServerManager::serverReady, this, [this]() {
      this->setWindowState(Qt::WindowMaximized);
      this->show();
      m_apiBaseUrl = "http://127.0.0.1:5000";
      m_networkManager->setApiBaseUrl(m_apiBaseUrl);
      startAuthFlow();
    });
    m_serverManager->initialize("master");
  } else {
    initStaffConnection();
  }
}

Kernel::~Kernel() {}

void Kernel::log(const QString &msg) {
  qDebug() << "[Kernel]:" << msg;
}

QVariant Kernel::callService(const QString &moduleId, const QString &method, const QVariantMap &params) {
  if (!m_pluginManager) return QVariant();
  ModuleRecord* rec = m_pluginManager->getModule(moduleId);
  if (rec) {
    return rec->instance->onServiceRequest(method, params);
  }
  return QVariant();
}

void Kernel::publish(const QString &topic, const QVariantMap &data) {
  if (!m_pluginManager) return;
  const auto& modules = m_pluginManager->allModules();
  for (auto it = modules.begin(); it != modules.end(); ++it) {
    ModuleRecord* record = it.value();
    QJsonArray listeners = record->manifest["listeners"].toArray();
    for (auto val : listeners) {
      if (val.toString() == topic) {
        record->instance->onMessage(topic, data);
      }
    }
  }
}

void Kernel::switchToModule(const QString &moduleId) {
  if (!m_pluginManager) return;
  ModuleRecord* rec = m_pluginManager->getModule(moduleId);
  if (rec && rec->view) {
    m_uiManager->moduleStack()->setCurrentWidget(rec->view);
    m_uiManager->setActiveModule(moduleId);
    m_activeModuleId = moduleId;
    log("Active: " + moduleId);
  }
}

void Kernel::launchIntent(const QString &moduleId, const QString &method, const QVariantMap &data) {
  if (!m_pluginManager) return;
  ModuleRecord* rec = m_pluginManager->getModule(moduleId);
  if (rec) {
    rec->instance->executeIntent(method, data);
  }
}

void Kernel::pushNotification(const NotificationType &type, const QString &msg) {
  log(QString("Notification (%1): %2").arg(static_cast<int>(type)).arg(msg));
}

void Kernel::httpGet(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) {
  m_networkManager->GET(url, data, isAuthenticated, callback);
}
void Kernel::httpPost(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) {
  m_networkManager->POST(url, data, isAuthenticated, callback);
}
void Kernel::httpPut(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) {
  m_networkManager->PUT(url, data, isAuthenticated, callback);
}
void Kernel::httpDownload(const QString& url, const QString& destPath, std::function<void(bool)> callback) {
  m_networkManager->startDownload(url, destPath, callback);
}


void Kernel::initStaffConnection() {
  log("Locating master server on the network...");
  connect(m_networkManager, &NetworkManager::masterFound, this, [this](QString name, QString ip) {
    m_uiManager->setConnectionStatus(true, name);
    m_apiBaseUrl = QString("http://%1:5000").arg(ip);
    m_networkManager->setApiBaseUrl(m_apiBaseUrl);
    log(QString("Found master: %1 [%2]").arg(name, m_apiBaseUrl));

    startAuthFlow();
  });
  m_networkManager->startDiscovery();
}

void Kernel::startAuthFlow() {
  LoginDialog login(m_networkManager, this);
  login.center();

  if (login.exec() == QDialog::Accepted) {
    log("Auth successful");
    initializeSystem();
  } else {
    qApp->quit();
  }
}

void Kernel::initializeSystem() {
  this->setWindowState(Qt::WindowMaximized);

  connect(m_networkManager, &NetworkManager::accessDenied, this, &Kernel::handleAccessDenied);

  // Update connection status — by this point auth has succeeded
  if (m_appRole == "master") {
    m_uiManager->setConnectionStatus(true, "localhost");
  }
  // Staff connection status is already set in initStaffConnection()

  m_pluginManager = new PluginManager(this, this);
  m_pluginManager->loadModules(QApplication::applicationDirPath() + "/plugins", m_appRole);

  syncIntentsToDatabase();

  const auto& modules = m_pluginManager->allModules();
  m_uiManager->createNavBar(modules);

  connect(m_uiManager, &UIManager::navRequested, this, &Kernel::switchToModule);

  for (auto it = modules.begin(); it != modules.end(); ++it) {
    ModuleRecord* record = it.value();
    if (record->manifest["hasUi"].toVariant().toBool()) {
      record->view = record->instance->createView(m_uiManager->moduleStack());
      if (record->view) {
        m_uiManager->moduleStack()->addWidget(record->view);
      }
    }
  }

  if (!modules.isEmpty()) {
    switchToModule(modules.firstKey());
  }

  // Set up the neovim-style input manager
  m_inputManager = new InputManager(m_pluginManager, this);

  connect(m_inputManager, &InputManager::moduleSwitchRequested,
          this, &Kernel::switchToModule);

  connect(m_inputManager, &InputManager::logRequired,
          this, &Kernel::log);

  connect(m_inputManager, &InputManager::modeChanged,
          this, [this](InputManager::Mode mode) {
    QString modeText = (mode == InputManager::Mode::NORMAL)
      ? "-- NORMAL --"
      : "-- INSERT --";
    m_uiManager->setModeIndicator(modeText);
  });

  connect(m_inputManager, &InputManager::bufferChanged,
          this, [this](const QString& buffer) {
    m_uiManager->setCommandBuffer(buffer);
  });

  // Wire up global hotkey signals
  connect(m_inputManager, &InputManager::navigatePrevModule, this, &Kernel::handleNavigatePrevModule);
  connect(m_inputManager, &InputManager::navigateNextModule, this, &Kernel::handleNavigateNextModule);
  connect(m_inputManager, &InputManager::navigateUp,         this, &Kernel::handleNavigateUp);
  connect(m_inputManager, &InputManager::navigateDown,       this, &Kernel::handleNavigateDown);
  connect(m_inputManager, &InputManager::deleteRequested,    this, &Kernel::handleDelete);
  connect(m_inputManager, &InputManager::searchRequested,    this, &Kernel::handleSearch);
  connect(m_inputManager, &InputManager::logoutRequested,    this, &Kernel::handleLogout);

  // Wire up command palette selection
  connect(m_uiManager, &UIManager::paletteActionSelected,
          this, [this](const QString& moduleId, const QString& intentName) {
    switchToModule(moduleId);
    if (!intentName.isEmpty()) {
      launchIntent(moduleId, intentName, QVariantMap());
    }
  });

  qApp->installEventFilter(m_inputManager);
}


void Kernel::teardownSystem() {
  if (m_inputManager) {
    qApp->removeEventFilter(m_inputManager);
    delete m_inputManager;
    m_inputManager = nullptr;
  }

  // Clear all module views from the stack
  if (m_pluginManager) {
    const auto& modules = m_pluginManager->allModules();
    for (auto it = modules.begin(); it != modules.end(); ++it) {
      if (it.value()->instance) {
        it.value()->instance->onShutdown();
      }
      if (it.value()->view) {
        m_uiManager->moduleStack()->removeWidget(it.value()->view);
      }
    }
    delete m_pluginManager;
    m_pluginManager = nullptr;
  }

  m_activeModuleId.clear();
  m_networkManager->setJwtToken("");
  m_uiManager->setConnectionStatus(false);
}


void Kernel::handleAccessDenied() {
  //pushNotification(NotificationType::Error, "Insufficient Permission");
  log("[Kernel] Insufficient permission");
}


void Kernel::syncIntentsToDatabase() {
  QSet<QString> protectedIntents;
  const QMap<QString, ModuleRecord*>& registry = m_pluginManager->allModules();

  for (auto it = registry.begin(); it != registry.end(); ++it) {
    QString moduleId = it.key();
    DModule* genericModule = m_pluginManager->getModuleInstance(moduleId);
    DBaseModule* plugin = dynamic_cast<DBaseModule*>(genericModule);

    if (plugin) {
      protectedIntents.unite(plugin->getProtectedIntents());
    }
  }

  QVariantMap groupedIntents;

  for (const QString& intentString : protectedIntents) {
    QStringList parts = intentString.split(".");
    // Since we are following "plugin.permissionname"
    if (parts.length() == 2) {
      QString pluginName = parts[0];
      QString permissionName = parts[1];

      QVariantList permList = groupedIntents.value(pluginName).toList();

      if (!permList.contains(permissionName)) {
        permList.append(permissionName);
      }

      groupedIntents[pluginName] = permList;
    } else {
      log("[Kernel] Protected intent ignored. Does not match 'plugin.permission' convention");
    }
  }

  QVariantMap finalPayload;
  finalPayload["protected_intents"] = groupedIntents;

  log("Syncing " + QString::number(protectedIntents.size()) + " protected intents to the db...");

  m_networkManager->POST("/api/v1/system/intents", finalPayload, true, [this](QJsonDocument response) {
    if (!response.isNull() && !response.isEmpty()) {
      log("[Kernel] Synced protected intents with the db.");
    } else {
      log("[Kernel] Failed to sync protected intents.");
    }
  });
}


// ─── Global hotkey handlers ────────────────────────────────

void Kernel::handleNavigatePrevModule() {
  if (!m_pluginManager) return;
  const auto& modules = m_pluginManager->allModules();
  QStringList keys = modules.keys();
  if (keys.isEmpty()) return;

  int idx = keys.indexOf(m_activeModuleId);
  int prev = (idx <= 0) ? keys.size() - 1 : idx - 1;
  switchToModule(keys[prev]);
}

void Kernel::handleNavigateNextModule() {
  if (!m_pluginManager) return;
  const auto& modules = m_pluginManager->allModules();
  QStringList keys = modules.keys();
  if (keys.isEmpty()) return;

  int idx = keys.indexOf(m_activeModuleId);
  int next = (idx < 0 || idx >= keys.size() - 1) ? 0 : idx + 1;
  switchToModule(keys[next]);
}

void Kernel::handleNavigateUp() {
  // Send synthetic Up arrow key to the current module's view
  if (!m_pluginManager || m_activeModuleId.isEmpty()) return;
  ModuleRecord* rec = m_pluginManager->getModule(m_activeModuleId);
  if (rec && rec->view) {
    QKeyEvent upPress(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    QKeyEvent upRelease(QEvent::KeyRelease, Qt::Key_Up, Qt::NoModifier);
    QApplication::sendEvent(rec->view, &upPress);
    QApplication::sendEvent(rec->view, &upRelease);
  }
}

void Kernel::handleNavigateDown() {
  // Send synthetic Down arrow key to the current module's view
  if (!m_pluginManager || m_activeModuleId.isEmpty()) return;
  ModuleRecord* rec = m_pluginManager->getModule(m_activeModuleId);
  if (rec && rec->view) {
    QKeyEvent downPress(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QKeyEvent downRelease(QEvent::KeyRelease, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(rec->view, &downPress);
    QApplication::sendEvent(rec->view, &downRelease);
  }
}

void Kernel::handleDelete() {
  // Execute the "delete" intent on the active module (convention-based)
  if (!m_pluginManager || m_activeModuleId.isEmpty()) return;
  ModuleRecord* rec = m_pluginManager->getModule(m_activeModuleId);
  if (rec) {
    // Look for a "d" intent key in the manifest (convention: d = delete)
    QJsonObject intents = rec->manifest["intents"].toObject();
    if (intents.contains("d")) {
      QString deleteMethod = intents["d"].toString();
      log("Delete: " + deleteMethod);
      rec->instance->executeIntent(deleteMethod, QVariantMap());
    } else {
      log("No delete intent defined for module: " + m_activeModuleId);
    }
  }
}

void Kernel::handleSearch() {
  if (!m_pluginManager) return;

  if (m_uiManager->isPaletteVisible()) {
    m_uiManager->hideCommandPalette();
  } else {
    m_uiManager->showCommandPalette(
      m_pluginManager->hotkeyRegistry(),
      m_pluginManager->allModules()
    );
  }
}

void Kernel::handleLogout() {
  log("Logout requested via Alt+L");

  teardownSystem();

  // Show login dialog again
  LoginDialog login(m_networkManager, this);
  login.center();

  if (login.exec() == QDialog::Accepted) {
    log("Re-auth successful");
    initializeSystem();
  } else {
    qApp->quit();
  }
}
