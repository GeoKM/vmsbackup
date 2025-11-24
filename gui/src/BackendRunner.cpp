#include "BackendRunner.h"

#include <QCoreApplication>

BackendRunner::BackendRunner(QObject *parent)
    : QObject(parent)
{
    connect(&process_, &QProcess::readyReadStandardOutput, this, &BackendRunner::handleReadyStdout);
    connect(&process_, &QProcess::readyReadStandardError, this, &BackendRunner::handleReadyStderr);
    connect(&process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BackendRunner::handleFinished);
}

void BackendRunner::runList(const QString &imagePath)
{
    startProcess(QStringList() << "-t" << "-f" << imagePath);
}

void BackendRunner::runExtractLatest(const QString &imagePath)
{
    startProcess(QStringList() << "-x" << "-f" << imagePath);
}

void BackendRunner::cancel()
{
    if (process_.state() != QProcess::NotRunning)
    {
        process_.kill();
    }
}

void BackendRunner::startProcess(const QStringList &args)
{
    if (process_.state() != QProcess::NotRunning)
    {
        emit logLine(tr("Another operation is running; please wait."));
        return;
    }

    QString program = QCoreApplication::applicationDirPath() + "/vmsbackup";
    emit started(QString("%1 %2").arg(program, args.join(' ')));
    process_.start(program, args);
}

void BackendRunner::handleReadyStdout()
{
    const auto data = process_.readAllStandardOutput();
    for (const QString &line : QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts))
        emit logLine(line);
}

void BackendRunner::handleReadyStderr()
{
    const auto data = process_.readAllStandardError();
    for (const QString &line : QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts))
        emit logLine(line);
}

void BackendRunner::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    emit finished(exitCode, status);
}
