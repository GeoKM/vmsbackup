#pragma once

#include <QObject>
#include <QStringList>
#include <QProcess>

class BackendRunner : public QObject
{
    Q_OBJECT
public:
    explicit BackendRunner(QObject *parent = nullptr);

signals:
    void started(const QString &commandLine);
    void finished(int exitCode, QProcess::ExitStatus status);
    void logLine(const QString &line);

public slots:
    void runList(const QString &imagePath);
    void runExtractLatest(const QString &imagePath);
    void cancel();

private slots:
    void handleReadyStdout();
    void handleReadyStderr();
    void handleFinished(int exitCode, QProcess::ExitStatus status);

private:
    void startProcess(const QStringList &args);

    QProcess process_;
};
