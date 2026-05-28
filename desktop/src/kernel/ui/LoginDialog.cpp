#include "LoginDialog.h"
#include <QGraphicsDropShadowEffect>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFrame>
#include <QVBoxLayout>
#include <QTimer>
#include <QScreen>

LoginDialog::LoginDialog(NetworkManager* nm, QWidget *parent)
  : QDialog(parent), m_network(nm) {
    setupUi();
}

void LoginDialog::setupUi() {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
  setAttribute(Qt::WA_TranslucentBackground);
  setFixedSize(360, 420);

  this->setStyleSheet(
      "QDialog { background: transparent; border: none; }"
      "#loginContainer { background-color: #ffffff; border: 1px solid #e0e0e0; border-radius: 15px; }"
      "QLineEdit { placeholder-text-color: #888888; color: #222222; padding: 12px; border: 2px solid #cccccc; border-radius: 8px; background: #fcfcfc; font-size: 14px; }"
      "QPushButton#btnLogin { background-color: #2196F3; color: white; padding: 14px; border-radius: 8px; font-weight: bold; font-size: 15px; }"
      "QPushButton#btnLogin:hover { background-color: #1976D2; }"
      "QPushButton#btnLogin:disabled { background-color: #90CAF9; }"
  );

  QFrame* container = new QFrame(this);
  container->setObjectName("loginContainer");

  QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
  shadow->setBlurRadius(20);
  shadow->setXOffset(0);
  shadow->setYOffset(4);
  shadow->setColor(QColor(0, 0, 0, 60));
  container->setGraphicsEffect(shadow);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(10, 10, 10, 10);
  mainLayout->addWidget(container);

  QVBoxLayout* contentLayout = new QVBoxLayout(container);
  contentLayout->setContentsMargins(30, 40, 30, 40);
  contentLayout->setSpacing(15);

  QLabel* logo = new QLabel();
  logo->setObjectName("loginLogo");
  QPixmap pixmap(":/src/resources/logo-blue.svg");
  logo->setAlignment(Qt::AlignCenter);

  if (!pixmap.isNull()) {
    logo->setPixmap(pixmap.scaledToHeight(64, Qt::SmoothTransformation));
    contentLayout->addWidget(logo);
  }

  m_username = new QLineEdit();
  m_username->setPlaceholderText("Username");
  contentLayout->addWidget(m_username);

  m_password = new QLineEdit();
  m_password->setPlaceholderText("Password");
  m_password->setEchoMode(QLineEdit::Password);
  contentLayout->addWidget(m_password);

  m_errorLabel = new QLabel();
  m_errorLabel->setStyleSheet("color: #f44336; font-size: 11px;");
  m_errorLabel->setAlignment(Qt::AlignCenter);
  m_errorLabel->setWordWrap(true);
  contentLayout->addWidget(m_errorLabel);

  m_btnLogin = new QPushButton("Login");
  m_btnLogin->setObjectName("btnLogin");
  contentLayout->addWidget(m_btnLogin);

  QPushButton* btnCancel = new QPushButton("Cancel");
  btnCancel->setStyleSheet("QPushButton { color: #888; border: none; font-size: 13px; } QPushButton:hover { color: #333; }");
  contentLayout->addWidget(btnCancel);

  connect(m_btnLogin, &QPushButton::clicked, this, &LoginDialog::handleLogin);
  connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
  connect(m_password, &QLineEdit::returnPressed, this, &LoginDialog::handleLogin);
  connect(m_username, &QLineEdit::returnPressed, [this]() { m_password->setFocus(); });
}

void LoginDialog::handleLogin() {
  QString user = m_username->text().trimmed();
  QString pass = m_password->text().trimmed();

  if (user.isEmpty() || pass.isEmpty()) {
    m_errorLabel->setText("Please enter username and password.");
    return;
  }

  m_errorLabel->setStyleSheet("color: #2196F3; font-size: 11px;");
  m_errorLabel->setText("Authenticating...");
  m_btnLogin->setEnabled(false);

  QVariantMap creds;
  creds["username"] = user;
  creds["password"] = pass;

  if (!m_network) {
    m_errorLabel->setStyleSheet("color: #f44336; font-size: 11px;");
    m_errorLabel->setText("Network unavailable.");
    m_btnLogin->setEnabled(true);
    return;
  }

  m_network->POST("/api/v1/login", creds, false, [this](QJsonDocument doc) {
    m_btnLogin->setEnabled(true);

    if (doc.isNull() || doc.isEmpty()) {
      m_errorLabel->setStyleSheet("color: #f44336; font-size: 11px;");
      m_errorLabel->setText("Cannot reach server. Is it running?");
      return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("token")) {
      m_network->setJwtToken(obj["token"].toString());
      m_currentStaffId = obj["id"].toString();
      m_currentStaffRole = obj["role"].toString();
      accept();
    } else {
      m_errorLabel->setStyleSheet("color: #f44336; font-size: 11px;");
      QString msg = obj["message"].toString();
      m_errorLabel->setText(msg.isEmpty() ? "Invalid credentials." : msg);
      m_password->clear();
      m_password->setFocus();
    }
  });
}

void LoginDialog::center() {
  if (parentWidget()) {
    if (parentWidget() -> isMaximized()) {
      QRect screenGeom = parentWidget()->screen()->geometry();
      move(screenGeom.center().x() - width()/2, screenGeom.center().y() - height()/2);
    } else {
      QPoint centerPos = parentWidget()->geometry().center();
      move(centerPos.x() - width() / 2, centerPos.y() - height() / 2);
    }
  }
}
