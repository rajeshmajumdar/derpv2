#include "DashboardPlugin.h"

DashboardPlugin::DashboardPlugin(QObject* parent)
  : DBaseModule(":/dashboard_plugin/manifest.json", parent) {}

DashboardPlugin::~DashboardPlugin() {
  if (m_widget && !m_widget->parent()) {
    delete m_widget;
  }
}

void DashboardPlugin::onInitialize() {
  if (m_core) m_core->log("Dashboard plugin loaded");

  QString cachePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache/ads";
  QDir().mkpath(cachePath);
}

void DashboardPlugin::onShutdown() {
  if (m_carouselTimer) {
    m_carouselTimer->stop();
  }
  if (m_core) m_core->log("Dashboard plugin shutting down");
}

QWidget* DashboardPlugin::createView(QWidget* parent) {
  if (!m_widget) {
    m_widget = new QWidget(parent);
    auto* grid = new QGridLayout(m_widget);
    grid->setContentsMargins(GRID_GAP, GRID_GAP, GRID_GAP, GRID_GAP);
    grid->setSpacing(GRID_GAP);

    grid->setColumnStretch(0, 8);
    grid->setColumnStretch(1, 2);
    grid->setRowStretch(0, 7);
    grid->setRowStretch(1, 3);

    grid->addWidget(createAdSection(), 0, 0);
    grid->addWidget(createSection("orderSection"), 0, 1);

    QWidget* bottomLeft = new QWidget();
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomLeft);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(GRID_GAP);
    bottomLayout->addWidget(createSection("lastPlacedSection"), 1);
    bottomLayout->addWidget(createSection("runningOfferSection"), 1);
    bottomLayout->addWidget(createSection("supplierOfferSection"), 1);
    grid->addWidget(bottomLeft, 1, 0);

    grid->addWidget(createSection("financialSection"), 1, 1);

    QTimer::singleShot(0, this, [this]() { fetchAdManifest(); });
  }
  return m_widget;
}

void DashboardPlugin::executeIntent(const QString& intent, const QVariantMap& data) {
  Q_UNUSED(data);
  if (intent == "open_ad_link" && !m_adList.isEmpty() && m_currentAdIndex < m_adList.size()) {
    QString link = m_adList[m_currentAdIndex].second;
    if (!link.isEmpty()) {
      QDesktopServices::openUrl(QUrl(link));
    }
  }
}

void DashboardPlugin::onMessage(const QString& topic, const QVariantMap& data) {
  if (m_core) m_core->log("Dashboard received a message: " + topic);
}

QVariant DashboardPlugin::onServiceRequest(const QString& method, const QVariantMap& params) {
  if (m_core) m_core->log("Dashboard requested service: " + method);
  return QVariant("received");
}

QWidget* DashboardPlugin::createAdSection() {
  QFrame* container = new QFrame();
  container->setObjectName("adSection");

  QVBoxLayout* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);

  m_adViewport = new QFrame(container);
  m_adViewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_adViewport->setMinimumSize(200, 100);
  m_adViewport->setStyleSheet(QString("background-color: %1;").arg(AD_BG));
  layout->addWidget(m_adViewport);

  m_adLabelCurrent = new QLabel(m_adViewport);
  m_adLabelCurrent->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  m_adLabelCurrent->setAlignment(Qt::AlignCenter);
  m_adLabelCurrent->setScaledContents(false);
  m_adLabelCurrent->setStyleSheet(QString("background-color: %1;").arg(AD_BG));
  m_adLabelCurrent->setCursor(Qt::PointingHandCursor);
  m_adLabelCurrent->installEventFilter(this);

  m_adLabelNext = new QLabel(m_adViewport);
  m_adLabelNext->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  m_adLabelNext->setAlignment(Qt::AlignCenter);
  m_adLabelNext->setScaledContents(false);
  m_adLabelNext->setStyleSheet(QString("background-color: %1;").arg(AD_BG));
  m_adLabelNext->hide();

  return container;
}

QWidget* DashboardPlugin::createSection(const QString& objectName) {
  QWidget* w = new QWidget();
  w->setObjectName(objectName);
  w->setMinimumSize(60, 40);
  w->setStyleSheet("background-color: #d1d2d3; border-radius: 4px;");
  return w;
}

bool DashboardPlugin::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_adLabelCurrent) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
      if (mouseEvent->button() == Qt::LeftButton) {
        if (!m_adList.isEmpty() && m_currentAdIndex < m_adList.size()) {
          QString link = m_adList[m_currentAdIndex].second;
          if (!link.isEmpty()) {
            if (m_core) m_core->log("Ad clicked: " + link);
            QDesktopServices::openUrl(QUrl(link));
          }
        }
        return true;
      }
    }
  }
  return QObject::eventFilter(obj, event);
}

