#include "createfolderform.h"
#include "ui_createfolderform.h"

CreateFolderForm::CreateFolderForm(QWidget *parent,QString folderName)
    : QDialog(parent)
    , ui(new Ui::CreateFolderForm)
{
    ui->setupUi(this);
    this->ui->label->setText(this->ui->label->text()+" "+folderName);
}

CreateFolderForm::~CreateFolderForm()
{
    delete ui;
}

void CreateFolderForm::okButtonClicked(){
    if (this->ui->lineEdit->text()!=""){
        this->createFlag = 1;
        this->nameOfCreatedDir = this->ui->lineEdit->text();
        this->close();
    }
}
void CreateFolderForm::cancelButtonClicked(){
    this->createFlag = 0;
    this->close();
}

QString CreateFolderForm::getNameOfCreatedDir() const
{
    return this->nameOfCreatedDir;
}

void CreateFolderForm::setNameOfCreatedDir(const QString& name)
{
    this->nameOfCreatedDir = name;
}

bool CreateFolderForm::getCreateFlag() const
{
    return this->createFlag;
}

void CreateFolderForm::setCreateFlag(bool flag)
{
    this->createFlag = flag;
}
