#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <string>
#include <QObject>
#include <QVector>
#include <QNetworkConfigurationManager>

QT_BEGIN_NAMESPACE
class QFile;
class QFtp;
class QUrlInfo;
class QNetworkSession;
QT_END_NAMESPACE

class FtpClient : public QObject
{
    Q_OBJECT

public:
    FtpClient(const std::string serverName);
    FtpClient(const std::string serverName, const std::string userName, const std::string passWord);
    ~FtpClient();

    bool init();
    bool close();
    void downloadFile(const std::string fileToDownload);
    void uploadFile(const std::string fileToUpload);
    void cancelDownload();
    void deleteFile(const std::string fileToDelete);
    void cdToDir(const std::string dir);
    void getDirFilesList();
    bool isFileInCurrentDir(const std::string fileName);

public slots:
    void connectToFtp();

    void ftpCommandFinished(int commandId, bool error);
    void addToList(const QUrlInfo &urlInfo);
    void cdToParent();
    void updateDataTransferProgress(qint64 readBytes, qint64 totalBytes);
    void doneExecPendingCommands(bool status);

signals:
    void connectedToHost(bool connected);
    void uploadedFile(bool uploaded);
    void downloadedFile(bool downloaded);
    void removedFile(bool removed);
    void done(bool status);

private:
    QString m_serverName;
    QString m_userName;
    QString m_passWord;
    QHash<QString, bool> isDirectory;
    QString currentPath;
    QFtp *ftp;
    QFile *m_dFile;
    QFile *m_uFile;

    QNetworkSession *networkSession;
    QNetworkConfigurationManager manager;

    QVector<QString> m_fileList;
};

#endif // FTPCLIENT_H
