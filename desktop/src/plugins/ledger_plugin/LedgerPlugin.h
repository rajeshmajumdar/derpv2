#ifndef LEDGERPLUGIN_H
#define LEDGERPLUGIN_H

#include "../../interfaces/DBaseModule.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class LedgerPlugin : public DBaseModule {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "ledger_plugin.json")
  Q_INTERFACES(DModule)

public:
  explicit LedgerPlugin(QObject *parent = nullptr);
  ~LedgerPlugin() override;

  DCore *getCore() const override { return m_core; }

  void onInitialize() override;
  void onShutdown() override;
  QWidget *createView(QWidget *parent = nullptr) override;
  void handleIntent(const QString &intent, const QVariantMap &data) override;
  void onMessage(const QString &topic, const QVariantMap &data) override;
  QVariant onServiceRequest(const QString &method,
                            const QVariantMap &params) override;

private:
  QWidget *m_widget = nullptr;
};

#endif // !LEDGERPLUGIN_H
