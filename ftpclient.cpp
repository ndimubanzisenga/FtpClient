#include "ftpclient.h"
#include <QtCore/QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QtFtp>


FtpClient::FtpClient(std::string serverName)
{
    m_serverName = QString::fromStdString(serverName);
    m_userName = QString::fromStdString("");
    m_passWord = QString::fromStdString("");

    ftp = nullptr;
    networkSession = nullptr;
}


FtpClient::FtpClient(std::string serverName, const std::string userName, const std::string passWord)
{
    m_serverName = QString::fromStdString(serverName);
    m_userName = QString::fromStdString(userName);
    m_passWord = QString::fromStdString(passWord);

    ftp = nullptr;
    networkSession = nullptr;
}

FtpClient::~FtpClient()
{
    delete ftp;
    delete networkSession;
    delete m_dFile;
    delete m_uFile;
}

bool FtpClient::init()
{
    if (!networkSession || !networkSession->isOpen()) {
        if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
            if (!networkSession) {
                // Get saved network configuration
                QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
                settings.beginGroup(QLatin1String("QtNetwork"));
                const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
                settings.endGroup();

                // If the saved network configuration is not currently discovered use the system default
                QNetworkConfiguration config = manager.configurationFromIdentifier(id);
                if ((config.state() & QNetworkConfiguration::Discovered) !=
                    QNetworkConfiguration::Discovered) {
                    config = manager.defaultConfiguration();
                }

                networkSession = new QNetworkSession(config, this);
                connect(networkSession, SIGNAL(opened()), this, SLOT(connectToFtp()));
            }
            qDebug() << (tr("FTP : Opening network session."));
            networkSession->open();
            return true;
        }
    }
    connectToFtp();
    return true;
}

bool FtpClient::close()
{
    if (ftp) {
        ftp->abort();
        ftp->deleteLater();
        ftp = nullptr;
    }
    return true;
}

void FtpClient::connectToFtp()
{
        ftp = new QFtp(this);
        connect(ftp, SIGNAL(commandFinished(int,bool)),
                this, SLOT(ftpCommandFinished(int,bool)));
        connect(ftp, SIGNAL(listInfo(QUrlInfo)),
                this, SLOT(addToList(QUrlInfo)));
        connect(ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
                this, SLOT(updateDataTransferProgress(qint64,qint64)));
        connect(ftp, SIGNAL(done(bool)), this, SLOT(doneExecPendingCommands(bool)));


        currentPath.clear();
        isDirectory.clear();
        m_fileList.clear();

        QUrl url(m_serverName);
        url.setUserName(m_userName);
        url.setPassword(m_passWord);

        if (!url.isValid() || url.scheme().toLower() != QLatin1String("ftp")) {
            ftp->connectToHost(m_serverName, 21);
            ftp->login();
        } else {
            ftp->connectToHost(url.host(), url.port(21));

            if (!url.userName().isEmpty())
                ftp->login(QUrl::fromPercentEncoding(url.userName().toLatin1()), url.password());
            else
                ftp->login();
            if (!url.path().isEmpty())
                ftp->cd(url.path());
        }

        qDebug() << (tr("FTP : Connecting to FTP server %1 With %2...").arg(m_serverName));
}

void FtpClient::downloadFile(const std::string fileName)
{
    QString fileToDownload = QString::fromStdString(fileName);
    QFileInfo fileInfo(fileToDownload);

    if (fileInfo.exists()) {
        qDebug() << ( tr("FTP : There already exists a file called %1"
                         "in the current directory.").arg(fileToDownload));
        return;
    }

    m_dFile = new QFile(fileToDownload);
    if (!m_dFile->open(QIODevice::WriteOnly)) {
        qDebug() << (tr("FTP : Unable to save the file %1: %2.")
                     .arg(fileToDownload).arg(m_dFile->errorString()));
        delete m_dFile;
        return;
    }

    ftp->get(fileToDownload, m_dFile);

    qDebug() << (tr("FTP : Downloading %1...").arg(fileToDownload));
}

void FtpClient::uploadFile(const std::string fileName)
{
    QString fileToUpload = QString::fromStdString(fileName);
    QFileInfo fileInfo(fileToUpload);

    if (!fileInfo.exists()) {
        qDebug() << tr("FTP : File to upload not found : %1.").arg(fileToUpload);
    }

    m_uFile = new QFile(fileToUpload);
    if (!m_uFile->open(QIODevice::ReadOnly)) {
        qDebug() << tr("FTP : Unable to open file %1 : %2").arg(fileToUpload).arg(m_uFile->errorString());
        delete m_uFile;

        return;
    }
    ftp->put(m_uFile, fileInfo.fileName());

    qDebug() << tr("FTP: Uploading %1...").arg(fileToUpload);
}

