#ifndef D_TABLE_WIDGET_H
#define D_TABLE_WIDGET_H

#include <QKeyEvent>
#include <QStringList>
#include <QTableWidget>
#include <QVariantList>

class DTableWidget : public QTableWidget {
  Q_OBJECT

public:
  explicit DTableWidget(QWidget *parent = nullptr);

  // Configure the columns and sets the header labels
  void setup(const QStringList &headers);

  // Appends a row of data
  void addRow(const QVariantList &rowData);

  // Clears the data row but keeps the header intact
  void clearData();

  void setSearchable(bool searchable);
  bool isSearchable() const;

public slots:
  void filterData(const QString &query);

signals:
  void typingDetected(const QString &currentQuery);

protected:
  void keyPressEvent(QKeyEvent *event) override;

private:
  bool m_searchable = false;
  QString m_searchBuffer;
};

#endif // !D_TABLE_WIDGET_H
