#ifndef DELETEFORM_H
#define DELETEFORM_H

#include <QDialog>

namespace Ui {
class deleteForm;
}

class deleteForm : public QDialog
{
    Q_OBJECT

public:
    explicit deleteForm(QWidget *parent = nullptr,QString str = "");
    ~deleteForm();
    bool getDeleteFlag();
    void setDeleteFlag(bool);
    void setDeletingFileName(QString);
    QString getDeletingFileName();

private:
    Ui::deleteForm *ui;
    bool deleteFlag;
    QString deletingFilename;

public slots:
    void okButtonClicked();
    void cancelButtonClicked();
};

#endif // DELETEFORM_H
