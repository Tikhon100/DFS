#ifndef DOWNLOADFORM_H
#define DOWNLOADFORM_H

#include <QDialog>

namespace Ui {
class downloadForm;
}

class downloadForm : public QDialog
{
    Q_OBJECT

public:
    explicit downloadForm(QWidget *parent = nullptr,QString fromPath = "", QString toPath = "");
    ~downloadForm();
    bool getUploadFlag() const;
    void setUploadFlag(bool flag);
    QString getFromPath() const;
    void setFromPath(const QString &path);
    QString getToPath();
    void setToPath(const QString &path);

private:
    Ui::downloadForm *ui;
    bool uploadFlag;
    QString fromPath;
    QString toPath;

public slots:
    void okButtonClicked();
    void cancelButtonClicked();

};

#endif // DOWNLOADFORM_H
