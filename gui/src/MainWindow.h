#pragma once

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>

class BackendRunner;

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
    void setBusy(bool busy);

    QString imagePath_;
    QWidget *controls_;
    QProgressBar *progress_;
    QPlainTextEdit *log_;
    BackendRunner *runner_;
};
