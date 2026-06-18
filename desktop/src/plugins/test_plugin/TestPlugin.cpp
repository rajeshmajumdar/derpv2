#include "TestPlugin.h"
#include <QWidget>

TestPlugin::TestPlugin(QObject *parent)
	: DBaseModule(":/test_plugin/manifest.json", parent) {}

TestPlugin::~TestPlugin() {
	if (m_widget && !m_widget->parent()) {
		delete m_widget;
	}
}

void TestPlugin::onInitialize() {
	if (m_core) m_core->log("[TestPlugin] Initialized.");
}

void TestPlugin::onShutdown() {
	if (m_core) m_core->log("[TestPlugin] Shutting down.");
}

QWidget *TestPlugin::createView(QWidget *parent) {
	if (!m_widget) {
		m_widget = new QWidget(parent);
	}
	return m_widget;
}

void TestPlugin::handleIntent(const QString &intent, const QVariantMap &data) {
	if (m_core) m_core->log("[TestPlugin] Intent received: " + intent);
}

void TestPlugin::onMessage(const QString &topic, const QVariantMap &data) {}

QVariant TestPlugin::onServiceRequest(const QString &method, const QVariantMap &params) {
	return QVariant();
}
	