#ifndef CUSTOMITEM_H
#define CUSTOMITEM_H
#include <QStandardItem>

class CustomItem : public QStandardItem
{
public:
    CustomItem(const QString& text, int type, int flag);
    CustomItem(const CustomItem& other); // Конструктор копирования

    int getType();
    int getIsLoaded();
    void setType(int);
    void setIsLoaded(int);
    CustomItem& operator=(const CustomItem& other);


private:
    int type;
    int isLoaded;
};


#endif // CUSTOMITEM_H
