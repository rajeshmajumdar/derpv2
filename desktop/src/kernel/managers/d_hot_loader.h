/*
   A single header Qt plugin hot-reloader for windows

   how to use:
   1. include this header anywhere you need to reference this class.
   2. in exactly one .cpp file, define D_HOT_LOADER_IMPLEMENTATION before
   including.
*/

#ifndef D_HOT_LOADER_H
#define D_HOT_LOADER_H

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QObject>
#include <QPluginLoader>
#include <QString>
#include <QThread>
#include <functional>

class DHotLoader : public QObject {

public:
  explicit DHotLoader(QObject *parent = nullptr);
  ~DHotLoader();

  // loads a dll, creates a shadow copy and returns the instance
  bool load(const QString &origDllPath);

  // unloads the current instance and deletes the shadow copy.
  void unload();

  // returns the active plugin instance
  QObject *instance() const;

  // replaces qt signals with standard cpp callbacks
  std::function<void(QObject *)> onPluginReload;
  std::function<void(QString)> onPluginReloadFailed;

private:
  QString m_originalPath;
  QString m_currentShadowPath;
  QPluginLoader *m_loader;
  QFileSystemWatcher *m_watcher;

  QString generateShadowPath(const QString &originalPath);
  void cleanupOldShadows(const QString &originalPath);
  void handleDllChanged(const QString &path);
};

#endif // !D_HOT_LOADER_H

// IMPLEMENTATION
#ifdef D_HOT_LOADER_IMPLEMENTATION

DHotLoader::DHotLoader(QObject *parent)
    : QObject(parent), m_loader(nullptr),
      m_watcher(new QFileSystemWatcher(this)) {
  connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
          [this](const QString &path) { this->handleDllChanged(path); });
}

DHotLoader::~DHotLoader() { unload(); }

bool DHotLoader::load(const QString &origDllPath) {
  m_originalPath = origDllPath;
  QFileInfo fileInfo(m_originalPath);

  if (!fileInfo.exists()) {
    qWarning() << "[DHotLoader] Target DLL does not exists:" << m_originalPath;
    return false;
  }

  cleanupOldShadows(m_originalPath);

  m_currentShadowPath = generateShadowPath(m_originalPath);

  if (!QFile::copy(m_originalPath, m_currentShadowPath)) {
    qWarning() << "[DHotLoader] Failed to create shadow copy at:"
               << m_currentShadowPath;
    return false;
  }

  m_loader = new QPluginLoader(m_currentShadowPath, this);

  if (!m_loader->load()) {
    qWarning() << "[DHotLoader] Failed to load plugin:"
               << m_loader->errorString();
    QFile::remove(m_currentShadowPath);
    delete m_loader;
    m_loader = nullptr;
    return false;
  }

  if (!m_watcher->files().contains(m_originalPath)) {
    m_watcher->addPath(m_originalPath);
  }

  qDebug() << "[DHotLoader] Successfully mounted shadow instance:"
           << m_currentShadowPath;
  return true;
}

void DHotLoader::unload() {
  if (m_loader) {
    m_loader->unload();
    delete m_loader;
    m_loader = nullptr;
  }

  if (!m_currentShadowPath.isEmpty()) {
    QFile::remove(m_currentShadowPath);
    m_currentShadowPath.clear();
  }
}

QObject *DHotLoader::instance() const {
  if (m_loader && m_loader->isLoaded()) {
    return m_loader->instance();
  }
  return nullptr;
}

void DHotLoader::handleDllChanged(const QString &path) {
  if (path != m_originalPath)
    return;
  qDebug() << "[DHotLoader] File change detected! Initiating hot-swap for:"
           << path;

  // small delay
  QThread::msleep(100);

  unload();

  if (load(m_originalPath)) {
    if (onPluginReload)
      onPluginReload(instance());
  } else {
    if (onPluginReloadFailed)
      onPluginReloadFailed("Failed to hot-swap plugin.");
  }
}

QString DHotLoader::generateShadowPath(const QString &originalPath) {
  QFileInfo info(originalPath);
  QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());

  return info.absolutePath() + "/" + info.baseName() + "_shadow_" + timestamp +
         "." + info.suffix();
}

void DHotLoader::cleanupOldShadows(const QString &originalPath) {
  QFileInfo info(originalPath);
  QDir dir = info.absoluteDir();
  QString base = info.baseName() + "_shadow_";

  for (const QFileInfo &oldFile :
       dir.entryInfoList({base + "*.dll"}, QDir::Files)) {
    QFile::remove(oldFile.absoluteFilePath());
  }
}

#endif
