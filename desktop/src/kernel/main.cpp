#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QTextStream>
#include "Kernel.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  int fontId = QFontDatabase::addApplicationFont(":/src/resources/fonts/IBMPlexSans-VariableFont_wdth,wght.ttf");
  if (fontId == -1) {
    qWarning() << "Failed to load IBMPLexSans";
  }
  fontId = QFontDatabase::addApplicationFont(":/src/resources/fonts/JetBrainsMono-VariableFont_wght.ttf");
  if (fontId == -1) {
    qWarning() << "Failed to load JetBrainsMono";
  }

  QFile styleFile(":/src/resources/style.qss");
  if (styleFile.open(QFile::ReadOnly)) {
    QTextStream textStream(&styleFile);
    a.setStyleSheet(textStream.readAll());
  }

  Kernel w;

  return a.exec();
}
