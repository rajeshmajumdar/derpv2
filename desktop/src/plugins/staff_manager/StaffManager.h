#ifndef STAFFMANAGER_H
#define STAFFMANAGER_H

#include <QObject>
#include <QLabel>
#include <QWidget>
#include <QVariantMap>
#include "../../interfaces/DBaseModule.h"

class QTableWidget;
class QLineEdit;
class QPushButton;
class QLabel;

class StaffManager : public DBaseModule {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "staff_manager.json")
  Q_INTERFACES(DModule)

  public:
    explicit StaffManager(QObject* parent = nullptr);
    ~StaffManager() override;

    void onInitialize() override;
    void onShutdown() override;
    QWidget* createView(QWidget* parent = nullptr) override;
    void executeIntent(const QString& intent, const QVariantMap& data) override;
    void onMessage(const QString& topic, const QVariantMap& data) override;
    QVariant onServiceRequest(const QString& method, const QVariantMap& params) override;

    void initializeMockData();

  private:
    QWidget* m_widget = nullptr;
    QLabel* m_statusLabel = nullptr;

    QTableWidget* m_staffTable = nullptr;
    QLineEdit* m_searchBar = nullptr;

    QLabel* m_activeUserLabel = nullptr;
    QTableWidget* m_permissionMatrixTable = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_revokeBtn = nullptr;

    void fetchStaffList();
    void setupPermissionMatrixUI();
};

#endif // !STAFFMANAGER_H
