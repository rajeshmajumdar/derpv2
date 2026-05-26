#include "CommandPalette.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QRegularExpression>
#include <algorithm>


CommandPalette::CommandPalette(QWidget* parent) : QWidget(parent) {
  // Full-window transparent overlay that captures all input
  setWindowFlags(Qt::Widget);
  setAttribute(Qt::WA_StyledBackground, true);
  setFocusPolicy(Qt::ClickFocus);
  QWidget::hide();

  // --- Container (the actual palette box) ---
  m_container = new QFrame(this);
  m_container->setObjectName("paletteContainer");

  QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(m_container);
  shadow->setBlurRadius(30);
  shadow->setColor(QColor(0, 0, 0, 180));
  shadow->setOffset(0, 4);
  m_container->setGraphicsEffect(shadow);

  QVBoxLayout* containerLayout = new QVBoxLayout(m_container);
  containerLayout->setContentsMargins(0, 0, 0, 0);
  containerLayout->setSpacing(0);

  // --- Input row ---
  QFrame* inputRow = new QFrame();
  inputRow->setObjectName("paletteInputRow");

  QHBoxLayout* inputLayout = new QHBoxLayout(inputRow);
  inputLayout->setContentsMargins(10, 6, 10, 6);
  inputLayout->setSpacing(8);

  // ">" prefix label like VS Code
  QLabel* prefixLabel = new QLabel(">");
  prefixLabel->setObjectName("palettePrefixLabel");
  inputLayout->addWidget(prefixLabel);

  m_input = new QLineEdit();
  m_input->setObjectName("paletteInput");
  m_input->setPlaceholderText("Type to search commands...");
  m_input->setFrame(false);
  inputLayout->addWidget(m_input);

  containerLayout->addWidget(inputRow);

  // --- Separator ---
  QFrame* separator = new QFrame();
  separator->setObjectName("paletteSeparator");
  separator->setFixedHeight(1);
  containerLayout->addWidget(separator);

  // --- Results list ---
  m_results = new QListWidget();
  m_results->setObjectName("paletteResults");
  m_results->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_results->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_results->setFocusPolicy(Qt::NoFocus);
  containerLayout->addWidget(m_results);

  // Load stylesheet from resources
  loadStyleSheet();

  // --- Signals ---
  connect(m_input, &QLineEdit::textChanged, this, &CommandPalette::filterResults);
  connect(m_input, &QLineEdit::returnPressed, this, &CommandPalette::executeSelection);
  connect(m_results, &QListWidget::itemClicked, this, [this](QListWidgetItem*) {
    executeSelection();
  });

  // Event filters for keyboard navigation
  m_input->installEventFilter(this);
}


void CommandPalette::loadStyleSheet() {
  QFile qss(":/src/resources/command_palette.qss");
  if (qss.open(QFile::ReadOnly | QFile::Text)) {
    m_container->setStyleSheet(QLatin1String(qss.readAll()));
    qss.close();
  }
}


bool CommandPalette::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_input && event->type() == QEvent::KeyPress) {
    QKeyEvent* ke = static_cast<QKeyEvent*>(event);

    if (ke->key() == Qt::Key_Escape) {
      hide();
      return true;
    }

    if (ke->key() == Qt::Key_Down) {
      int row = m_results->currentRow();
      if (row < m_results->count() - 1) {
        m_results->setCurrentRow(row + 1);
      }
      return true;
    }

    if (ke->key() == Qt::Key_Up) {
      int row = m_results->currentRow();
      if (row > 0) {
        m_results->setCurrentRow(row - 1);
      }
      return true;
    }
  }

  return QWidget::eventFilter(obj, event);
}


void CommandPalette::paintEvent(QPaintEvent*) {
  // Draw semi-transparent dark scrim behind the palette
  QPainter painter(this);
  painter.fillRect(rect(), QColor(0, 0, 0, 80));
}


void CommandPalette::mousePressEvent(QMouseEvent* event) {
  // Click outside the container → close
  if (!m_container->geometry().contains(event->pos())) {
    hide();
  }
}


void CommandPalette::populate(const QHash<QString, KeyBindingTarget>& registry,
                               const QMap<QString, ModuleRecord*>& modules) {
  m_entries.clear();

  // Add modules
  for (auto it = modules.begin(); it != modules.end(); ++it) {
    PaletteEntry entry;
    entry.label = it.value()->manifest["name"].toString();
    entry.category = "Module";
    entry.shortcut = it.value()->manifest["hotkey"].toString();
    entry.moduleId = it.key();
    entry.searchText = entry.label.toLower() + " " + entry.moduleId.toLower();
    m_entries.append(entry);
  }

  // Add intents
  for (auto it = registry.begin(); it != registry.end(); ++it) {
    if (!it.value().intentName.isEmpty()) {
      PaletteEntry entry;
      entry.label = it.value().intentName;
      entry.category = "Action";
      entry.shortcut = it.key();
      entry.moduleId = it.value().moduleId;
      entry.intentName = it.value().intentName;
      entry.searchText = entry.label.toLower() + " " + entry.moduleId.toLower();
      m_entries.append(entry);
    }
  }

  m_input->clear();
  filterResults("");
}


