#include "InputManager.h"
#include "../ui/CommandPalette.h"
#include "../../interfaces/DModule.h"

#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDebug>

InputManager::InputManager(PluginManager* pm, QObject* parent)
  : QObject(parent), m_pluginManager(pm), m_commandBuffer("") {

  m_timeoutTimer = new QTimer(this);
  m_timeoutTimer->setSingleShot(true);
  connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
    if (!m_commandBuffer.isEmpty()) {
      emit logRequired("Sequence timeout - cancelled: " + m_commandBuffer);
      m_commandBuffer.clear();
      emit bufferChanged(m_commandBuffer);
    }
  });
}


bool InputManager::isTextInputWidget(QWidget* widget) const {
  if (!widget) return false;
  return widget->inherits("QLineEdit")
      || widget->inherits("QTextEdit")
      || widget->inherits("QPlainTextEdit")
      || widget->inherits("QComboBox")
      || widget->inherits("QSpinBox")
      || widget->inherits("QDoubleSpinBox");
}


bool InputManager::handleBuiltinKey(const QString& key) {
  if (key == "h") { emit navigatePrevModule(); return true; }
  if (key == "l") { emit navigateNextModule(); return true; }
  if (key == "j") { emit navigateDown(); return true; }
  if (key == "k") { emit navigateUp(); return true; }
  if (key == "/") { emit searchRequested(); return true; }
  if (key == "x") { emit deleteRequested(); return true; }
  return false;
}


bool InputManager::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() != QEvent::KeyPress) {
    return QObject::eventFilter(obj, event);
  }

  // Anti bubble gaurd: stop processing the same key multiple times as it bubbles up the ui tree
  if (obj != QApplication::focusWidget() && obj != parent()) {
    return QObject::eventFilter(obj, event);
  }

  QKeyEvent *ke = static_cast<QKeyEvent*>(event);
  QWidget *focusedWidget = QApplication::focusWidget();

  if (focusedWidget) {
    QWidget* ancestor = focusedWidget;
    while (ancestor) {
      if (qobject_cast<CommandPalette*>(ancestor)) {
        return QObject::eventFilter(obj, event);
      }
      ancestor = ancestor->parentWidget();
    }
  }

  // Simple translation: grab standard keys reliably
  QString keyText;
  int rawKey = ke->key();

  if (rawKey >= Qt::Key_F1 && rawKey <= Qt::Key_F12) {
    keyText = QString("f%1").arg(rawKey - Qt::Key_F1 + 1);
  } else if (rawKey == Qt::Key_Return || rawKey == Qt::Key_Enter) {
    keyText = "return";
  } else if (rawKey == Qt::Key_Delete) {
    keyText = "delete";
  } else if (rawKey == Qt::Key_Tab) {
    keyText = "tab";
  } else if (rawKey == Qt::Key_Escape) {
    keyText = "escape";
  } else {
    keyText = ke->text().toLower().trimmed();
  }

  // Ignore modifier-only presses (Shift, Ctrl, Alt, etc.)
  if (keyText.isEmpty()) {
    return QObject::eventFilter(obj, event);
  }

  // Let ctrl/alt shortcuts pass through to the os
  if (ke->modifiers() & Qt::ControlModifier || ke->modifiers() & Qt::AltModifier) {
    // keep alt+l for logout
    if (!((ke->modifiers() & Qt::AltModifier) && rawKey == Qt::Key_L)) {
      return QObject::eventFilter(obj, event);
    }
  }

  // text input protection
  bool isTextInput = isTextInputWidget(QApplication::focusWidget());
  if (isTextInput && keyText != "escape") {
    if (!(keyText.startsWith("f") || keyText == "delete" || keyText == "return")) {
      return QObject::eventFilter(obj, event);
    }
  }

  // escape routing
  if (keyText == "escape") {
    if (isTextInput) QApplication::focusWidget()->clearFocus();
    m_mode = Mode::NORMAL;
    m_commandBuffer.clear();
    m_timeoutTimer->stop();
    emit modeChanged(m_mode);
    emit bufferChanged(m_commandBuffer);
    return true;
  }

  // alt+l logout binding
  if ((ke->modifiers() & Qt::AltModifier) && rawKey == Qt::Key_L) {
    m_commandBuffer.clear();
    m_timeoutTimer->stop();
    emit bufferChanged(m_commandBuffer);
    emit logoutRequested();
    return true;
  }

  // Priority 1: Private bindings
  QString activeModuleId = m_pluginManager->activeModuleId();
  DModule* activePlugin = m_pluginManager->getModuleInstance(activeModuleId);

  if (activePlugin && m_commandBuffer.isEmpty()) {
    if (activePlugin->hasPrivateBinding(keyText)) {
      m_timeoutTimer->stop();
      activePlugin->executeIntent(activePlugin->getPrivateIntent(keyText), QVariantMap());
      return true; // so it doesn't bleed
    }
  }

  m_commandBuffer += keyText;
  emit bufferChanged(m_commandBuffer);

  // priority 2: chords
  if (m_commandBuffer.length() == 2) {
    QString prefix = m_commandBuffer.left(1);
    QString suffix = m_commandBuffer.mid(1, 1);
    QString targetId = m_pluginManager->findModuleByGlobalSwitch(prefix);

    if (!targetId.isEmpty()) {
      DModule* targetPlugin = m_pluginManager->getModuleInstance(targetId);
      if (targetPlugin && targetPlugin->hasPublicChord(suffix)) {
        m_timeoutTimer->stop();

        // keep state synchronized internally
        m_pluginManager->setActiveModuleId(targetId);

        emit moduleSwitchRequested(targetId);
        targetPlugin->executeIntent(targetPlugin->getPublicIntent(suffix), QVariantMap());
        m_commandBuffer.clear();
        emit bufferChanged(m_commandBuffer);
        return true;
      }
    }

    // invalid chord type. drop the prefix, keep the new letter and try again
    m_commandBuffer = suffix;
    emit bufferChanged(m_commandBuffer);
    m_timeoutTimer->stop();
  }

  // priority 3: global switches and look-ahead
  if (m_commandBuffer.length() == 1) {
    QString targetModuleId = m_pluginManager->findModuleByGlobalSwitch(m_commandBuffer);
    if (!targetModuleId.isEmpty()) {
      // keep state synchronized internally
      m_pluginManager->setActiveModuleId(targetModuleId);
      emit moduleSwitchRequested(targetModuleId);

      DModule* targetPlugin = m_pluginManager->getModuleInstance(targetModuleId);
      if (targetPlugin && targetPlugin->hasPublicChordsConfigured()) {
        m_timeoutTimer->start(SEQUENCE_TIMEOUT_MS);
      } else {
        m_commandBuffer.clear();
        emit bufferChanged(m_commandBuffer);
      }
      return true;
    }

    if (handleBuiltinKey(m_commandBuffer)) {
      m_commandBuffer.clear();
      emit bufferChanged(m_commandBuffer);
      return true;
    }
  }

  // reject unknown sequence
  emit logRequired("unknown sequence trace rejected: " + m_commandBuffer);
  m_commandBuffer.clear();
  emit bufferChanged(m_commandBuffer);
  return QObject::eventFilter(obj, event);
}


void InputManager::enterNormalMode() {
  m_mode = Mode::NORMAL;
  m_commandBuffer.clear();
  m_timeoutTimer->stop();
  emit modeChanged(m_mode);
  emit bufferChanged(m_commandBuffer);
}


void InputManager::executeCommand(const QString& command) {
  emit logRequired("DEPRECATED: functionality merged directly up into the core event filter prioritization tree.");
}
