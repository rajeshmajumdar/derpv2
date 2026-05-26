#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QTimer>

class ServerManager : public QObject {
  Q_OBJECT

  public:
    explicit ServerManager(QObject *parent = nullptr);
    ~ServerManager();

    void initialize(const QString& appRole);
    bool isRunning() const;
    bool isReady() const { return m_readyState; }

    void stopServer();

  signals:
    void serverReady();
    void serverFault(const QString& details);
    void logReceived(const QString& message);

  private slots:
    void checkHeartbeat();
    void handlePlaybackLogs();
    void onProcessError(QProcess::ProcessError error);

  private:
    void coldBootServer();
    void wireProcessHooks();

    QProcess* m_serverProcess;
    QNetworkAccessManager* m_networkCheck;
    QTimer* m_heartbeatTimer;

    QString m_appRole;
    int m_retryCount;
    bool m_readyState;
    const int MAX_RETRIES = 30;
};

#endif // !SERVERMANAGER_H
