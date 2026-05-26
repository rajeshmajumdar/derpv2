#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QUdpSocket>
#include <functional>

class NetworkManager : public QObject {
  Q_OBJECT

  public:
    explicit NetworkManager(QObject* parent = nullptr);

    void setJwtToken(const QString& token) { m_jwtToken = token; }
    void setApiBaseUrl(const QString& url) { m_apiBaseUrl = url; }

    void GET(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback);
    void POST(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback);
    void PUT(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback);

    void startDownload(const QString& url, const QString& destPath, std::function<void(bool)> callback);

    void startDiscovery();
    QString apiBaseUrl() const { return m_apiBaseUrl; }

  signals:
    void masterFound(const QString& name, const QString& ip);
    void accessDenied();

  private slots:
    void onDiscoveryReply();

  private:
    QNetworkRequest makeRequest(const QString& url, bool isAuthenticated);
    void performRequest(QNetworkReply* resp, std::function<void(QJsonDocument)> callback);

    QNetworkAccessManager* m_manager;
    QUdpSocket* m_udpSocket;
    QString m_jwtToken;
    QString m_apiBaseUrl;
};
#endif // !NETWORKMANAGER_H
