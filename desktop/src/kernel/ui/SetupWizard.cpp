#include "SetupWizard.h"
#include <QHostAddress>
#include <QScrollArea>
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QJsonObject>

SetupWizard::SetupWizard(ServerManager* serverManager, QWidget *parent)
  : QDialog(parent), m_serverManager(serverManager), m_discoveryTimer(nullptr), m_discoveryAttempts(0) {

  m_networkManager = new NetworkManager(this);
  connect(m_networkManager, &NetworkManager::masterFound, this, &SetupWizard::addServerToList);

  m_stackedWidget = new QStackedWidget(this);

  m_btnSave = new QPushButton("Finish setup");
  m_btnSave->setStyleSheet("background-color: #4CAF50; color: white; padding: 12px; font-weight: bold; border-radius: 5px; ");

  setupRolePage();
  setupCompanyPage();
  setupAccountPage();

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(m_stackedWidget);
  mainLayout->addWidget(m_btnSave);

  setFixedSize(450, 500);
  setWindowTitle("derp - Configuration");
  setStyleSheet("background-color: #ffffff; color: #333333; ");

  connect(m_btnSave, &QPushButton::clicked, this, &SetupWizard::onSaveClicked);
}


SetupWizard::~SetupWizard() {
  stopStaffDiscovery();
}


void SetupWizard::setupAccountPage() {
  m_accountPage = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(m_accountPage);
  layout->setContentsMargins(30, 30, 30, 30);
  layout->setSpacing(15);

  QLabel* title = new QLabel("Account setup");
  title->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
  title->setAlignment(Qt::AlignCenter);
  layout->addWidget(title);

  auto styleInput = [](QLineEdit* e) {
    e->setStyleSheet("padding: 8px; border: 1px solid #ccc; border-radius: 4px;");
  };

  m_username = new QLineEdit(); styleInput(m_username);
  m_username->setPlaceholderText("Username");

  m_password = new QLineEdit(); styleInput(m_password);
  m_password->setEchoMode(QLineEdit::Password);
  m_password->setPlaceholderText("Password");

  layout->addWidget(new QLabel("Set up credentials to access master nodes."));
  layout->addWidget(m_username);
  layout->addWidget(m_password);
  layout->addStretch();

  m_stackedWidget->addWidget(m_accountPage);
}

void SetupWizard::setupRolePage() {
  m_rolePage = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(m_rolePage);
  layout->setContentsMargins(30, 30, 30, 30);
  layout->setSpacing(15);

  m_title = new QLabel("Welcome to dERP\nWho will be using this PC?");
  m_title->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
  m_title->setAlignment(Qt::AlignCenter);
  layout->addWidget(m_title);
 
  m_btnMaster = new QPushButton("Master (Server & DB)");
  m_btnStaff = new QPushButton("Staff (Counter)");

  QString btnStyle = "QPushButton { padding: 15px; font-size: 14px; border: 2px solid #ddd; border-radius: 8px; color: #333; background-color: #f9f9f9; }"
                     "QPushButton:checked { border: 2px solid #2196F3; background-color: #E3F2FD; color: #0d47a1; }";

  m_btnMaster->setCheckable(true);
  m_btnStaff->setCheckable(true);
  m_btnMaster->setStyleSheet(btnStyle);
  m_btnStaff->setStyleSheet(btnStyle);

  layout->addWidget(m_btnStaff);
  layout->addWidget(m_btnMaster);

  m_staffOptions = new QWidget();
  QVBoxLayout* staffLayout = new QVBoxLayout(m_staffOptions);
  staffLayout->setContentsMargins(0, 10, 0, 0);

  m_discoveryStatus = new QLabel("Searching for servers...");
  m_discoveryStatus->setStyleSheet("color: #1976D2; font-size: 12px;");
  m_discoveryStatus->setAlignment(Qt::AlignCenter);
  m_discoveryStatus->setVisible(false);
  staffLayout->addWidget(m_discoveryStatus);

  m_serverList = new QListWidget();
  m_serverList->setStyleSheet("QListWidget { border: 1px solid #ddd; border-radius: 4px; padding: 5px; }");
  staffLayout->addWidget(m_serverList);

  m_scanProgress = new QProgressBar();
  m_scanProgress->setRange(0, 0);
  m_scanProgress->setFixedHeight(8);
  m_scanProgress->setTextVisible(false);
  m_scanProgress->setVisible(false);
  staffLayout->addWidget(m_scanProgress);

  m_retryButton = new QPushButton("Scan again");
  m_retryButton->setStyleSheet("QPushButton { padding: 6px; border: 1px solid #ccc; border-radius: 4px; color: #333; background: #f5f5f5; }");
  m_retryButton->setVisible(false);
  connect(m_retryButton, &QPushButton::clicked, this, &SetupWizard::startStaffDiscovery);
  staffLayout->addWidget(m_retryButton);

  QLabel* manualLabel = new QLabel("<a href='#'>Enter IP manually</a>");
  manualLabel->setAlignment(Qt::AlignRight);
  staffLayout->addWidget(manualLabel);

  m_ipInput = new QLineEdit();
  m_ipInput->setPlaceholderText("Enter IP (e.g. 192.168.1.1)");
  m_ipInput->setStyleSheet("padding: 8px; border: 1px solid #ccc; border-radius: 4px; ");
  m_ipInput->setVisible(false);
  staffLayout->addWidget(m_ipInput);

  m_staffOptions->setVisible(false);
  layout->addWidget(m_staffOptions);

  connect(m_btnMaster, &QPushButton::clicked, this, &SetupWizard::onMasterSelected);
  connect(m_btnStaff, &QPushButton::clicked, this, &SetupWizard::onStaffSelected);
  connect(manualLabel, &QLabel::linkActivated, [this]() { m_ipInput->setVisible(true); });

  m_stackedWidget->addWidget(m_rolePage);
}


