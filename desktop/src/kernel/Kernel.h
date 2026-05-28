#ifndef KERNEL_H
#define KERNEL_H

#include <QDebug>
#include <QTimer>
#include <QDialog>
#include <QSettings>
#include <QProcess>
#include <QJsonArray>
#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>
#include <QVariant>
#include "../interfaces/DCore.h"
#include "PluginManager.h"
#include "InputManager.h"
#include "UIManager.h"
#include "NetworkManager.h"
#include "SetupWizard.h"
#include "controllers/ServerManager.h"

class ServerManager;

class Kernel : public QMainWindow, public DCore {
  Q_OBJECT

  public:
    explicit Kernel(QWidget *parent = nullptr);
    ~Kernel();

    void publish(const QString &topic, const QVariantMap &data) override;
    void pushNotification(const NotificationType &notificationType, const QString &msg) override;

    void switchToModule(const QString &moduleId) override;
    void launchIntent(const QString &moduleId, const QString &method, const QVariantMap &data) override;

    QVariant callService(const QString &moduleId, const QString &method, const QVariantMap &params) override;

    void httpGet(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) override;
    void httpPost(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) override;
    void httpPut(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) override;
    void httpDownload(const QString& url, const QString& destPath, std::function<void(bool)> callback) override;

    void log(const QString &msg) override;

    QStackedWidget* moduleStack() const;

    bool hasPermission(const QString& targetIntent) override;

  private slots:
    // Catches the global 403 forbidden signal from NetworkManager
    void handleAccessDenied();

  private:
    PluginManager *m_pluginManager = nullptr;
    InputManager *m_inputManager = nullptr;
    UIManager *m_uiManager;
    NetworkManager* m_networkManager;
    ServerManager* m_serverManager;
    QString m_appRole;
    QString m_apiBaseUrl;
    QString m_activeModuleId;
    QString m_activeStaffId;
    QString m_activeUserRole;
    QSet<QString> m_activePermissionsCache;

    void cacheActiveUserPermissions();

    void startAuthFlow();
    void initializeSystem();
    void initStaffConnection();
    void teardownSystem();

    void syncIntentsToDatabase();

    // Global hotkey handlers
    void handleNavigatePrevModule();
    void handleNavigateNextModule();
    void handleNavigateUp();
    void handleNavigateDown();
    void handleDelete();
    void handleSearch();
    void handleLogout();
};

#endif // !KERNEL_H
