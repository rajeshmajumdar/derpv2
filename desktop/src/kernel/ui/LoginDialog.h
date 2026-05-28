#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

#include "../managers/NetworkManager.h"

class LoginDialog : public QDialog {
  Q_OBJECT

  public:
    explicit LoginDialog(NetworkManager* nm, QWidget *parent = nullptr);
    void center();

    QString getCurrentStaffId() { return m_currentStaffId; }
    QString getCurrentStaffRole() { return m_currentStaffRole; }

  private slots:
    void handleLogin();

  private:
    NetworkManager* m_network;
    QLineEdit* m_username;
    QLineEdit* m_password;
    QLabel* m_errorLabel;
    QPushButton* m_btnLogin;
    QString m_currentStaffId;
    QString m_currentStaffRole;

    void setupUi();
};

#endif
