#ifndef DBASEMODULE_H
#define DBASEMODULE_H

#include "DModule.h"
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>

class DBaseModule : public QObject, public DModule {
  Q_OBJECT
  Q_INTERFACES(DModule)

public:
  explicit DBaseModule(const QString &manifestPath, QObject *parent = nullptr)
      : QObject(parent), m_manifestPath(manifestPath) {}

  virtual ~DBaseModule() override = default;

  void initialize(DCore *core) override {
    m_core = core;
    loadManifestBindings();
    onInitialize();
  }

  bool hasPrivateBinding(const QString &key) {
    return m_privateBindingsMap.contains(key.toLower().trimmed());
  }

  QString getPrivateIntent(const QString &key) {
    return m_privateBindingsMap.value(key.toLower().trimmed(), QString());
  }

  bool hasPublicChord(const QString &chordSuffix) {
    return m_publicChordsMap.contains(chordSuffix.toLower().trimmed());
  }

  QString getPublicIntent(const QString &chordSuffix) {
    return m_publicChordsMap.value(chordSuffix.toLower().trimmed(), QString());
  }

  bool hasPublicChordsConfigured() { return !m_publicChordsMap.isEmpty(); }

  void registerProtectedIntent(const QString &intentName) {
    s_protectedIntents.insert(intentName);
  }

  QSet<QString> getProtectedIntents() { return s_protectedIntents; }

  QMap<QString, QString> getIntentDescription() const override {
    return m_intentDescription;
  }

protected:
  virtual void onInitialize() {}

  DCore *m_core = nullptr;
  QString m_manifestPath;

  QHash<QString, QString> m_privateBindingsMap;
  QHash<QString, QString> m_publicChordsMap;
  QMap<QString, QString> m_intentDescription;

  QSet<QString> s_protectedIntents;

private:
  void loadManifestBindings() {
    m_privateBindingsMap.clear();
    m_publicChordsMap.clear();
    m_intentDescription.clear();

    QString finalPath = this->property("manifest_path").toString();
    if (finalPath.isEmpty()) {
      finalPath = m_manifestPath;
    }

    QFile manifestFile(finalPath);
    if (!manifestFile.open(QIODevice::ReadOnly)) {
      if (m_core)
        m_core->log(
            "[DBaseModule] Error: Unable to locate or open manifest.json");
      return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
    QJsonObject manifestObj = doc.object();

    QJsonObject privateSection = manifestObj["private"].toObject();
    for (auto it = privateSection.begin(); it != privateSection.end(); ++it) {
      QString keyLabel = it.key().toLower().trimmed();
      QString targetIntent = it.value().toString();
      m_privateBindingsMap[keyLabel] = targetIntent;

      if (m_core)
        m_core->log(QString("[DBaseModule] Registered '%1' -> %2")
                        .arg(keyLabel, targetIntent));
    }

    QJsonObject chordSection = manifestObj["chord"].toObject();
    for (auto it = chordSection.begin(); it != chordSection.end(); ++it) {
      QString chordSuffix = it.key().toLower().trimmed();
      QJsonObject chordDetails = it.value().toObject();
      QString targetIntent = chordDetails["intent"].toString();
      m_publicChordsMap[chordSuffix] = targetIntent;

      if (m_core)
        m_core->log(QString("[DBaseModule] Registered '%1' -> %2")
                        .arg(chordSuffix, targetIntent));
    }

    QJsonObject permissionsSection = manifestObj["permissions"].toObject();
    for (auto it = permissionsSection.begin(); it != permissionsSection.end();
         ++it) {
      QString actionName = it.key().toLower().trimmed();
      QString description = it.value().toString();
      m_intentDescription[actionName] = description;
    }
  }
};

#define PROTECTED_INTENT(plugin, action)                                       \
  this->registerProtectedIntent(#plugin "." #action);

#endif // !DBASEMODULE_H
