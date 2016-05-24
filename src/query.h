/**************************************************************************
**
** This file is part of Nut project.
** https://github.com/HamedMasafi/Nut
**
** Nut is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Nut is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with Nut.  If not, see <http://www.gnu.org/licenses/>.
**
**************************************************************************/

#ifndef QUERY_H
#define QUERY_H

#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QScopedPointer>
#include <QtCore/QRegularExpression>

#include "database.h"
#include "databasemodel.h"
#include "tablesetbase_p.h"
#include "sqlgeneratorbase_p.h"
#include "querybase_p.h"
#include "wherephrase.h"

QT_BEGIN_NAMESPACE

template<class T>
class NUT_EXPORT Query : public QueryBase
{
    QString _tableName;
    QString _select;
//    QString _where;
    Database *_database;
    TableSetBase *_tableSet;
    QString _joinClassName;
    QList<WherePhrase> _wheres;
public:
    Query(Database *database, TableSetBase *tableSet);

    Query(TableSet<T> *tset){
        _database = tset->database();
        _tableName = _database->tableName(T::staticMetaObject.className());
    }

    QList<T *> toList(int count = -1);
    T *first();
    int count();
    int remove();

    Query<T> *join(const QString &tableName);
    Query<T> *setWhere(WherePhrase where);
//    Query<T> *setWhere(const QString &where);
    Query<T> *orderBy(QString fieldName, QString type);

private:
    static QHash<QString, QString> _compiledCommands;
    QString compileCommand(QString command);
    QString queryText();
    QHash<QString, QString> _orders;
};

//template <typename T>
//inline Query<T> createQuery(TableSet<T> *tset)
//{
//    return Query<T>(tset);
//}

template<class T>
QHash<QString, QString>  Query<T>::_compiledCommands;

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T>::Query(Database *database, TableSetBase *tableSet) : QueryBase(database),
    _database(database), _tableSet(tableSet), _joinClassName(QString::null)
{
    _tableName = _database->tableName(T::staticMetaObject.className());
}

template<class T>
Q_OUTOFLINE_TEMPLATE QList<T *> Query<T>::toList(int count)
{
    QList<T*> result;
    _select = "*";
    qDebug()<<queryText();
    QSqlQuery q = _database->exec(_database->sqlGenertor()->selectCommand(_wheres, _orders, _tableName, _joinClassName));

    QString pk =_database->model().model(_tableName)->primaryKey();
    QVariant lastPkValue = QVariant();
    int childTypeId = 0;
    T *lastRow = 0;
    TableSetBase *childTableSet;
    QStringList masterFields = _database->model().model(_tableName)->fieldsNames();
    QStringList childFields;
    if(!_joinClassName.isNull()){
        childFields = _database->model().modelByClass(_joinClassName)->fieldsNames();
        QString joinTableName = _database->tableName(_joinClassName);
        childTypeId = _database->model().model(joinTableName)->typeId();
    }

    while (q.next()) {
        if(lastPkValue != q.value(pk)){
            T *t = new T();

            foreach (QString field, masterFields)
                t->setProperty(field.toLatin1().data(), q.value(field));

            t->setTableSet(_tableSet);
            t->setStatus(Table::FeatchedFromDB);
            t->setParent(this);

            result.append(t);
            lastRow = t;

            if(childTypeId){
                QSet<TableSetBase*> tableSets = t->tableSets;
                foreach (TableSetBase *ts, tableSets)
                    if(ts->childClassName() == _joinClassName)
                        childTableSet = ts;
            }
        }

        if(childTypeId){
            const QMetaObject *childMetaObject = QMetaType::metaObjectForType(childTypeId);
            Table *childTable = qobject_cast<Table*>(childMetaObject->newInstance());

            foreach (QString field, childFields)
                childTable->setProperty(field.toLatin1().data(), q.value(field));

            childTable->setParent(this);
            childTable->setParentTable(lastRow);
            childTable->setStatus(Table::FeatchedFromDB);
            childTable->setTableSet(childTableSet);
            childTableSet->add(childTable);
        }
        lastPkValue = q.value(pk);

        if(!--count)
            break;
    }

    deleteLater();
    return result;
}

