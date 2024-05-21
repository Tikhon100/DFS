    #include "renameform.h"
#include "ui_renameform.h"

renameForm::renameForm(QWidget *parent, QString filename)
    : QDialog(parent)
    , ui(new Ui::renameForm)
{
    this->renameFileName = filename;
    ui->setupUi(this);
    this->ui->label->setText(this->ui->label->text() + filename);

}

renameForm::~renameForm()
{
    delete ui;
}


QString renameForm::getRenameFileName(){
    return this->renameFileName;
}

void renameForm::setRenameFileName(QString str){
    this->renameFileName = str;
}


void renameForm::okButtonClicked(){
    this->newName = this->ui->lineEdit->text();
    this->nameisEntered = true;
    this->close();
}

void renameForm::cancelButtonClicked(){
    this->nameisEntered = false;
    this->close();
}
