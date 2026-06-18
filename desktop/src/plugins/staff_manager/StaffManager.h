#ifndef STAFFMANAGER_H
#define STAFFMANAGER_H

#include "../../interfaces/DBaseModule.h"
#include <QLabel>
#include <QObject>
#include <QVariantMap>
#include <QWidget>

class QTableWidget;
class QLineEdit;
class QPushButton;
class QLabel;

class StaffManager : public DBaseModule {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "staff_manager.json")
  Q_INTERFACES(DModule)

public:
  explicit StaffManager(QObject *parent = nullptr);
  ~StaffManager() override;

  DCore *getCore() const override { return m_core; }

  void onInitialize() override;
  void onShutdown() override;
  QWidget *createView(QWidget *parent = nullptr) override;
  void handleIntent(const QString &intent, const QVariantMap &data) override;
  void onMessage(const QString &topic, const QVariantMap &data) override;
  QVariant onServiceRequest(const QString &method,
                            const QVariantMap &params) override;

private slots:
  void loadPermissionsForSelectedStaff();

private:
  QWidget *m_widget = nullptr;
  QLabel *m_statusLabel = nullptr;

  QTableWidget *m_staffTable = nullptr;
  QLineEdit *m_searchBar = nullptr;

  QLabel *m_activeUserLabel = nullptr;
  QTableWidget *m_permissionMatrixTable = nullptr;
  QPushButton *m_saveBtn = nullptr;
  QPushButton *m_revokeBtn = nullptr;

  void fetchStaffList();
  void setupPermissionMatrixUI();
  void commitPermissions();
  void handleSearch();
};

#endif // !STAFFMANAGER_H
