#include "UIManager.h"
#include <QVBoxLayout>
#include <QPixmap>
#include <QStyle>

UIManager::UIManager(QMainWindow* window) : QObject(window), m_window(window) {}


void UIManager::setupShell() {
  m_window->setMinimumSize(0, 0);
  m_window->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

  QWidget* central = new QWidget(m_window);
  central->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QVBoxLayout* mainLayout = new QVBoxLayout(central);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  createHeader();
  mainLayout->addWidget(m_header);

  m_moduleStack = new QStackedWidget(central);
  m_moduleStack->setStyleSheet("background-color: #ECF0FF");
  mainLayout->addWidget(m_moduleStack);

  createFooter();
  mainLayout->addWidget(m_footer);

  m_window->setCentralWidget(central);

  // Create the command palette (parented to central widget so it overlays correctly)
  m_commandPalette = new CommandPalette(central);
  connect(m_commandPalette, &CommandPalette::actionSelected,
          this, &UIManager::paletteActionSelected);
}


void UIManager::createHeader() {
  m_header = new QFrame();
  m_header->setFixedHeight(40);
  m_header->setObjectName("headerFrame");

  QHBoxLayout* layout = new QHBoxLayout(m_header);
  layout->setContentsMargins(10, 0, 10, 0);
  layout->setSpacing(0);

  QLabel* logo = new QLabel();
  logo->setObjectName("logoLabel");
  QPixmap pixmap(":/src/resources/logo.svg");

  if (!pixmap.isNull()) {
    logo->setPixmap(pixmap.scaledToHeight(32, Qt::SmoothTransformation));
    layout->addWidget(logo);
  }

  layout->addStretch(1);

  m_navLayout = new QHBoxLayout();
  m_navLayout->setSpacing(5);
  layout->addLayout(m_navLayout);

  layout->addStretch(1);
}


void UIManager::createFooter() {
  m_footer = new QFrame();
  m_footer->setFixedHeight(32);
  m_footer->setObjectName("footerFrame");

  QHBoxLayout* layout = new QHBoxLayout(m_footer);
  layout->setContentsMargins(10, 0, 10, 0);

  // Connection status (left side)
  m_statusLabel = new QLabel("Initializing...");
  m_statusLabel->setObjectName("statusLabel");

  // Mode indicator (center-right)
  m_modeLabel = new QLabel("-- NORMAL --");
  m_modeLabel->setObjectName("modeLabel");

  // Command buffer display (right side)
  m_bufferLabel = new QLabel("");
  m_bufferLabel->setObjectName("bufferLabel");

  layout->addWidget(m_statusLabel);
  layout->addStretch();
  layout->addWidget(m_modeLabel);
  layout->addWidget(m_bufferLabel);
}


// ─── Command palette delegation ──────────────────────────────

void UIManager::showCommandPalette(const QHash<QString, KeyBindingTarget>& registry,
                                    const QMap<QString, ModuleRecord*>& modules) {
  m_commandPalette->populate(registry, modules);
  m_commandPalette->show();
}


void UIManager::hideCommandPalette() {
  if (m_commandPalette) {
    m_commandPalette->hide();
  }
}


bool UIManager::isPaletteVisible() const {
  return m_commandPalette && m_commandPalette->isVisible();
}


// ─── Navigation bar ──────────────────────────────────────────

void UIManager::createNavBar(const QMap<QString, ModuleRecord*>& modules) {
  for (auto it = m_navButtons.begin(); it != m_navButtons.end(); ++it) {
    m_navLayout->removeWidget(it.value());
    delete it.value();
  }
  m_navButtons.clear();

  for (auto it = modules.begin(); it != modules.end(); ++it) {
    ModuleRecord* record = it.value();
    QString name = record->manifest["name"].toString().toUpper();
    QString id = record->manifest["id"].toString();
    QString hotkey = record->manifest["hotkey"].toString().toLower();

    // Show hotkey hint in nav button text
    QString label = hotkey.isEmpty() ? name : QString("%1 [%2]").arg(name, hotkey);

    QPushButton* btn = new QPushButton(label);
    btn->setObjectName("navButton");
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("active", false);

    QFont font = btn->font();
    font.setBold(true);
    font.setUnderline(false);
    btn->setFont(font);

    connect(btn, &QPushButton::clicked, [this, id]() {
      emit navRequested(id);
    });

    m_navLayout->addWidget(btn);
    m_navButtons[id] = btn;
  }
}

void UIManager::setActiveModule(const QString& moduleId) {
  m_activeModuleId = moduleId;

  for (auto it = m_navButtons.begin(); it != m_navButtons.end(); ++it) {
    bool isActive = (it.key() == moduleId);
    it.value()->setProperty("active", isActive);

    QFont font = it.value()->font();
    font.setBold(true);
    it.value()->setFont(font);

    it.value()->style()->unpolish(it.value());
    it.value()->style()->polish(it.value());
  }
}

void UIManager::setConnectionStatus(bool online, const QString& serverName){
  if (online) {
    m_statusLabel->setText(QString("● Connected: %1").arg(serverName.isEmpty() ? "Master" : serverName));
    m_statusLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
  } else {
    m_statusLabel->setText("● Disconnected");
    m_statusLabel->setStyleSheet("color: #d32f2f; font-weight: bold;");
  }
}

void UIManager::setModeIndicator(const QString& mode) {
  if (m_modeLabel) {
    m_modeLabel->setText(mode);
  }
}

void UIManager::setCommandBuffer(const QString& buffer) {
  if (m_bufferLabel) {
    m_bufferLabel->setText(buffer);
  }
}
