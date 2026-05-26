#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QHash>
#include <QStringList>
#include <QTimer>
#include "PluginManager.h"

// Neovim-style modal input manager.
//
// Two modes:
//   NORMAL — default. Keys are interpreted as commands.
//     Single key matching a module hotkey → switch to that module.
//     Action prefix + module key → switch + execute intent.
//     Escape → cancel partial sequence.
//     Built-in keys: h/j/k/l (navigate), / (search), x (delete)
//     Alt+L → logout
//
//   INSERT — when a text input (QLineEdit, QTextEdit, etc.) has focus.
//     All keys pass through to the widget.
//     Escape → clear focus → return to NORMAL mode.
//
class InputManager : public QObject {
  Q_OBJECT

public:
  enum class Mode { NORMAL, INSERT };

  explicit InputManager(PluginManager* pm, QObject* parent = nullptr);

  Mode currentMode() const { return m_mode; }
  QString commandBuffer() const { return m_commandBuffer; }

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

signals:
  // Emitted when a module switch is requested via hotkey
  void moduleSwitchRequested(const QString& moduleId);

  // Emitted when the input mode changes (for UI indicator)
  void modeChanged(InputManager::Mode mode);

  // Emitted when the command buffer changes (for UI display)
  void bufferChanged(const QString& buffer);

  // Emitted for log messages
  void logRequired(const QString& msg);

  // ─── Global action signals ───────────────────────────────
  void searchRequested();           // /
  void deleteRequested();           // x
  void navigatePrevModule();        // h  (previous module)
  void navigateNextModule();        // l  (next module)
  void navigateUp();                // k  (up in current view)
  void navigateDown();              // j  (down in current view)
  void logoutRequested();           // Alt+L

private:
  void enterNormalMode();
  void executeCommand(const QString& command);
  bool isTextInputWidget(QWidget* widget) const;
  bool handleBuiltinKey(const QString& key);

  PluginManager* m_pluginManager;
  Mode m_mode = Mode::NORMAL;
  QString m_commandBuffer;
  QTimer* m_timeoutTimer;

  static const int SEQUENCE_TIMEOUT_MS = 800;
};

#endif // !INPUTMANAGER_H
