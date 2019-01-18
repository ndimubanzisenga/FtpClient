#include "widget.h"
#include "ui_widget.h"

#include <QFileDialog>
#include <QDebug>


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    m_connected = false;

    ui->uploadUrlLineEdit->setText("ftp://measx.biz/Euromap63Test/");
    m_ftp = new FtpClient("ftp://measx.biz/Euromap63Test/",  "f00a951f", "vCvLU9EMJg6x8dur");
    if (!m_ftp->init()) {
        qDebug() << "Could not initialize FTP connection!";
        return;
    }

    connect(m_ftp, SIGNAL(connectedToHost(bool)), this, SLOT(connected2Host()));
    connect(m_ftp, SIGNAL(uploadedFile(bool)), this, SLOT(uploadFinished()));
    connect(m_ftp, SIGNAL(downloadedFile(bool)), this, SLOT(downloadFinished()));
    connect(m_ftp, SIGNAL(removedFile(bool)), this, SLOT(removalFinished()));

    updateUi();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::updateUi()
{
    ui->selectFileButton->setEnabled(m_connected);
    ui->uploadUrlLineEdit->setEnabled(m_connected);
    ui->uploadButton->setEnabled(m_connected);
    ui->downloadButton->setEnabled(m_connected);
    ui->deleteButton->setEnabled(m_connected);
}

void Widget::on_selectFileButton_clicked()
{
    m_fileName = QFileDialog::getOpenFileName(this, "Get Any File");
    ui->fileNameLineEdit->setText(m_fileName);
}

void Widget::on_uploadButton_clicked()
{
    qDebug() << "Uploading file";
    m_ftp->uploadFile(ui->fileNameLineEdit->text().toStdString());
}

void Widget::on_downloadButton_clicked()
{
    qDebug() << "Downloading file";
    QFileInfo fileInfo(ui->fileNameLineEdit->text());
    m_ftp->downloadFile(fileInfo.fileName().toStdString());
}

void Widget::on_deleteButton_clicked()
{
    qDebug() << "Deleting file ...";
    QFileInfo fileInfo(ui->fileNameLineEdit->text());
    m_ftp->deleteFile(fileInfo.fileName().toStdString());
}

void Widget::connected2Host()
{
    m_connected = true;
    updateUi();
}

void Widget::uploadFinished()
{
    qDebug() << "Upload finished!";
}

void Widget::downloadFinished()
{
    qDebug() << "Finished download!";
}

void Widget::removalFinished()
{
    qDebug() << "Deleted file!";
}

void Widget::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    ui->progressBar->setValue(100 * bytesSent/bytesTotal);
}