void CommandPalette::show() {
  if (!parentWidget()) return;

  // Size the overlay to fill the entire parent (central widget)
  setGeometry(parentWidget()->rect());
  raise();
  QWidget::show();
  m_input->setFocus();

  // Position the container: centered horizontally, pinned to top
  int cw = qMin(560, width() - 40);
  int ch = m_container->sizeHint().height();
  int cx = (width() - cw) / 2;
  m_container->setGeometry(cx, 0, cw, ch);
}


void CommandPalette::hide() {
  QWidget::hide();
  m_input->clear();
  emit paletteClosed();
}


bool CommandPalette::isVisible() const {
  return QWidget::isVisible();
}


// ─── Fuzzy search ──────────────────────────────────────────────

int CommandPalette::levenshteinDistance(const QString& a, const QString& b) {
  int m = a.length(), n = b.length();
  QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1, 0));
  for (int i = 0; i <= m; i++) dp[i][0] = i;
  for (int j = 0; j <= n; j++) dp[0][j] = j;
  for (int i = 1; i <= m; i++) {
    for (int j = 1; j <= n; j++) {
      int cost = (a[i-1] == b[j-1]) ? 0 : 1;
      dp[i][j] = std::min({dp[i-1][j] + 1, dp[i][j-1] + 1, dp[i-1][j-1] + cost});
    }
  }
  return dp[m][n];
}


int CommandPalette::fuzzyScore(const QString& query, const QString& target) {
  if (query.isEmpty()) return 0;
  if (target.isEmpty()) return -1;

  // 1. Exact substring match → best score
  if (target.contains(query)) {
    return 0;
  }

  // 2. Subsequence match → decent score (position-weighted)
  {
    int qi = 0;
    for (int ti = 0; ti < target.length() && qi < query.length(); ti++) {
      if (query[qi] == target[ti]) qi++;
    }
    if (qi == query.length()) {
      return 1;  // subsequence found
    }
  }

  // 3. Check each word in the target for edit-distance tolerance
  QStringList words = target.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  int bestWordDist = INT_MAX;
  for (const QString& word : words) {
    // Allow up to ~40% of query length as edit distance, minimum 1
    int dist = levenshteinDistance(query, word);
    int threshold = qMax(1, query.length() * 2 / 5);
    if (dist <= threshold) {
      bestWordDist = qMin(bestWordDist, dist);
    }
  }
  if (bestWordDist < INT_MAX) {
    return 2 + bestWordDist;  // fuzzy match, scored by distance
  }

  // 4. Whole-target edit distance (for short queries against short targets)
  if (query.length() <= 6 && target.length() <= 20) {
    int dist = levenshteinDistance(query, target);
    int threshold = qMax(2, query.length() / 2);
    if (dist <= threshold) {
      return 3 + dist;
    }
  }

  return -1;  // no match
}


void CommandPalette::filterResults(const QString& text) {
  m_results->clear();
  QString query = text.toLower().trimmed();

  struct ScoredEntry {
    int index;
    int score;
  };

  QList<ScoredEntry> scored;

  for (int i = 0; i < m_entries.size(); ++i) {
    const PaletteEntry& entry = m_entries[i];
    int score = fuzzyScore(query, entry.searchText);
    if (score >= 0) {
      scored.append({i, score});
    }
  }

  // Sort by score (lower = better match)
  std::sort(scored.begin(), scored.end(), [](const ScoredEntry& a, const ScoredEntry& b) {
    return a.score < b.score;
  });

  for (const ScoredEntry& s : scored) {
    const PaletteEntry& entry = m_entries[s.index];

    // Build rich display text: "category: label    shortcut"
    QString display;
    if (!entry.category.isEmpty()) {
      display = entry.category + ": " + entry.label;
    } else {
      display = entry.label;
    }
    if (!entry.shortcut.isEmpty()) {
      display += "    [" + entry.shortcut + "]";
    }

    QListWidgetItem* item = new QListWidgetItem(display);
    item->setData(Qt::UserRole, s.index);
    m_results->addItem(item);
  }

  // Resize results to fit content (max ~350px)
  int itemH = m_results->sizeHintForRow(0);
  if (itemH <= 0) itemH = 28;
  int visibleItems = qMin(m_results->count(), 12);
  m_results->setFixedHeight(visibleItems * itemH + 4);

  // Re-layout the container
  m_container->adjustSize();
  int cw = qMin(560, width() - 40);
  int ch = m_container->sizeHint().height();
  int cx = (width() - cw) / 2;
  m_container->setGeometry(cx, 0, cw, ch);

  if (m_results->count() > 0) {
    m_results->setCurrentRow(0);
  }
}


void CommandPalette::executeSelection() {
  QListWidgetItem* current = m_results->currentItem();
  if (!current) {
    hide();
    return;
  }

  int idx = current->data(Qt::UserRole).toInt();
  if (idx >= 0 && idx < m_entries.size()) {
    const PaletteEntry& entry = m_entries[idx];
    emit actionSelected(entry.moduleId, entry.intentName);
  }

  hide();
}
