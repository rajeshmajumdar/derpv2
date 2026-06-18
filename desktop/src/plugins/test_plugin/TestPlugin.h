#ifndef TEST_PLUGIN_H
#define TEST_PLUGIN_H

#include "../../interfaces/DBaseModule.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class TestPlugin : public DBaseModule {
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "test_plugin.json")
	Q_INTERFACES(DModule)

	public:
		explicit TestPlugin(QObject *parent = nullptr);
		~TestPlugin() override;

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

#endif // TEST_PLUGIN_H
	