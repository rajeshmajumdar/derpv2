#ifndef DMODULE_H
#define DMODULE_H

#include <QString>
#include <QVariantMap>
#include <QWidget>
#include "DCore.h"

class DModule {
  public:
    virtual ~DModule() {}

    virtual void initialize(DCore *core) = 0;
    virtual void onShutdown() = 0;

    virtual QWidget* createView(QWidget* parent = nullptr) = 0;

    virtual DCore* getCore() const = 0;

    void executeIntent(const QString &intent, const QVariantMap &data) {
      DCore* core = getCore();
      if (core) {
        if (!core->hasPermission(intent)) {
          core->log("[DModule] Blocked execution: " + intent);
          return;
        }
      }

      handleIntent(intent, data);
    }

    virtual void onMessage(const QString &topic, const QVariantMap &data) = 0;
    virtual QVariant onServiceRequest(const QString &method, const QVariantMap &params) = 0;

    virtual bool hasPrivateBinding(const QString& key) = 0;
    virtual QString getPrivateIntent(const QString& key) = 0;
    virtual bool hasPublicChord(const QString& chordSuffix) = 0;
    virtual QString getPublicIntent(const QString& chordSuffix) = 0;
    virtual bool hasPublicChordsConfigured() = 0;

  protected:
    virtual void handleIntent(const QString& intent, const QVariantMap &data) = 0;
};

Q_DECLARE_INTERFACE(DModule, "com.derp.DModule")

#endif // !DMODULE_H
