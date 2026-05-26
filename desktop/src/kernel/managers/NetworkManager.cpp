#include "NetworkManager.h"

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
  m_manager = new QNetworkAccessManager(this);
}


QNetworkRequest NetworkManager::makeRequest(const QString& url, bool isAuthenticated) {
  QString fullUrl = url;
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    fullUrl = m_apiBaseUrl + url;
  }
  QNetworkRequest request((QUrl(fullUrl)));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  if (isAuthenticated && !m_jwtToken.isEmpty()) {
    request.setRawHeader("Authorization", "Bearer " + m_jwtToken.toLocal8Bit());
  }

  return request;
}


void NetworkManager::performRequest(QNetworkReply* resp, std::function<void(QJsonDocument)> callback) {
  connect(resp, &QNetworkReply::finished, [resp, callback]() {
    QJsonDocument result;
    if (resp->error() == QNetworkReply::NoError) {
      result = QJsonDocument::fromJson(resp->readAll());
    } else {
      qWarning() << "[NetworkManager] Error: " << resp->errorString();
    }
    callback(result);
    resp->deleteLater();
  });
}


void NetworkManager::GET(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) {
  QNetworkRequest request = makeRequest(url, isAuthenticated);

  if (data.isEmpty()) {
    performRequest(m_manager->get(request), callback);
  } else {
    QJsonDocument doc = QJsonDocument::fromVariant(data);
    performRequest(m_manager->get(request, doc.toJson()), callback);
  }
}


void NetworkManager::POST(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) {
  QJsonDocument doc = QJsonDocument::fromVariant(data);
  performRequest(m_manager->post(makeRequest(url, isAuthenticated), doc.toJson()), callback);
}


void NetworkManager::PUT(const QString& url, const QVariantMap& data, bool isAuthenticated, std::function<void(QJsonDocument)> callback) {
  QJsonDocument doc = QJsonDocument::fromVariant(data);
  performRequest(m_manager->put(makeRequest(url, isAuthenticated), doc.toJson()), callback);
}


void NetworkManager::startDownload(const QString& url, const QString& destPath, std::function<void(bool)> callback) {
  QNetworkReply* resp = m_manager->get(QNetworkRequest(QUrl(url)));

  connect(resp, &QNetworkReply::finished, [resp, destPath, callback]() {
      bool success = false;
      if (resp->error() == QNetworkReply::NoError) {
        QFile file(destPath);
        if (file.open(QIODevice::WriteOnly)) {
          file.write(resp->readAll());
          file.close();
          success = true;
        }
      } else {
        qWarning() << "[NetworkManager] Download failed" << resp->errorString();
      }
      callback(success);
      resp->deleteLater();
    });
}


void NetworkManager::startDiscovery() {
  if (!m_udpSocket) {
    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onDiscoveryReply);
  }
  m_udpSocket->bind(0);
  m_udpSocket->writeDatagram("DERP_DISCOVER_REQUEST", QHostAddress::Broadcast, 5555);
}

void NetworkManager::onDiscoveryReply() {
  while (m_udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(m_udpSocket->pendingDatagramSize());
    QHostAddress sender;
    m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender);

    QString res = QString::fromUtf8(datagram);
    if (res.startsWith("DERP_MASTER_IDENTITY")) {
      QString ip = sender.toString().remove("::ffff:");
      m_apiBaseUrl = QString("http://%1:5000").arg(ip);

      QString name = res.split("|").value(1);
      emit masterFound(name, ip);
    }
  }
}
