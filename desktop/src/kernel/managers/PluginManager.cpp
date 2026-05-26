#include "PluginManager.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

PluginManager::PluginManager(DCore* core, QObject *parent)
  : QObject(parent), m_core(core), m_activeModuleId("") {}


void PluginManager::loadModules(const QString &pluginsPath, const QString &currentRole) {
  QDir dir(pluginsPath);
  if (!dir.exists()) {
    if (m_core) m_core->log("Plugin directory not found: " + pluginsPath);
    return;
  }

  m_registry.clear();
  m_globalSwitchMap.clear();

  for (const QString &subDir : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QDir pluginDir(dir.absoluteFilePath(subDir));
    QFile manifestFile(pluginDir.filePath("manifest.json"));

    if (!manifestFile.open(QIODevice::ReadOnly)) continue;

    QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
    QJsonObject manifest = doc.object();
    QString moduleId = manifest["id"].toString();

    QString requiredRole = manifest["role"].toString().toLower();

    if (!requiredRole.isEmpty() && requiredRole != currentRole.toLower()) {
      m_core->log("Skipping " + moduleId + ": Requires " + requiredRole + " role.");
      continue;
    }

    QStringList filters; filters << "*.dll" << "*.so";
    QStringList dllFiles = pluginDir.entryList(filters);

    if (dllFiles.isEmpty()) {
      m_core->log("No library found in " + subDir);
      continue;
    }

    QPluginLoader* loader = new QPluginLoader(pluginDir.absoluteFilePath(dllFiles.first()));
    QObject* pluginInstance = loader->instance();

    if (DModule* module = qobject_cast<DModule*>(pluginInstance)) {
      ModuleRecord* record = new ModuleRecord();
      record->instance = module;
      record->loader = loader;
      record->manifest = manifest;
      record->view = nullptr; // TODO: To be populated when main ui wires views

      m_registry[moduleId] = record;

      pluginInstance->setProperty("manifest_path", manifestFile.fileName());

      module->initialize(m_core);

      m_core->log("Successfully loaded: " + moduleId);
    } else {
      m_core->log("Failed to cast plugin: " + loader->errorString());
      delete loader;
    }
  }

  buildRoutingTables();
}

ModuleRecord* PluginManager::getModule(const QString &id) {
  return m_registry.value(id, nullptr);
}

DModule* PluginManager::getModuleInstance(const QString &id) {
  ModuleRecord* rec = m_registry.value(id, nullptr);
  return rec ? rec->instance : nullptr;
}

QString PluginManager::findModuleByGlobalSwitch(const QString &globalSwitch) {
  return m_globalSwitchMap.value(globalSwitch.toLower().trimmed(), QString());
}

void PluginManager::buildRoutingTables() {
  m_globalSwitchMap.clear();
  m_hotkeyRegistry.clear();

  for (auto it = m_registry.begin(); it != m_registry.end(); ++it) {
    const QString& moduleId = it.key();
    const QJsonObject& manifest = it.value()->manifest;

    QString globalSwitchKey = manifest["hotkey"].toString().toLower().trimmed();

    if (!globalSwitchKey.isEmpty()) {
      m_globalSwitchMap[globalSwitchKey] = moduleId;
      if (m_core) {
        m_core->log(QString("[PluginManager] Key '%1' registered to module: %2").arg(globalSwitchKey, moduleId));
      }

      // Register intents from manifest for the command palette
      QJsonObject intents = manifest["intents"].toObject();
      for (auto iit = intents.begin(); iit != intents.end(); ++iit) {
        QString combo = globalSwitchKey + iit.key();
        KeyBindingTarget target;
        target.moduleId = moduleId;
        target.intentName = iit.value().toString();
        m_hotkeyRegistry[combo] = target;
      }

      // Register chord intents for the command palette
      QJsonObject chords = manifest["chord"].toObject();
      for (auto cit = chords.begin(); cit != chords.end(); ++cit) {
        QString combo = globalSwitchKey + cit.key();
        QJsonObject chordObj = cit.value().toObject();
        KeyBindingTarget target;
        target.moduleId = moduleId;
        target.intentName = chordObj["intent"].toString();
        m_hotkeyRegistry[combo] = target;
      }
    }
  }
}
