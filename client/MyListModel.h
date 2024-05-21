#ifndef MYLISTMODEL_H
#define MYLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QVariant>

class MyListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MyListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
        // Здесь вы можете заполнить модель данными
        // В этом примере мы используем QList<QString> для хранения строковых данных
        QStringList data = { "Apple", "Banana", "Orange", "Grapes" };

        // Заполняем список модели данными из QStringList
        for (const QString& item : data) {
            m_list.append(item);
        }
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0; // Мы используем один уровень, поэтому дочерних элементов нет

        return m_list.count(); // Возвращаем количество элементов в списке
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid())
            return QVariant();

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            // Возвращаем данные для указанного индекса
            return m_list.at(index.row());
        }

        return QVariant();
    }

private:
    QList<QString> m_list;
};

#endif // MYLISTMODEL_H
