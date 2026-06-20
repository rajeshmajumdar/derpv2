#ifndef INVENTORY_PLUGIN_H
#define INVENTORY_PLUGIN_H

#include "../../interfaces/DBaseModule.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class InventoryPlugin : public DBaseModule {
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "inventory_plugin.json")
	Q_INTERFACES(DModule)

	public:
		explicit InventoryPlugin(QObject *parent = nullptr);
		~InventoryPlugin() override;

		DCore *getCore() const override { return m_core; }

		void onInitialize() override;
		void onShutdown() override;
		QWidget *createView(QWidget *parent = nullptr) override;
		void handleIntent(const QString &intent, const QVariantMap &data) override;
		void onMessage(const QString &topic, const QVariantMap &data) override;
		QVariant onServiceRequest(const QString &method, const QVariantMap &params) override;

	private:
		QWidget *m_widget = nullptr;
};

#endif // INVENTORY_PLUGIN_H
	