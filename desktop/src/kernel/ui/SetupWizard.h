#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QSettings>
#include <QListWidget>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QFormLayout>
#include <QTimer>

#include "../managers/NetworkManager.h"
#include "../controllers/ServerManager.h"

class SetupWizard: public QDialog {
  Q_OBJECT

  public:
    explicit SetupWizard(ServerManager* serverManager, QWidget *parent = nullptr);
    ~SetupWizard();

  private slots:
    void onMasterSelected();
    void onStaffSelected();
    void onSaveClicked();
    void onDiscoveryTick();

  private:
    QLabel *m_title;
    QPushButton *m_btnMaster;
    QPushButton *m_btnStaff;
    QPushButton *m_btnSave;

    QWidget *m_staffOptions;
    QLineEdit *m_ipInput;
    QLabel *m_discoveryStatus;
    QPushButton *m_retryButton;

    QStackedWidget* m_stackedWidget;
    QWidget* m_rolePage;
    QWidget* m_companyPage;
    QWidget* m_accountPage;

    QLineEdit* m_companyName;
    QLineEdit* m_tagline;
    QLineEdit* m_companyAddress;
    QLineEdit* m_dlNumber;
    QLineEdit* m_gstin;
    QLineEdit* m_iecCode;
    QLineEdit* m_fssaiCode;
    QLineEdit* m_contactNumber;
    QLineEdit* m_website;
    QLineEdit* m_email;

    QLineEdit* m_username;
    QLineEdit* m_password;

    QString m_selectedRole;

    QListWidget *m_serverList;
    QProgressBar *m_scanProgress;
    NetworkManager *m_networkManager;
    ServerManager* m_serverManager;

    QTimer* m_discoveryTimer;
    int m_discoveryAttempts;
    static const int MAX_DISCOVERY_ATTEMPTS = 5;
    static const int DISCOVERY_INTERVAL_MS = 2000;

    void setupRolePage();
    void setupCompanyPage();
    void setupAccountPage();
    void addServerToList(const QString &name, const QString &ip);
    void startStaffDiscovery();
    void stopStaffDiscovery();
    void performSetupPost();
    void showError(const QString& message);
};

#endif // !SETUPWIZARD_H
