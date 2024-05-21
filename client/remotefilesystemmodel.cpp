#include "remotefilesystemmodel.h"

RemoteFileSystemModel::RemoteFileSystemModel(QObject* parent)
    : QAbstractItemModel(parent), rootPath("/") {
}

RemoteFileSystemModel::~RemoteFileSystemModel() {
}



Qt::ItemFlags RemoteFileSystemModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant RemoteFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return section == 0 ? tr("Name") : QVariant();
    }

    return QVariant();
}

QModelIndex RemoteFileSystemModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    QString parentPath = parent.isValid() ? parent.data(Qt::UserRole).toString() : rootPath;
    QStringList entries = directoryContents.value(parentPath);

    if (row >= entries.size()) {
        return QModelIndex();
    }

    QString path = parentPath + "/" + entries.at(row);
    return createIndex(row, column, qPrintable(path));
}

QModelIndex RemoteFileSystemModel::parent(const QModelIndex& index) const {
    if (!index.isValid()) {
        return QModelIndex();
    }

    QString path = index.data(Qt::UserRole).toString();
    QDir dir = QDir(path);
    dir.cdUp();

    if (dir.path() == rootPath) {
        return QModelIndex();
    }

    return createIndex(dir.dirName().toInt(), 0, qPrintable(dir.path()));
}

int RemoteFileSystemModel::rowCount(const QModelIndex& parent) const {
    QString parentPath = parent.isValid() ? parent.data(Qt::UserRole).toString() : rootPath;
    return directoryContents.value(parentPath).size();
}

int RemoteFileSystemModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 1;
}

void RemoteFileSystemModel::fetchDirectoryContents(const QString& path) {
    if (!directoryContents.contains(path)) {
        QStringList contents = getDirectoryContents(path);
        directoryContents.insert(path, contents);
    }
}

QStringList RemoteFileSystemModel::getDirectoryContents(const QString& path) {
    // Здесь вы должны реализовать код для получения содержимого директории с сервера
    // Например, отправить запрос на сервер и получить ответ с содержимым директории
    QStringList contents;
    // ... код для получения содержимого директории с сервера
    return contents;
}


