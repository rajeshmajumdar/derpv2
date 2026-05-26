#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QHash>
#include <QList>
#include <QString>
#include "../managers/PluginManager.h"

class CommandPalette : public QWidget {
  Q_OBJECT

signals:
  void actionSelected(const QString& moduleId, const QString& intentName);
  void paletteClosed();

public:
  explicit CommandPalette(QWidget* parent = nullptr);

  void populate(const QHash<QString, KeyBindingTarget>& registry,
                const QMap<QString, ModuleRecord*>& modules);

  void show();
  void hide();
  bool isVisible() const;

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

private:
  struct PaletteEntry {
    QString label;       // Display label (e.g. "Dashboard")
    QString category;    // "Module" or "Action"
    QString shortcut;    // Hotkey hint
    QString moduleId;
    QString intentName;
    QString searchText;  // Pre-computed lowercase search target
  };

  void loadStyleSheet();
  void filterResults(const QString& text);
  void executeSelection();

  // Fuzzy scoring: returns a score >= 0 (lower is better), or -1 for no match.
  // Combines subsequence matching with edit-distance tolerance.
  static int fuzzyScore(const QString& query, const QString& target);
  static int levenshteinDistance(const QString& a, const QString& b);

  QFrame* m_container;
  QLineEdit* m_input;
  QListWidget* m_results;

  QList<PaletteEntry> m_entries;
};

#endif // COMMANDPALETTE_H
