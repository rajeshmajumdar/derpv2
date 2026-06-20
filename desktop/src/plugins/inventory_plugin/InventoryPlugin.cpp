#include "InventoryPlugin.h"
#include <QWidget>

InventoryPlugin::InventoryPlugin(QObject *parent)
	: DBaseModule(":/inventory_plugin/manifest.json", parent) {}

InventoryPlugin::~InventoryPlugin() {
	if (m_widget && !m_widget->parent()) {
		delete m_widget;
	}
}

void InventoryPlugin::onInitialize() {
	if (m_core) m_core->log("[InventoryPlugin] Initialized.");
}

void InventoryPlugin::onShutdown() {
	if (m_core) m_core->log("[InventoryPlugin] Shutting down.");
}

QWidget *InventoryPlugin::createView(QWidget *parent) {
	if (!m_widget) {
		m_widget = new QWidget(parent);
	}
	return m_widget;
}

void InventoryPlugin::handleIntent(const QString &intent, const QVariantMap &data) {
	if (m_core) m_core->log("[InventoryPlugin] Intent received: " + intent);
}

void InventoryPlugin::onMessage(const QString &topic, const QVariantMap &data) {}

QVariant InventoryPlugin::onServiceRequest(const QString &method, const QVariantMap &params) {
	return QVariant();
}
	