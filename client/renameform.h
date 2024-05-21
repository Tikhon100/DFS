#ifndef RENAMEFORM_H
#define RENAMEFORM_H

#include <QDialog>

namespace Ui {
class renameForm;
}

class renameForm : public QDialog
{
    Q_OBJECT

public:
    explicit renameForm(QWidget *parent = nullptr,QString filename = "");
    ~renameForm();
    QString getRenameFileName();
    void setRenameFileName(QString);
    bool isNameEntered() {
        return nameisEntered;
    }

    void setNameEntered(bool value) {
        nameisEntered = value;
    }
    bool getNameEntered(){
        return this->nameisEntered;
    }

    QString getNewName(){
        return this->newName;
    }

private:
    Ui::renameForm *ui;
    QString renameFileName;
    QString newName;
    bool nameisEntered;

public slots:
    void okButtonClicked();
    void cancelButtonClicked();
};

#endif // RENAMEFORM_H
