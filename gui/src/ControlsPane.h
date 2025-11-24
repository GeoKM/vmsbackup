#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;

class ControlsPane : public QWidget
{
    Q_OBJECT
public:
    explicit ControlsPane(QWidget *parent = nullptr);

signals:
    void chooseImageRequested();
    void listRequested();
    void extractRequested();

public slots:
    void setImagePath(const QString &path);

private:
    QLabel *imageLabel_;
    QPushButton *openButton_;
    QPushButton *listButton_;
    QPushButton *extractButton_;
    QComboBox *modeCombo_;
};
