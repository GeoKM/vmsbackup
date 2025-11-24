#include "ControlsPane.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

ControlsPane::ControlsPane(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);

    imageLabel_ = new QLabel(tr("No image selected"), this);
    openButton_ = new QPushButton(tr("Open Image"), this);
    connect(openButton_, &QPushButton::clicked, this, &ControlsPane::chooseImageRequested);

    modeCombo_ = new QComboBox(this);
    modeCombo_->addItem(tr("List contents"));
    modeCombo_->addItem(tr("Extract latest saveset"));

    listButton_ = new QPushButton(tr("List"), this);
    extractButton_ = new QPushButton(tr("Extract"), this);
    connect(listButton_, &QPushButton::clicked, this, &ControlsPane::listRequested);
    connect(extractButton_, &QPushButton::clicked, this, &ControlsPane::extractRequested);

    layout->addWidget(imageLabel_);
    layout->addWidget(openButton_);
    layout->addWidget(modeCombo_);
    layout->addWidget(listButton_);
    layout->addWidget(extractButton_);
}

void ControlsPane::setImagePath(const QString &path)
{
    imageLabel_->setText(path.isEmpty() ? tr("No image selected") : tr("Image: %1").arg(path));
}
