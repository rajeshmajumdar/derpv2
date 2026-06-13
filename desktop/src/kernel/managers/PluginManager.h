#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QPluginLoader>
#include <QStringList>

#include "../../interfaces/DCore.h"
#include "../../interfaces/DModule.h"
#include "d_hot_loader.h"

class QWidget;

struct ModuleRecord {
  DModule *instance;
  DHotLoader *loader;
  QJsonObject manifest;
  QWidget *view = nullptr;
};

struct KeyBindingTarget {
  QString moduleId;
  QString intentName;
};

class PluginManager : public QObject {
  Q_OBJECT

public:
  explicit PluginManager(DCore *core, QObject *parent = nullptr);

  void loadModules(const QString &pluginsPath, const QString &currentRole);

  DModule *getModuleInstance(const QString &id);
  QString findModuleByGlobalSwitch(const QString &globalSwitch);
  QString activeModuleId() const { return m_activeModuleId; }

  void setActiveModuleId(const QString &id) { m_activeModuleId = id; }

  ModuleRecord *getModule(const QString &id);
  const QMap<QString, ModuleRecord *> &allModules() const { return m_registry; }

  const QHash<QString, KeyBindingTarget> &hotkeyRegistry() const {
    return m_hotkeyRegistry;
  }

signals:
  void pluginHotReloaded(const QString &moduleId);

private:
  void buildRoutingTables();

  DCore *m_core;
  QString m_activeModuleId;
  QMap<QString, ModuleRecord *> m_registry;
  QHash<QString, QString> m_globalSwitchMap;
  QHash<QString, KeyBindingTarget> m_hotkeyRegistry;
};

#endif // PLUGINMANAGER_H
