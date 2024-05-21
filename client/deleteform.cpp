#include "deleteform.h"
#include "ui_deleteform.h"

deleteForm::deleteForm(QWidget *parent, QString deletedFileName)
    : QDialog(parent)
    , ui(new Ui::deleteForm)
{
    ui->setupUi(this);
    this->deleteFlag = 0;
    this->deletingFilename = deletedFileName;
    this->ui->label->setText(this->ui->label->text()+this->deletingFilename + QString("?"));
}

deleteForm::~deleteForm()
{
    delete ui;
}

bool deleteForm::getDeleteFlag(){
    return this->deleteFlag;
}
void deleteForm::setDeleteFlag( bool flag){
    this->deleteFlag = flag;
}
void deleteForm::setDeletingFileName(QString str){
    this->deletingFilename = str;
}
QString deleteForm::getDeletingFileName(){
    return this->deletingFilename;
}

void deleteForm::okButtonClicked(){
    this->deleteFlag = 1;
    this->close();
}
void deleteForm::cancelButtonClicked(){
    this->deleteFlag = 0;
    this->close();
}
