#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <QObject>
#include <QMap>
#include <QPushButton>
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include "CommandPalette.h"
#include "../managers/PluginManager.h"

class UIManager : public QObject {
  Q_OBJECT

  signals:
    void navRequested(const QString& moduleId);
    void paletteActionSelected(const QString& moduleId, const QString& intentName);

  public:
    explicit UIManager(QMainWindow* window);

    void setupShell();
    QStackedWidget* moduleStack() const { return m_moduleStack; }

    void setStatusMessage(const QString &msg);

    void createNavBar(const QMap<QString, ModuleRecord*>& modules);
    void setActiveModule(const QString& moduleId);
    QString activeModuleId() const { return m_activeModuleId; }

    // Mode indicator for neovim-style input
    void setModeIndicator(const QString& mode);
    void setCommandBuffer(const QString& buffer);

    // Command palette
    void showCommandPalette(const QHash<QString, KeyBindingTarget>& registry,
                            const QMap<QString, ModuleRecord*>& modules);
    void hideCommandPalette();
    bool isPaletteVisible() const;

  public slots:
    void setConnectionStatus(bool online, const QString& serverName = "");

  private:
    QMainWindow* m_window;

    QStackedWidget* m_moduleStack;
    QFrame* m_header;
    QFrame* m_footer;
    QLabel *m_statusLabel;
    QLabel *m_modeLabel;
    QLabel *m_bufferLabel;
    QString m_activeModuleId;

    QMap<QString, QPushButton*> m_navButtons;
    QHBoxLayout* m_navLayout;

    // Command palette (separate widget)
    CommandPalette* m_commandPalette = nullptr;

    void createHeader();
    void createFooter();
};

#endif // !UIMANAGER_H
