#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "ftpclient.h"


namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_selectFileButton_clicked();
    void on_uploadButton_clicked();
    void on_downloadButton_clicked();
    void on_deleteButton_clicked();
    void connected2Host();
    void uploadFinished();
    void downloadFinished();
    void removalFinished();
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    void updateUi();

    Ui::Widget *ui;
    FtpClient *m_ftp;
    QString m_fileName;
    QFile *m_file;
    bool m_connected;
};

#endif // WIDGET_H
