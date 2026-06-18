#include "LedgerPlugin.h"
#include <qcontainerfwd.h>

LedgerPlugin::LedgerPlugin(QObject *parent)
    : DBaseModule(":/ledger_plugin/manifest.json", parent) {}

LedgerPlugin::~LedgerPlugin() {
  if (m_widget && !m_widget->parent()) {
    delete m_widget;
  }
}

void LedgerPlugin::onInitialize() {
  if (m_core)
    m_core->log("[LedgerPlugin] initialized");
}

void LedgerPlugin::onShutdown() {
  if (m_core)
    m_core->log("[LedgerPlugin] shutting down");
}

QWidget *LedgerPlugin::createView(QWidget *parent) {
  if (!m_widget) {
    m_widget = new QWidget(parent);
  }
  return m_widget;
}

void LedgerPlugin::handleIntent(const QString &intent,
                                const QVariantMap &data) {
  if (m_core)
    m_core->log("[LedgerPlugin] executeIntent: " + intent);
}

void LedgerPlugin::onMessage(const QString &topic, const QVariantMap &data) {
  if (m_core)
    m_core->log("[LedgerPlugin] Received message on topic: " + topic);
}

QVariant LedgerPlugin::onServiceRequest(const QString &method,
                                        const QVariantMap &params) {
  if (m_core)
    m_core->log("[LedgerPlugin] onServiceRequest: " + method);
  return QVariant();
}
