#include "MainWindow.h"

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

    imageLabel_ = new QLabel(tr("No image selected"), this);
    openButton_ = new QPushButton(tr("Open Image"), this);
    connect(openButton_, &QPushButton::clicked, this, &MainWindow::chooseImage);

    modeCombo_ = new QComboBox(this);
    modeCombo_->addItem(tr("List contents"));
    modeCombo_->addItem(tr("Extract selected"));

    listButton_ = new QPushButton(tr("List"), this);
    extractButton_ = new QPushButton(tr("Extract"), this);
    connect(listButton_, &QPushButton::clicked, this, &MainWindow::startListing);
    connect(extractButton_, &QPushButton::clicked, this, &MainWindow::startExtract);

    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    progress_->setValue(0);

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setPlaceholderText(tr("Logs will appear here"));

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addWidget(openButton_);
    buttonRow->addWidget(modeCombo_);
    buttonRow->addWidget(listButton_);
    buttonRow->addWidget(extractButton_);

    layout->addWidget(imageLabel_);
    layout->addLayout(buttonRow);
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
    imageLabel_->setText(tr("Image: %1").arg(file));
    appendLog(tr("Selected image: %1").arg(file));
}

void MainWindow::startListing()
{
    if (imagePath_.isEmpty())
    {
        appendLog(tr("Please choose an image first."));
        return;
    }
    appendLog(tr("Listing contents of %1 (stub)...").arg(imagePath_));
    progress_->setValue(10);
    // TODO: hook into libvmsbackup listing when the re-entrant API is ready.
}

void MainWindow::startExtract()
{
    if (imagePath_.isEmpty())
    {
        appendLog(tr("Please choose an image first."));
        return;
    }
    appendLog(tr("Extracting selection from %1 (stub)...").arg(imagePath_));
    progress_->setValue(10);
    // TODO: hook into libvmsbackup extraction when the re-entrant API is ready.
}

void MainWindow::appendLog(const QString &line)
{
    log_->appendPlainText(line);
    log_->ensureCursorVisible();
}
