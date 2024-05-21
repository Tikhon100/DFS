#include "customitem.h"

CustomItem::CustomItem(const QString& text, int type, int isLoaded) : QStandardItem(text){
    this->isLoaded = isLoaded;
    this->type = type;
}

CustomItem::CustomItem(const CustomItem& other) : QStandardItem(other), type(other.type), isLoaded(other.isLoaded) {}

void CustomItem::setIsLoaded(int isLoaded){
    this->isLoaded = isLoaded;
}

void CustomItem::setType(int type){
    this->type = type;
}

int CustomItem::getIsLoaded(){
    return this->isLoaded;
}
int CustomItem::getType(){
    return this->type;
}
CustomItem& CustomItem::operator=(const CustomItem& other)
{
    if (this != &other) // Проверяем, что мы не пытаемся присвоить объект самому себе
    {
        QStandardItem::operator=(other); // Вызываем оператор присваивания базового класса

        // Копируем данные из другого объекта
        type = other.type;
        isLoaded = other.isLoaded;
    }
    return *this;
}