void SetupWizard::setupCompanyPage() {
  m_companyPage = new QWidget();
  QVBoxLayout* mainLayout = new QVBoxLayout(m_companyPage);

  QScrollArea* scroll = new QScrollArea();
  QWidget* scrollContent = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(scrollContent);
  layout->setContentsMargins(20, 20, 20, 20);

  QLabel* title = new QLabel("Company Information");
  title->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
  layout->addWidget(title);

  QFormLayout* form = new QFormLayout();
  form->setSpacing(15);

  auto styleInput = [](QLineEdit* e) {
    e->setStyleSheet("padding: 8px; border: 1px solid #ccc; border-radius: 4px;");
  };

  m_companyName = new QLineEdit(); styleInput(m_companyName);
  m_companyName->setPlaceholderText("Durga Pharmacy");

  m_tagline = new QLineEdit(); styleInput(m_tagline);
  m_tagline->setPlaceholderText("Pharmaceutical Distributor");

  m_companyAddress = new QLineEdit(); styleInput(m_companyAddress);
  m_companyAddress->setPlaceholderText("Sec-142, Noida, U.P.");

  m_dlNumber = new QLineEdit(); styleInput(m_dlNumber);
  m_dlNumber->setPlaceholderText("Drug License no. (e.g. 20B/21B-XXXX)");

  m_gstin = new QLineEdit(); styleInput(m_gstin);
  m_gstin->setPlaceholderText("GSTIN No.");

  m_iecCode = new QLineEdit(); styleInput(m_iecCode);
  m_iecCode->setPlaceholderText("IEC Code");

  m_fssaiCode = new QLineEdit(); styleInput(m_fssaiCode);
  m_fssaiCode->setPlaceholderText("FSSAI Code");

  m_contactNumber = new QLineEdit(); styleInput(m_contactNumber);
  m_contactNumber->setPlaceholderText("7290828982");

  m_website = new QLineEdit(); styleInput(m_website);
  m_website->setPlaceholderText("www.durga.care");

  m_email = new QLineEdit(); styleInput(m_email);
  m_email->setPlaceholderText("email@durga.care");

  form->addRow("Business name:", m_companyName);
  form->addRow("Tagline:", m_tagline);
  form->addRow("Address:", m_companyAddress);
  form->addRow("Contact:", m_contactNumber);
  form->addRow("Email:", m_email);
  form->addRow("Website:", m_website);
  form->addRow("Drug License no.:", m_dlNumber);
  form->addRow("GSTIN:", m_gstin);
  form->addRow("FSSAI No.:", m_fssaiCode);
  form->addRow("IEC Code:", m_iecCode);

  layout->addLayout(form);
  layout->addStretch();

  scroll->setWidget(scrollContent);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);

  mainLayout->addWidget(scroll);
  m_stackedWidget->addWidget(m_companyPage);
}


void SetupWizard::onMasterSelected() {
  m_selectedRole = "master";
  m_btnMaster->setChecked(true);
  m_btnStaff->setChecked(false);
  m_staffOptions->setVisible(false);
  m_btnSave->setText("Continue");
}


void SetupWizard::onStaffSelected() {
  m_selectedRole = "staff";
  m_btnMaster->setChecked(false);
  m_btnStaff->setChecked(true);
  m_staffOptions->setVisible(true);
  m_btnSave->setText("Finish setup");

  startStaffDiscovery();
}


void SetupWizard::startStaffDiscovery() {
  stopStaffDiscovery();

  m_serverList->clear();
  m_scanProgress->setVisible(true);
  m_discoveryStatus->setText("Searching for servers...");
  m_discoveryStatus->setStyleSheet("color: #1976D2; font-size: 12px;");
  m_discoveryStatus->setVisible(true);
  m_retryButton->setVisible(false);

  m_discoveryAttempts = 0;

  m_discoveryTimer = new QTimer(this);
  connect(m_discoveryTimer, &QTimer::timeout, this, &SetupWizard::onDiscoveryTick);
  m_discoveryTimer->start(DISCOVERY_INTERVAL_MS);

  onDiscoveryTick();
}


