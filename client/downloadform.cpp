#include "downloadform.h"
#include "ui_downloadform.h"

downloadForm::downloadForm(QWidget *parent, QString fromPath, QString toPath)
    : QDialog(parent)
    , ui(new Ui::downloadForm)
{
    ui->setupUi(this);
    this->ui->label->setText(this->ui->label->text() + fromPath + " в " + toPath + "?");
}

downloadForm::~downloadForm()
{
    delete ui;
}

bool downloadForm::getUploadFlag() const {
    return this->uploadFlag;
}

void downloadForm::setUploadFlag(bool flag) {
    this->uploadFlag = flag;
}

QString downloadForm::getFromPath() const {
    return this->fromPath;
}

void downloadForm::setFromPath(const QString &path) {
    this->fromPath = path;
}

QString downloadForm::getToPath(){
    return this->toPath;
}

void downloadForm::setToPath(const QString &path) {
    this->toPath = path;
}

void downloadForm::okButtonClicked(){
    this->uploadFlag = 1;
    this->close();
}

void downloadForm::cancelButtonClicked(){
    this->uploadFlag = 0;
    this->close();
}

