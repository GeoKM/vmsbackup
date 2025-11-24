#include "MainWindow.h"
#include "ControlsPane.h"
#include "BackendRunner.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *controlsPane = new ControlsPane(this);
    controls_ = controlsPane;
    connect(controlsPane, &ControlsPane::chooseImageRequested, this, &MainWindow::chooseImage);
    connect(controlsPane, &ControlsPane::listRequested, this, &MainWindow::startListing);
    connect(controlsPane, &ControlsPane::extractRequested, this, &MainWindow::startExtract);

    runner_ = new BackendRunner(this);
    connect(runner_, &BackendRunner::started, this, [this](const QString &cmd){
        appendLog(tr("Started: %1").arg(cmd));
        setBusy(true);
        progress_->setValue(5);
    });
    connect(runner_, &BackendRunner::logLine, this, &MainWindow::appendLog);
    connect(runner_, &BackendRunner::finished, this, [this](int code, QProcess::ExitStatus status){
        appendLog(tr("Finished (%1)").arg(code));
        setBusy(false);
        progress_->setValue(status == QProcess::NormalExit ? 100 : 0);
    });

    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    progress_->setValue(0);

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setPlaceholderText(tr("Logs will appear here"));

    layout->addWidget(controls_);
    layout->addWidget(progress_);
    layout->addWidget(log_);

    setCentralWidget(central);
    setWindowTitle(tr("VMS Backup GUI"));
    resize(720, 480);
}

void MainWindow::chooseImage()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Select tape image"));
    if (file.isEmpty())
        return;
    imagePath_ = file;
    auto *pane = qobject_cast<ControlsPane *>(controls_);
    if (pane)
        pane->setImagePath(file);
    appendLog(tr("Selected image: %1").arg(file));
}

void MainWindow::startListing()
{
    if (imagePath_.isEmpty())
    {
        appendLog(tr("Please choose an image first."));
        return;
    }
    appendLog(tr("Listing contents of %1").arg(imagePath_));
    runner_->runList(imagePath_);
}

void MainWindow::startExtract()
{
    if (imagePath_.isEmpty())
    {
        appendLog(tr("Please choose an image first."));
        return;
    }
    appendLog(tr("Extracting selection from %1").arg(imagePath_));
    runner_->runExtractLatest(imagePath_);
}

void MainWindow::appendLog(const QString &line)
{
    log_->appendPlainText(line);
    log_->ensureCursorVisible();
}

void MainWindow::setBusy(bool busy)
{
    if (controls_)
        controls_->setDisabled(busy);
}
