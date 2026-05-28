#ifndef DCORE_H
#define DCORE_H

#include <QString>
#include <QVariantMap>
#include <QJsonDocument>
#include <functional>

class DModule;

enum class NotificationType {
  INFO,
  DEBUG,
  WARNING,
  ERROR,
  SUCCESS
};

class DCore {
  public:
    virtual ~DCore() {}

    virtual void publish(const QString &topic, const QVariantMap &data) = 0;
    virtual void pushNotification(const NotificationType &notificationType, const QString &msg) = 0;

    virtual void switchToModule(const QString &moduleId) = 0;
    virtual void launchIntent(const QString &moduleId, const QString &method, const QVariantMap &data = QVariantMap()) = 0;

    virtual QVariant callService(const QString &moduleId, const QString &method, const QVariantMap &params) = 0;

    virtual void httpGet(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) = 0;
    virtual void httpPost(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) = 0;
    virtual void httpPut(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) = 0;
    virtual void httpDownload(const QString& url, const QString& destPath, std::function<void(bool)> callback) = 0;

    virtual void log(const QString &msg) = 0;

    virtual bool hasPermission(const QString& targetIntent) = 0;
};

#endif