void FtpClient::cancelDownload()
{
    ftp->abort();

    if (m_dFile->exists()) {
        m_dFile->close();
        m_dFile->remove();
    }
    delete m_dFile;
}

void FtpClient::deleteFile(const std::string fileName)
{
    QString fileToDelete = QString::fromStdString(fileName);
    ftp->remove(fileToDelete);
}


void FtpClient::ftpCommandFinished(int, bool error)
{
    if (ftp->currentCommand() == QFtp::ConnectToHost) {
        if (error) {
            qDebug() << (tr("FTP : Unable to connect to the FTP server at %1."
                            "Please check that the host name is correct.")
                         .arg(m_serverName));
            connectToFtp();
            return;
        }
        qDebug() << (tr("FTP : Connected to %1.").arg(m_serverName));
        return;
    }

    if (ftp->currentCommand() == QFtp::Login) {
        if (error) {
            qDebug() << (tr("FTP : Unable to login to the FTP server at %1."
                            "Please check tha the username and password are correct.")
                         .arg(m_serverName));
            emit connectedToHost(false);
            return;
        }
        ftp->list();
        emit connectedToHost(true);
        qDebug() << (tr("FTP : Logged onto %1.").arg(m_serverName));
        return;
    }

    if (ftp->currentCommand() == QFtp::Put) {
        if (error) {
            qDebug() << tr("FTP : Canceled upload of %1.").arg(m_uFile->fileName());
            m_uFile->close();
            m_uFile->remove();
            emit uploadedFile(false);
        } else {
            qDebug() << tr("FTP : Uploaded %1 to %2").arg(m_uFile->fileName()).arg(m_serverName);
            m_uFile->close();
            emit uploadedFile(true);
        }
        delete m_uFile;
    }

    if (ftp->currentCommand() == QFtp::Get) {
        if (error) {
            qDebug() << (tr("FTP : Canceled download of %1.").arg(m_dFile->fileName()));
            m_dFile->close();
            m_dFile->remove();
            emit downloadedFile(false);
        } else {
            qDebug() << (tr("FTP : Downloaded %1 to current directory.").arg(m_dFile->fileName()));
            m_dFile->close();
            emit downloadedFile(true);
        }
        delete m_dFile;

    }
    if (ftp->currentCommand() == QFtp::Remove) {
        if (error) {
            qDebug() << tr("FTP : Could not remove remote file!");
            emit removedFile(false);
        } else {
            qDebug() << tr("FTP : Removed file!");
            emit removedFile(true);
        }
    }

    if (ftp->currentCommand() == QFtp::List) {
        if (isDirectory.isEmpty()) {

        }
    }
}

void FtpClient::addToList(const QUrlInfo &urlInfo)
{
    // qDebug() << urlInfo.name();
    m_fileList.append(urlInfo.name());
    isDirectory[urlInfo.name()] = urlInfo.isDir();
}

void FtpClient::getDirFilesList()
{
    //qDebug() << tr("Files in dir :%1").arg(currentPath);
    //qDebug() << m_fileList;
}

bool FtpClient::isFileInCurrentDir(const std::string fileName)
{
    bool fileFound = false;

    for (int i = 0; i < m_fileList.length(); i++) {
        if (m_fileList[i].toStdString() == fileName) {
            fileFound = true;
            break;
        }
    }

    return fileFound;
}

void FtpClient::cdToParent()
{
    m_fileList.clear();
    isDirectory.clear();
    currentPath = currentPath.left(currentPath.lastIndexOf('/'));
    if (currentPath.isEmpty()) {
        ftp->cd("/");
    } else {
        ftp->cd(currentPath);
    }
    ftp->list();
}

void FtpClient::cdToDir(const std::string dir)
{
    m_fileList.clear();
    isDirectory.clear();
    currentPath = QString::fromStdString(dir).left(currentPath.lastIndexOf('/'));
    if (currentPath.isEmpty()) {
        ftp->cd("/");
    } else {
        ftp->cd(currentPath);
    }
    ftp->list();
}

void FtpClient::updateDataTransferProgress(qint64 readBytes, qint64 totalBytes)
{
    // qDebug() << (tr("Progress : %1 %").arg(100 * readBytes/totalBytes));
}

void FtpClient::doneExecPendingCommands(bool status)
{
    emit done(true);
}
