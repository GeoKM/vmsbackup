#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QProgressBar>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void chooseImage();
    void startListing();
    void startExtract();
    void appendLog(const QString &line);

private:
    void buildUi();

    QString imagePath_;
    QLabel *imageLabel_;
    QPushButton *openButton_;
    QPushButton *listButton_;
    QPushButton *extractButton_;
    QComboBox *modeCombo_;
    QProgressBar *progress_;
    QPlainTextEdit *log_;
};
