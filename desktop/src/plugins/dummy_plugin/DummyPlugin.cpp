#include "DummyPlugin.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

PROTECTED_INTENT(dummy, delete)
PROTECTED_INTENT(dummy, save)

DummyPlugin::DummyPlugin(QObject* parent)
  : DBaseModule(":/dummy_plugin/manifest.json", parent) {}

DummyPlugin::~DummyPlugin() {
  if (m_widget && !m_widget->parent()) {
    delete m_widget;
  }
}

void DummyPlugin::onInitialize() {
  if (m_core) m_core->log("Dummy plugin initialized");
}

void DummyPlugin::onShutdown() {
  if (m_core) m_core->log("Dummy plugin shutting down");
}

QWidget* DummyPlugin::createView(QWidget* parent) {
  if (!m_widget) {
    m_widget = new QWidget(parent);
    auto* layout = new QVBoxLayout(m_widget);

    m_statusLabel = new QLabel("Status: idle", m_widget);
    layout->addWidget(m_statusLabel);

    auto* helpText = new QLabel(
      "Neovim-style hotkeys (Normal mode):\n"
      "  'd' → switch to this module\n"
      "  'dn' → new, 'ds' → save, 'f2' → print\n"
      "  'dd' → delete, 'fd' → search, 'cd' → close\n"
      "  ESC → cancel sequence / exit Insert mode\n"
      "\n"
      "Service: ping→pong, echo→returns params\n"
      "Listens to: dummy.test_topic",
      m_widget);
    helpText->setWordWrap(true);
    layout->addWidget(helpText);

    auto* btnNew = new QPushButton("New (n)", m_widget);
    connect(btnNew, &QPushButton::clicked, [this]() {
      executeIntent("dummy.new", QVariantMap());
    });
    layout->addWidget(btnNew);

    auto* btnSave = new QPushButton("Save (s)", m_widget);
    connect(btnSave, &QPushButton::clicked, [this]() {
      executeIntent("dummy.save", QVariantMap());
    });
    layout->addWidget(btnSave);

    auto* btnPrint = new QPushButton("Print (p)", m_widget);
    connect(btnPrint, &QPushButton::clicked, [this]() {
      executeIntent("dummy.print", QVariantMap());
    });
    layout->addWidget(btnPrint);

    auto* btnDelete = new QPushButton("Delete (d)", m_widget);
    connect(btnDelete, &QPushButton::clicked, [this]() {
      executeIntent("dummy.delete", QVariantMap());
    });
    layout->addWidget(btnDelete);

    auto* btnSearch = new QPushButton("Search (f)", m_widget);
    connect(btnSearch, &QPushButton::clicked, [this]() {
      executeIntent("dummy.search", QVariantMap());
    });
    layout->addWidget(btnSearch);

    auto* btnClose = new QPushButton("Close (c)", m_widget);
    connect(btnClose, &QPushButton::clicked, [this]() {
      executeIntent("dummy.close", QVariantMap());
    });
    layout->addWidget(btnClose);

    auto* btnPublish = new QPushButton("Publish Test Message", m_widget);
    connect(btnPublish, &QPushButton::clicked, [this]() {
      if (m_core) {
        QVariantMap data;
        data["from"] = "dummy";
        data["message"] = "hello from dummy plugin";
        m_core->publish("dummy.test_topic", data);
        m_statusLabel->setText("Status: Published test message");
      }
    });
    layout->addWidget(btnPublish);

    auto* btnPing = new QPushButton("Call Service: ping", m_widget);
    connect(btnPing, &QPushButton::clicked, [this]() {
      if (m_core) {
        QVariant result = m_core->callService("dummy", "ping", QVariantMap());
        m_statusLabel->setText("Status: ping → " + result.toString());
      }
    });
    layout->addWidget(btnPing);

    auto* btnLaunch = new QPushButton("Launch Intent on Self", m_widget);
    connect(btnLaunch, &QPushButton::clicked, [this]() {
      if (m_core) {
        m_core->launchIntent("dummy", "dummy.new", QVariantMap());
      }
    });
    layout->addWidget(btnLaunch);

    layout->addStretch();
  }
  return m_widget;
}

void DummyPlugin::executeIntent(const QString& intent, const QVariantMap& data) {
  if (m_core) m_core->log("Dummy executeIntent: " + intent);
  if (m_statusLabel) {
    m_statusLabel->setText("Status: Intent [" + intent + "] fired");
  }
  if (intent == "dummy.delete") {
    handleDelete(data);
  } else if (intent == "dummy.save") {
    handleSave(data);
  }
}

void DummyPlugin::onMessage(const QString& topic, const QVariantMap& data) {
  if (m_core) m_core->log("Dummy received message on topic: " + topic);
  if (m_statusLabel) {
    QString msg = data.value("message", "no message").toString();
    m_statusLabel->setText("Status: Got message [" + msg + "] on [" + topic + "]");
  }
}

QVariant DummyPlugin::onServiceRequest(const QString& method, const QVariantMap& params) {
  if (m_core) m_core->log("Dummy onServiceRequest: " + method);

  if (method == "ping") {
    return QVariant("pong");
  }
  if (method == "echo") {
    return params;
  }
  if (method == "status") {
    return QVariant(m_statusLabel ? m_statusLabel->text() : "no widget");
  }
  return QVariant();
}


void DummyPlugin::handleDelete(const QVariantMap& data) {
  if (m_core) m_core->log("Executing protected DELETE operation in background...");
  if (m_statusLabel) m_statusLabel->setText("Protected delete executed.");
}


void DummyPlugin::handleSave(const QVariantMap& data) {
  if (m_core) m_core->log("Executing protected SAVE operation in background...");
  if (m_statusLabel) m_statusLabel->setText("Protected save executed.");
}
