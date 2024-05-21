#ifndef CREATEFOLDERFORM_H
#define CREATEFOLDERFORM_H

#include <QDialog>

namespace Ui {
class CreateFolderForm;
}

class CreateFolderForm : public QDialog
{
    Q_OBJECT

public:
    explicit CreateFolderForm(QWidget *parent = nullptr,QString folderName = "");
    ~CreateFolderForm();
    QString getNameOfCreatedDir() const;
    void setNameOfCreatedDir(const QString& name);

    bool getCreateFlag() const;
    void setCreateFlag(bool flag);


private:
    Ui::CreateFolderForm *ui;
    QString nameOfCreatedDir;
    bool createFlag = 0;

public slots:
    void okButtonClicked();
    void cancelButtonClicked();
};

#endif // CREATEFOLDERFORM_H
