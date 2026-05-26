#include "ServerManager.h"
#include <QApplication>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>

ServerManager::ServerManager(QObject *parent)
  : QObject(parent), m_serverProcess(nullptr), m_retryCount(0), m_readyState(false) {
    m_networkCheck = new QNetworkAccessManager(this);
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ServerManager::checkHeartbeat);
  }


ServerManager::~ServerManager() {
  if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
    emit logReceived("[ServerManager] Shutting down the server...");
    m_serverProcess->terminate();
    if (!m_serverProcess->waitForFinished(3000)) {
      emit logReceived("[ServerManager] Backend unresponsive. Force terminating process...");
      m_serverProcess->kill();
    }
  }
}


void ServerManager::initialize(const QString& appRole) {
  m_appRole = appRole;

  if (m_appRole != "master") {
    m_readyState = true;
    emit serverReady();
    return;
  }

  coldBootServer();

  if (m_serverProcess) {
    wireProcessHooks();
    m_retryCount = 0;
    m_readyState = false;
    m_heartbeatTimer->start(150);
  }
}


void ServerManager::stopServer() {
  if (m_heartbeatTimer->isActive()) {
    m_heartbeatTimer->stop();
  }
  m_readyState = false;
  if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
    m_serverProcess->terminate();
    m_serverProcess->waitForFinished(3000);
    if (m_serverProcess->state() == QProcess::Running) {
      m_serverProcess->kill();
    }
  }
}


bool ServerManager::isRunning() const {
  return m_serverProcess && m_serverProcess->state() == QProcess::Running;
}


void ServerManager::coldBootServer() {
  QString serverPath = QApplication::applicationDirPath() + "/server.exe";
  if (!QFile::exists(serverPath)) {
    emit serverFault("[ServerManager] server.exe not found at: " + serverPath);
    return;
  }

  emit logReceived("[ServerManager] Starting server process...");
  m_serverProcess = new QProcess(this);
  m_serverProcess->start(serverPath);
}


void ServerManager::wireProcessHooks() {
  disconnect(m_serverProcess, &QProcess::readyReadStandardOutput, this, &ServerManager::handlePlaybackLogs);
  connect(m_serverProcess, &QProcess::readyReadStandardOutput, this, &ServerManager::handlePlaybackLogs);
  connect(m_serverProcess, &QProcess::errorOccurred, this, &ServerManager::onProcessError);
}


void ServerManager::handlePlaybackLogs() {
  emit logReceived("[ServerManager] " + m_serverProcess->readAllStandardOutput().trimmed());
}


void ServerManager::checkHeartbeat() {
  m_retryCount++;
  if (m_retryCount > MAX_RETRIES) {
    m_heartbeatTimer->stop();
    emit serverFault("[ServerManager] Server heartbeat timed out after " + QString::number(MAX_RETRIES) + " attempts. Is port 5000 available?");
    return;
  }

  QNetworkRequest request(QUrl("http://127.0.0.1:5000/api/v1/ping"));
  QNetworkReply* reply = m_networkCheck->get(request);

  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError) {
      m_heartbeatTimer->stop();
      m_readyState = true;
      emit logReceived(QString("Server online after %1 attempt(s).").arg(m_retryCount));
      emit serverReady();
    }
  });
}


void ServerManager::onProcessError(QProcess::ProcessError error) {
  if (error == QProcess::Crashed) {
    m_heartbeatTimer->stop();
    m_readyState = false;
    emit serverFault("[ServerManager] Server process crashed unexpectedly.");
  }
}
