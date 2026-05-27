#include "StaffManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QCheckBox>
#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QGraphicsDropShadowEffect>

// ---------------------------------------------------
// FLOATING ADD NEW STAFF DIALOG
// ---------------------------------------------------
class AddStaffDialog : public QDialog {
  public:
    explicit AddStaffDialog(QWidget* parent = nullptr) : QDialog(parent) {
      setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
      setAttribute(Qt::WA_TranslucentBackground);
      setFixedSize(360, 420);

      this->setStyleSheet(
        "QDialog { background: transparent; border: none; }"
        "#addContainer { background-color: #ffffff; border: 1px solid #e0e0e0; border-radius: 15px; }"
        "QLineEdit { placeholder-text-color: #888888; color: #222222; padding: 12px; border: 2px solid #cccccc; border-radius: 8px; background: #fcfcfc; font-size: 14px; }"
        "QPushButton#btnAdd { background-color: #2196F3; color: white; padding: 14px; border-radius: 8px; font-weight: bold; font-size: 15px; }"
        "QPushButton#btnAdd:hover { background-color: #1976D2; }"
        "QPushButton#btnAdd:disabled { background-color: #90CAF9; }"
        "QPushButton#btnCancel { background-color: transparent; color: #757575; padding: 10px; font-weight: bold; font-size: 14px; border-radius: 8px; }"
        "QPushButton#btnCancel:hover { background-color: #f5f5f5; color: #d32f2f; }"
      );

      QFrame* container = new QFrame(this);
      container->setObjectName("addContainer");

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

      //Header
      QLabel* title = new QLabel("Add new staff", container);
      title->setStyleSheet("font-size: 22px; font-weight: 900;");
      title->setAlignment(Qt::AlignCenter);
      contentLayout->addWidget(title);
      contentLayout->addSpacing(10);

      // Inputs
      m_usernameEdit = new QLineEdit(container);
      m_usernameEdit->setPlaceholderText("Username");
      contentLayout->addWidget(m_usernameEdit);

      m_passwordEdit = new QLineEdit(container);
      m_passwordEdit->setPlaceholderText("Password");
      m_passwordEdit->setEchoMode(QLineEdit::Password);
      contentLayout->addWidget(m_passwordEdit);

      contentLayout->addStretch();

      QPushButton* btnAdd = new QPushButton("Create Account", container);
      btnAdd->setObjectName("btnAdd");
      contentLayout->addWidget(btnAdd);

      QPushButton* btnCancel = new QPushButton("Cancel", container);
      btnCancel->setObjectName("btnCancel");
      contentLayout->addWidget(btnCancel);

      connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
      connect(btnAdd, &QPushButton::clicked, [this]() {
        if (!m_usernameEdit->text().isEmpty() && !m_passwordEdit->text().isEmpty()) {
          accept();
        }
      });
    }

    QString getUsername() const { return m_usernameEdit->text(); }
    QString getPassword() const { return m_passwordEdit->text(); }

  private:
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
};


// -------------------------
// Staff manager impl
// -------------------------

StaffManager::StaffManager(QObject* parent)
  : DBaseModule(":/staff_manager/manifest.json", parent) {}


StaffManager::~StaffManager() {
  if (m_widget && !m_widget->parent()) {
    delete m_widget;
  }
}


void StaffManager::onInitialize() {
  if (m_core) m_core->log("[StaffManager] initialized");
}

void StaffManager::onShutdown() {
  if (m_core) m_core->log("[StaffManager] shutting down");
}


