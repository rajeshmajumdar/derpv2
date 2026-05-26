#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QJsonObject>
#include <QStringList>
#include <QPluginLoader>

#include "../../interfaces/DModule.h"
#include "../../interfaces/DCore.h"

class QWidget;

struct ModuleRecord {
  DModule* instance;
  QPluginLoader* loader;
  QJsonObject manifest;
  QWidget* view = nullptr;
};

struct KeyBindingTarget {
  QString moduleId;
  QString intentName;
};

class PluginManager : public QObject {
  Q_OBJECT

public:
  explicit PluginManager(DCore* core, QObject *parent = nullptr);

  void loadModules(const QString &pluginsPath, const QString &currentRole);

  DModule* getModuleInstance(const QString &id);
  QString findModuleByGlobalSwitch(const QString &globalSwitch);
  QString activeModuleId() const { return m_activeModuleId; }

  void setActiveModuleId(const QString &id) { m_activeModuleId = id; }

  ModuleRecord* getModule(const QString &id);
  const QMap<QString, ModuleRecord*>& allModules() const { return m_registry; }

  const QHash<QString, KeyBindingTarget>& hotkeyRegistry() const { return m_hotkeyRegistry; }

private:
  void buildRoutingTables();

  DCore* m_core;
  QString m_activeModuleId;
  QMap<QString, ModuleRecord*> m_registry;
  QHash<QString, QString> m_globalSwitchMap;
  QHash<QString, KeyBindingTarget> m_hotkeyRegistry;
};

#endif // PLUGINMANAGER_H
