#include "DTableWidget.h"
#include <QHeaderView>

DTableWidget::DTableWidget(QWidget *parent) : QTableWidget(parent) {
  this->setStyleSheet("QTableWidget {"
                      "   background-color: #FFFFFF;"
                      "   color: #212121;"
                      "   gridline-color: #E0E0E0;"
                      "   border: 1px solid #CFD8DC;"
                      "   border-radius: 4px;"
                      "   selection-background-color: #1565C0;"
                      "   selection-color: #FFFFFF;"
                      "}"
                      "QTableWidget::item:selected {"
                      "   color: #FFFFFF;"
                      "   background-color: #1565C0;"
                      "}"
                      "QHeaderView::section {"
                      "   background-color: #F5F7FA;"
                      "   color: #37474F;"
                      "   font-weight: bold;"
                      "   padding: 6px;"
                      "   border: 1px solid #E0E0E0;"
                      "   border-top: none;"
                      "   border-left: none;"
                      "}");

  this->setAlternatingRowColors(true);
  this->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->setSelectionMode(QAbstractItemView::SingleSelection);
  this->verticalHeader()->setVisible(false);
  this->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void DTableWidget::setup(const QStringList &headers) {
  this->clear();
  this->setColumnCount(headers.size());
  this->setHorizontalHeaderLabels(headers);
  this->setRowCount(0);

  // automatically stretch the last column to fill the screen
  this->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  if (headers.size() > 0) {
    this->horizontalHeader()->setSectionResizeMode(headers.size() - 1,
                                                   QHeaderView::Stretch);
  }
}

void DTableWidget::addRow(const QVariantList &rowData) {
  int row = this->rowCount();
  this->insertRow(row);

  for (int col = 0; col < rowData.size() && col < this->columnCount(); ++col) {
    QTableWidgetItem *item = new QTableWidgetItem(rowData[col].toString());
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    this->setItem(row, col, item);
  }
}

void DTableWidget::clearData() { this->setRowCount(0); }

void DTableWidget::setSearchable(bool searchable) { m_searchable = searchable; }

bool DTableWidget::isSearchable() const { return m_searchable; }

void DTableWidget::filterData(const QString &query) {
  m_searchBuffer = query;

  int firstVisibleRow = -1;

  for (int row = 0; row < this->rowCount(); ++row) {
    bool rowMatches = false;

    if (query.isEmpty()) {
      rowMatches = true;
    } else {
      for (int col = 0; col < this->columnCount(); ++col) {
        QTableWidgetItem *cell = this->item(row, col);
        if (cell && cell->text().contains(query, Qt::CaseInsensitive)) {
          rowMatches = true;
          break;
        }
      }
    }

    this->setRowHidden(row, !rowMatches);

    if (rowMatches && firstVisibleRow == -1) {
      firstVisibleRow = row;
    }
  }

  if (firstVisibleRow != -1) {
    this->selectRow(firstVisibleRow);
  } else {
    this->clearSelection();
  }
}

void DTableWidget::keyPressEvent(QKeyEvent *event) {
  if (!m_searchable) {
    QTableWidget::keyPressEvent(event);
    return;
  }

  int key = event->key();

  if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left ||
      key == Qt::Key_Right) {
    QTableWidget::keyPressEvent(event);
    return;
  }

  if (key == Qt::Key_Backspace) {
    if (!m_searchBuffer.isEmpty()) {
      m_searchBuffer.chop(1);
      filterData(m_searchBuffer);
      emit typingDetected(m_searchBuffer);
    }
    return;
  }

  if (key == Qt::Key_Escape) {
    m_searchBuffer.clear();
    filterData("");
    emit typingDetected(m_searchBuffer);
    return;
  }

  QString typedChar = event->text();
  if (!typedChar.isEmpty() && typedChar.at(0).isPrint()) {
    m_searchBuffer += typedChar;
    filterData(m_searchBuffer);
    emit typingDetected(m_searchBuffer);
    return;
  }

  QTableWidget::keyPressEvent(event);
}