void SetupWizard::stopStaffDiscovery() {
  if (m_discoveryTimer) {
    m_discoveryTimer->stop();
    delete m_discoveryTimer;
    m_discoveryTimer = nullptr;
  }
}


void SetupWizard::onDiscoveryTick() {
  m_discoveryAttempts++;
  m_networkManager->startDiscovery();

  if (m_discoveryAttempts >= MAX_DISCOVERY_ATTEMPTS) {
    stopStaffDiscovery();
    m_scanProgress->setVisible(false);

    if (m_serverList->count() == 0) {
      m_discoveryStatus->setText("No servers found on the network.");
      m_discoveryStatus->setStyleSheet("color: #F44336; font-size: 12px;");
      m_retryButton->setVisible(true);
    } else {
      m_discoveryStatus->setText(QString("Found %1 server(s).").arg(m_serverList->count()));
      m_discoveryStatus->setStyleSheet("color: #2e7d32; font-size: 12px;");
    }
  }
}


void SetupWizard::addServerToList(const QString &name, const QString &ip) {
  for (int i = 0; i < m_serverList->count(); i++) {
    if (m_serverList->item(i)->data(Qt::UserRole).toString() == ip) {
      return;
    }
  }
  QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2)").arg(name, ip));
  item->setData(Qt::UserRole, ip);
  m_serverList->addItem(item);

  m_discoveryStatus->setText(QString("Found %1 server(s).").arg(m_serverList->count()));
  m_discoveryStatus->setStyleSheet("color: #2e7d32; font-size: 12px;");
}


void SetupWizard::showError(const QString& message) {
  m_btnSave->setEnabled(true);
  m_btnSave->setText("Complete setup");
  QMessageBox::warning(this, "Setup Error", message);
}


void SetupWizard::performSetupPost() {
  if (m_serverManager && !m_serverManager->isReady()) {
    showError("Server is still starting up.\nPlease wait a moment and try again.");
    return;
  }

  m_btnSave->setEnabled(false);
  m_btnSave->setText("Saving...");

  QVariantMap payload;

  QVariantMap companyData;
  companyData["name"] = m_companyName->text();
  companyData["address"] = m_companyAddress->text();
  companyData["tagline"] = m_tagline->text();
  companyData["contact"] = m_contactNumber->text();
  companyData["email"] = m_email->text();
  companyData["website"] = m_website->text();
  companyData["dl_number"] = m_dlNumber->text();
  companyData["gstin"] = m_gstin->text();
  companyData["fssai"] = m_fssaiCode->text();
  companyData["iec_code"] = m_iecCode->text();
  payload["company"] = companyData;

  QVariantMap admin;
  admin["username"] = m_username->text().trimmed();
  admin["password"] = m_password->text().trimmed();
  payload["admin"] = admin;

  m_networkManager->POST("http://127.0.0.1:5000/api/v1/setup", payload, false, [this](QJsonDocument doc) {
    m_btnSave->setEnabled(true);
    m_btnSave->setText("Complete setup");

    if (doc.isNull() || doc.isEmpty()) {
      showError("Cannot reach server.\nPlease ensure the server is running and try again.");
      return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("error")) {
      showError("Setup failed: " + obj["error"].toString());
      return;
    }
    if (obj.contains("message")) {
      QString msg = obj["message"].toString();
      if (msg.toLower().contains("fail") || msg.toLower().contains("error")) {
        showError(msg);
        return;
      }
    }

    accept();
  });
}


void SetupWizard::onSaveClicked() {
  if (m_selectedRole.isEmpty()) return;

  QSettings settings("derp", "AppConfig");
  settings.setValue("role", m_selectedRole);

  if (m_stackedWidget->currentIndex() == 0) {
    if (m_selectedRole == "master") {
      if (m_serverManager && !m_serverManager->isRunning()) {
        m_serverManager->initialize("master");
      }
      m_stackedWidget->setCurrentIndex(1);
      m_btnSave->setText("Continue");
      return;

    } else {
      QString ip;

      if (m_ipInput->isVisible() && !m_ipInput->text().isEmpty()) {
        ip = m_ipInput->text().trimmed();
      } else if (m_serverList->currentItem()) {
        ip = m_serverList->currentItem()->data(Qt::UserRole).toString();
      }

      if (ip.isEmpty()) {
        m_ipInput->setVisible(true);
        m_ipInput->setStyleSheet("border: 1px solid red; padding: 8px;");
        return;
      }

      m_ipInput->setStyleSheet("padding: 8px; border: 1px solid #ccc; border-radius: 4px; ");
      settings.setValue("master_ip", ip);
      settings.sync();
      accept();
    }
  } else if (m_stackedWidget->currentIndex() == 1) {
    m_stackedWidget->setCurrentIndex(2);
    m_btnSave->setText("Complete setup");
  } else {
    if (m_username->text().isEmpty() || m_password->text().isEmpty()) {
      m_username->setStyleSheet("border: 1px solid red; padding: 8px;");
      m_password->setStyleSheet("border: 1px solid red; padding: 8px;");
      return;
    }

    performSetupPost();
  }
}
