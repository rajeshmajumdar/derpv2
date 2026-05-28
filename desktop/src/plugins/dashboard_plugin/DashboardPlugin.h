#ifndef DASHBOARDPLUGIN_H
#define DASHBOARDPLUGIN_H

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QEasingCurve>
#include <QPixmap>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include <QObject>
#include <QLabel>
#include <QWidget>
#include <QFrame>
#include <QVariantMap>
#include <QTimer>
#include "../../interfaces/DBaseModule.h"

#define CAROUSEL_TIMER 5000
#define AD_API_ENDPOINT "http://127.0.0.1:5000/ads"
#define ANIM_IN_DURATION 600
#define ANIM_OUT_DURATION 600
#define GRID_GAP 4
#define AD_BG "#1e1e1e"

class DashboardPlugin : public DBaseModule {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.derp.DModule" FILE "dashboard_plugin.json")
  Q_INTERFACES(DModule)

  public:
    explicit DashboardPlugin(QObject* parent = nullptr);
    ~DashboardPlugin();

    DCore* getCore() const override { return m_core; }

    void onInitialize() override;
    void onShutdown() override;
    QWidget* createView(QWidget* parent = nullptr) override;
    void handleIntent(const QString& intent, const QVariantMap& data) override;
    void onMessage(const QString& topic, const QVariantMap& data) override;
    QVariant onServiceRequest(const QString& method, const QVariantMap& params) override;

  protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

  private:
    QWidget* m_widget = nullptr;
    QFrame* m_adViewport = nullptr;
    QLabel* m_adLabelCurrent = nullptr;
    QLabel* m_adLabelNext = nullptr;
    QTimer* m_carouselTimer = nullptr;
    bool m_carouselRunning = false;
    bool m_isAnimating = false;

    QList<QPair<QString, QString>> m_adList;
    int m_currentAdIndex = 0;

    QWidget* createAdSection();
    QWidget* createSection(const QString& objectName);
    void fetchAdManifest();
    void processAd(const QString& url, const QString& link);
    void startCarousel();
    void showCurrentAd();
    void cycleToNextAd();
    void loadAdsFromCache();
    QSize adViewSize() const;
};

#endif