void DashboardPlugin::fetchAdManifest() {
  if (!m_core) {
    loadAdsFromCache();
    return;
  }

  m_core->httpGet(AD_API_ENDPOINT, {}, false, [this](QJsonDocument doc) {
    if (doc.isNull() || !doc.isArray()) {
      if (m_core) m_core->log("[Dashboard] Server offline — using cache");
      loadAdsFromCache();
      return;
    }

    QJsonArray ads = doc.array();
    for (const QJsonValue& value : ads) {
      QJsonObject obj = value.toObject();
      QString imgUrl = obj["image"].toString();
      QString link = obj["link"].toString();
      if (!imgUrl.isEmpty()) {
        processAd(imgUrl, link);
      }
    }

    if (!m_adList.isEmpty()) {
      startCarousel();
    } else {
      loadAdsFromCache();
    }
  });
}

void DashboardPlugin::processAd(const QString& url, const QString& link) {
  QString fileName = url.section('/', -1);
  if (fileName.isEmpty()) return;

  QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache/ads/";
  QString localPath = cacheDir + fileName;

  if (QFile::exists(localPath)) {
    m_adList.append({localPath, link});
    if (!m_carouselRunning && !m_adList.isEmpty()) {
      startCarousel();
    }
  } else if (m_core) {
    m_core->httpDownload(url, localPath, [this, localPath, link](bool success) {
      if (success) {
        m_adList.append({localPath, link});
        if (!m_carouselRunning && !m_adList.isEmpty()) {
          startCarousel();
        }
      }
    });
  }
}

void DashboardPlugin::startCarousel() {
  if (m_adList.isEmpty()) return;
  if (m_carouselRunning) return;

  m_carouselRunning = true;
  m_currentAdIndex = 0;

  QTimer::singleShot(50, this, [this]() {
    showCurrentAd();
  });

  if (!m_carouselTimer) {
    m_carouselTimer = new QTimer(this);
    connect(m_carouselTimer, &QTimer::timeout, this, &DashboardPlugin::cycleToNextAd);
  }
  m_carouselTimer->start(CAROUSEL_TIMER);
}

QSize DashboardPlugin::adViewSize() const {
  QSize s = m_adViewport->size();
  if (s.width() > 0 && s.height() > 0) return s;
  return QSize(800, 400);
}

void DashboardPlugin::showCurrentAd() {
  if (!m_adLabelCurrent || !m_adViewport || m_adList.isEmpty()) return;

  QSize vs = adViewSize();
  m_adLabelCurrent->resize(vs);
  m_adLabelCurrent->move(0, 0);

  QString path = m_adList[m_currentAdIndex].first;
  QPixmap pix(path);
  if (!pix.isNull()) {
    QPixmap scaled = pix.scaled(vs, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_adLabelCurrent->setPixmap(scaled);
  }

  m_adLabelCurrent->show();
  m_adLabelNext->hide();
}

void DashboardPlugin::cycleToNextAd() {
  if (m_adList.isEmpty() || m_isAnimating) return;

  m_isAnimating = true;
  int nextIndex = (m_currentAdIndex + 1) % m_adList.size();
  QSize vs = adViewSize();

  QPixmap nextPix(m_adList[nextIndex].first);
  if (nextPix.isNull()) {
    m_currentAdIndex = nextIndex;
    m_isAnimating = false;
    return;
  }

  m_adLabelNext->resize(vs);
  m_adLabelNext->move(vs.width(), 0);
  m_adLabelNext->setPixmap(nextPix.scaled(vs, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
  m_adLabelNext->show();

  QPropertyAnimation* animOut = new QPropertyAnimation(m_adLabelCurrent, "pos");
  animOut->setDuration(ANIM_OUT_DURATION);
  animOut->setStartValue(QPoint(0, 0));
  animOut->setEndValue(QPoint(-vs.width(), 0));
  animOut->setEasingCurve(QEasingCurve::InOutQuint);

  QPropertyAnimation* animIn = new QPropertyAnimation(m_adLabelNext, "pos");
  animIn->setDuration(ANIM_IN_DURATION);
  animIn->setStartValue(QPoint(vs.width(), 0));
  animIn->setEndValue(QPoint(0, 0));
  animIn->setEasingCurve(QEasingCurve::InOutQuint);

  QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
  group->addAnimation(animOut);
  group->addAnimation(animIn);

  connect(group, &QParallelAnimationGroup::finished, [this, nextIndex, group]() {
    m_currentAdIndex = nextIndex;
    m_adLabelCurrent->setPixmap(m_adLabelNext->pixmap());
    m_adLabelCurrent->move(0, 0);
    m_adLabelCurrent->resize(m_adViewport->size());
    m_adLabelNext->hide();
    m_isAnimating = false;
    group->deleteLater();
  });

  group->start();
}

void DashboardPlugin::loadAdsFromCache() {
  if (m_carouselRunning) return;

  QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache/ads/";
  QDir dir(cacheDir);
  if (!dir.exists()) return;

  QStringList files = dir.entryList(QDir::Files);
  for (const QString& file : files) {
    m_adList.append({cacheDir + file, "https://durga.care/"});
  }

  if (!m_adList.isEmpty()) {
    startCarousel();
  }
}