QWidget* StaffManager::createView(QWidget* parent) {
  if (!m_widget) {
    m_widget = new QWidget(parent);

    auto* rootLayout = new QVBoxLayout(m_widget);
    rootLayout->setContentsMargins(15, 15, 15, 15);
    rootLayout->setSpacing(15);

    auto* splitLayout = new QHBoxLayout();
    splitLayout->setSpacing(20);

    QString tableStyle = "QTableWidget {"
                        "   background-color: #FFFFFF;"
                        "   color: #212121;"
                        "   gridline-color: #E0E0E0;"
                        "   border: 1px solid #B0BEC5;"
                        "   border-radius: 4px;"
                        "   selection-background-color: #1565C0;"
                        "   selection-color: #FFFFFF;"
                        "}"
                        "QTableWidget::item:selected {"
                        "   background-color: #1565C0;"
                        "   color: #FFFFFF;"
                        "}"
                        "QHeaderView::section {"
                        "   background-color: #ECEFF1;"
                        "   color: #37474F;"
                        "   font-weight: bold;"
                        "   padding: 6px;"
                        "   border: 1px solid #CFD8DC;"
                        "}";

    auto* leftPane = new QWidget(m_widget);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // Search bar
    m_searchBar = new QLineEdit(leftPane);
    m_searchBar->setPlaceholderText("SEARCH FOR STAFF... [F5]");
    m_searchBar->setFixedHeight(35);
    m_searchBar->setStyleSheet("QLineEdit {"
                              "   background-color: #FFFFFF;"
                              "   color: #212121;"
                              "   border: 1px solid #B0BEC5;"
                              "   border-radius: 4px;"
                              "   padding-left: 10px;"
                              "}");
    leftLayout->addWidget(m_searchBar);

    // Directory grid table
    m_staffTable = new QTableWidget(0, 4, leftPane);
    m_staffTable->setHorizontalHeaderLabels({"ID", "NAME", "ROLE", "STATUS"});
    m_staffTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_staffTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_staffTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_staffTable->setStyleSheet(tableStyle);
    leftLayout->addWidget(m_staffTable);

    // RIGHT PANE
    auto* rightPane = new QWidget(m_widget);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* topRightLayout =new QHBoxLayout();

    m_activeUserLabel = new QLabel("ADM-01 / D. ADM\nPERMISSION_LEVEL: LEVEL_4_MGMT", rightPane);
    m_activeUserLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #1A237E;");

    auto* newStaffBtn = new QPushButton("ADD STAFF [F2]", rightPane);
    newStaffBtn->setStyleSheet("QPushButton { background-color: #1565C0; color: white; font-weight: bold; padding: 8px; border-radius: 4px; }");

    connect(newStaffBtn, &QPushButton::clicked, [this]() {
      executeIntent("staff.create", QVariantMap());
    });

    topRightLayout->addWidget(m_activeUserLabel);
    topRightLayout->addStretch();
    topRightLayout->addWidget(newStaffBtn);
    rightLayout->addLayout(topRightLayout);

    m_permissionMatrixTable = new QTableWidget(0, 5, rightPane);
    m_permissionMatrixTable->setHorizontalHeaderLabels({"SYSTEM MODULE", "READ", "WRITE", "DELETE", "OVERRIDE"});
    m_permissionMatrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_permissionMatrixTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_permissionMatrixTable->setStyleSheet(tableStyle);
    rightLayout->addWidget(m_permissionMatrixTable);

    leftLayout->addStretch(1);
    rightLayout->addStretch(1);

    // Immediate action button
    auto* buttonLayout = new QHBoxLayout();
    m_revokeBtn = new QPushButton("REVOKE ACCESS [DEL]", rightPane);
    m_revokeBtn->setStyleSheet("QPushButton { background-color: #D32F2F; color: white; font-weight: bold; padding: 8px; border-radius: 4px; }");

    auto* cancelBtn = new QPushButton("CANCEL", rightPane);

    m_saveBtn = new QPushButton("SAVE CHANGES [F10]", rightPane);
    m_saveBtn->setStyleSheet("QPushButton { background-color: #1565C0; color: white; font-weight: bold; padding: 8px; border-radius: 4px; }");

    buttonLayout->addWidget(m_revokeBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(m_saveBtn);
    rightLayout->addLayout(buttonLayout);

    splitLayout->addWidget(leftPane, 4);
    splitLayout->addWidget(rightPane, 5);

    rootLayout->addLayout(splitLayout);

    setupPermissionMatrixUI();
    fetchStaffList();
  }
  return m_widget;
}


void StaffManager::executeIntent(const QString& intent, const QVariantMap& data) {
  if (m_core) m_core->log("[StaffManager] executeIntent: " + intent);

  // staff.create intent
  if (intent == "staff.create") {
    AddStaffDialog dialog(m_widget);
    if (dialog.exec() == QDialog::Accepted) {
      QVariantMap data;
      data["username"] = dialog.getUsername();
      data["password"] = dialog.getPassword();
      if (m_core) m_core->log("[StaffManager] Add new staff to backend...");
      m_core->httpPost("/api/v1/admin/staff", data, true, [this](QJsonDocument response) {
        if (!response.isNull() && !response.isEmpty()) {
          if (m_core) {
            // TODO: show it through toast notifications
            m_core->log("[StaffManager] Staff member created successfully");
          }
          fetchStaffList();
          setupPermissionMatrixUI();
          } else {
          if (m_core) m_core->log("[StaffManager] Failed to create staff.");
          }
      });
    } else {
      if (m_core) m_core->log("[StaffManager] New staff dialog closed.");
    }

    return;
  }

  // staff.nav_down
  if (intent == "staff.nav_down") {
    int currentRow = m_staffTable->currentRow();
    if (currentRow < m_staffTable->rowCount() - 1) {
      m_staffTable->selectRow(currentRow + 1);
    }
    return;
  }

  // staff.nav_up
  if (intent == "staff.nav_up") {
    int currentRow = m_staffTable->currentRow();
    if (currentRow > 0) {
      m_staffTable->selectRow(currentRow - 1);
    }
    return;
  }
}


void StaffManager::onMessage(const QString& topic, const QVariantMap& data) {
  if (m_core) m_core->log("Staff manager received a message on topic: " + topic);
  if (m_statusLabel) {
    QString msg = data.value("message", "no message").toString();
    m_statusLabel->setText("Status: Got message [" + msg + "] on [" + topic + "]");
  }
}


QVariant StaffManager::onServiceRequest(const QString& method, const QVariantMap& params) {
  if (m_core) m_core->log("StaffManager onServiceRequest: " + method);

  if (method == "status") {
    return QVariant(m_statusLabel ? m_statusLabel->text() : "no widget");
  }
  return QVariant();
}


void StaffManager::fetchStaffList() {
  if (!m_core) return;
  m_core->log("[StaffManager] Fetching staffs from the server...");

  m_core->httpGet("/api/v1/admin/staff", QVariantMap(), true, [this](QJsonDocument response) {
    if (response.isNull() || !response.isArray()) {
      if (m_core) m_core->log("[StaffManager] Error: failed to load staff list.");
      return;
    }

    QJsonArray staffArray = response.array();

    m_staffTable->setRowCount(0);

    auto createVisibleItem = [](const QString& text) {
      auto* item = new QTableWidgetItem(text);
      item->setForeground(QBrush(QColor("#212121")));
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      return item;
    };

    for (int i = 0; i < staffArray.size(); ++i) {
      QJsonObject staffObj = staffArray[i].toObject();
      QString id = QString::number(staffObj["id"].toInt());
      QString username = staffObj["username"].toString();
      QString role = staffObj["role"].toString().toUpper();

      m_staffTable->insertRow(i);
      m_staffTable->setItem(i, 0, createVisibleItem(id));
      m_staffTable->setItem(i, 1, createVisibleItem(username));
      m_staffTable->setItem(i, 2, createVisibleItem(role));

      auto* statusItem = createVisibleItem("ACTIVE");
      statusItem->setForeground(QBrush(QColor("#2E7D32")));
      m_staffTable->setItem(i, 3, statusItem);
    }

    if (m_staffTable->rowCount() > 0) {
      m_staffTable->selectRow(0);
    }
  });
}


void StaffManager::setupPermissionMatrixUI() {
  if (m_core) m_core->log("[StaffManager] Fetching intents from backend...");

  m_core->httpGet("/api/v1/system/intents", QVariantMap(), true, [this](QJsonDocument response) {
    if (response.isNull() || !response.isObject()) {
      if (m_core) m_core->log("[StaffManager] Error: failed to fetch intents from backend.");
      return;
    }
    m_permissionMatrixTable->setSortingEnabled(false);
    m_permissionMatrixTable->setRowCount(0);

    m_permissionMatrixTable->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; color: #212121; gridline-color: #E0E0E0; }"
      "QToolTip { color: #ffffff; background-color: #263238; border: 1px solid #BOBEC5; padding: 5px; font-size: 12px; font-weight: bold; border-radius: 4px; }"
    );

    m_permissionMatrixTable->horizontalHeader()->setStyleSheet(
      "QHeaderView::section { background-color: #ECEFF1; color: #1A237E; font-weight: 900; padding: 6px; border: 1px solid #CFD8DC; }"
    );

    auto createVisibleItem = [](const QString& text, bool isHeader = false) {
      auto* item = new QTableWidgetItem(text);
      if (isHeader) {
        item->setBackground(QBrush(QColor("#ECEFF1")));
        item->setForeground(QBrush(QColor("#1A237E")));
      } else {
        item->setForeground(QBrush(QColor("#212121")));
      }
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      return item;
    };

    QJsonObject root = response.object();
    QJsonObject intentsObj = root["protected_intents"].toObject();

    QStringList plugins = intentsObj.keys();

    int currentRow = 0;

    for (int i = 0; i < plugins.size(); ++i) {
      QString pluginName = plugins[i];
      m_permissionMatrixTable->insertRow(currentRow);
      QString headerText = pluginName.toUpper();
      m_permissionMatrixTable->setItem(currentRow, 0, createVisibleItem(headerText));
      m_permissionMatrixTable->setSpan(currentRow, 0, 1, 5);

      currentRow++;

      QJsonArray intentsArr = intentsObj[pluginName].toArray();

      for (const QJsonValue& val : intentsArr) {
        QString intent = val.toString().toLower();
        m_permissionMatrixTable->insertRow(currentRow);
        auto* intentItem = createVisibleItem("   ↳ " + intent);
        m_permissionMatrixTable->setItem(currentRow, 0, intentItem);

        for (int col = 1; col <= 4; ++col) {
          auto* container = new QWidget(m_permissionMatrixTable);
          auto* layout = new QHBoxLayout(container);
          auto* cb = new QCheckBox(container);

          cb->setToolTip(pluginName + "." + intent);

          cb->setStyleSheet("QCheckBox { background: transparent; }"
                          "QCheckBox::indicator { width: 12px; height: 12px; border: 2px solid #90A4AE; border-radius: 3px; background: white; }"
                          "QCheckBox::indicator:hover { border: 2px solid #1565C0; }"
                          "QCheckBox::indicator:checked { background-color: #1565C0; border: 2px solid #1565C0; }"
                          "QCheckBox::disabled {background-color: #f5f5f5; border: 2px solid #CFD8DC; }");


          cb->setProperty("plugin_name", pluginName);
          cb->setProperty("permission_name", intent);
          cb->setEnabled(true);

          layout->addWidget(cb);
          layout->setAlignment(Qt::AlignCenter);
          layout->setContentsMargins(0, 0, 0, 0);
          container->setLayout(layout);

          m_permissionMatrixTable->setCellWidget(currentRow, col, container);
        }
        currentRow++;
      }
    }

    if (m_core) m_core->log("[StaffManager] permission matrix ui built.");
  });
}