template<class T>
Q_OUTOFLINE_TEMPLATE T *Query<T>::first()
{
    QList<T *> list = toList(1);

    if(list.count())
        return list.first();
    else
        return 0;
}

template<class T>
Q_OUTOFLINE_TEMPLATE int Query<T>::count()
{
    _select = "COUNT(*)";
    QSqlQuery q = _database->exec(queryText());
    if(q.next())
        return q.value(0).toInt();
    return 0;
}

template<class T>
Q_OUTOFLINE_TEMPLATE int Query<T>::remove()
{
    QString sql = _database->sqlGenertor()->deleteCommand(_wheres, _tableName);
//            _database->sqlGenertor()->deleteRecords(_tableName, queryText());
//    sql = compileCommand(sql);
    QSqlQuery q = _database->exec(sql);
    return q.numRowsAffected();
}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::join(const QString &tableName)
{
    _joinClassName = tableName;
    return this;
}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::setWhere(WherePhrase where)
{
    _wheres.append(where);
    return this;
}

//template<class T>
//Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::setWhere(const QString &where)
//{
//    _where = where;
//    return this;
//}

template<class T>
Q_OUTOFLINE_TEMPLATE Query<T> *Query<T>::orderBy(QString fieldName, QString type)
{
    _orders.insert(fieldName, type);
    return this;
}

template<class T>
Q_OUTOFLINE_TEMPLATE QString Query<T>::compileCommand(QString command)
{
    if(!_compiledCommands.contains(command)){
        QString q = command
                        .replace("::", ".")
                        .replace("()", "")
                        .replace("==", "=")
                        .replace("!=", "<>");

        QRegularExpression r("(\\w+)\\.(\\w+)");
        QRegularExpressionMatchIterator i = r.globalMatch(command);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString tableName = match.captured(1);
            QString fieldName = match.captured(2);
            tableName = _database->tableName(tableName);
            q = command.replace(match.captured(), tableName + "." + fieldName);
        }
        _compiledCommands.insert(command, q);
    }

    return _compiledCommands[command];
}

template<class T>
Q_OUTOFLINE_TEMPLATE QString Query<T>::queryText()
{
    QStringList orderby;
    QString q = "";//compileCommand(_where);
    foreach (WherePhrase p, _wheres) {
        if(q != "")
            q.append(" AND ");
        q.append(p.command(_database->sqlGenertor()));
    }

    QString t = _tableName;
    if(!_joinClassName.isNull()){
        QString joinTableName = _database->tableName(_joinClassName);
        RelationModel *rel = _database->model().relationByTableNames(_tableName, joinTableName);
        if(rel){
            QString pk = _database->model().model(_tableName)->primaryKey();
            t = QString("%1 INNER JOIN %2 ON (%1.%3 = %2.%4)")
                    .arg(_tableName)
                    .arg(joinTableName)
                    .arg(pk)
                    .arg(rel->localColumn);
            orderby.append(_tableName + "." + pk);
        }else{
            qWarning(QString("Relation between table %1 and class %2 (%3) not exists!")
                     .arg(_tableName)
                     .arg(_joinClassName)
                     .arg(joinTableName.isNull() ? "NULL" : joinTableName)
                     .toLatin1().data());
            _joinClassName = QString::null;
        }
    }

    QString orderText = "";
    if(_orders.count())
        foreach (QString o, _orders.keys())
            orderby.append(o + " " + _orders.value(o));

    if(orderby.count())
        orderText = " ORDER BY " + orderby.join(", ");

    QString command = QString("SELECT %1 FROM %2 %3%4")
            .arg(_select)
            .arg(t)
            .arg(q.isEmpty() ? "" : "WHERE " + q)
            .arg(orderText);

    for(int i = 0; i < _database->model().count(); i++)
        command = command.replace(_database->model().at(i)->className() + "." , _database->model().at(i)->name() + ".");

    qDebug() << command
             << _database->sqlGenertor()->selectCommand(_wheres, _orders, _tableName, _joinClassName);
    return command;
}

QT_END_NAMESPACE

#endif // QUERY_H
