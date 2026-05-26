#ifndef DUMMYPLUGIN_H
#define DUMMYPLUGIN_H

#include <QObject>
#include <QLabel>
#include <QWidget>
#include <QVariantMap>
#include "../../interfaces/DBaseModule.h"

class DummyPlugin : public DBaseModule {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "dummy_plugin.json")
  Q_INTERFACES(DModule)

  public:
    explicit DummyPlugin(QObject* parent = nullptr);
    ~DummyPlugin();

    void onInitialize() override;
    void onShutdown() override;
    QWidget* createView(QWidget* parent = nullptr) override;
    void executeIntent(const QString& intent, const QVariantMap& data) override;
    void onMessage(const QString& topic, const QVariantMap& data) override;
    QVariant onServiceRequest(const QString& method, const QVariantMap& params) override;

  private:
    QWidget* m_widget = nullptr;
    QLabel* m_statusLabel = nullptr;

    void handleDelete(const QVariantMap& data);
    void handleSave(const QVariantMap& data);
};

#endif
