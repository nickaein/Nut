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

#include "sqlitegenerator.h"
#include "table.h"
#include "tablemodel.h"

SqliteGenerator::SqliteGenerator(Database *parent) : SqlGeneratorBase(parent)
{

}

QString SqliteGenerator::fieldType(FieldModel *field)
{
    QString ret = field->name + " ";
    QString dbType;

    switch (field->type) {
    case QVariant::Bool:
        dbType = "int";
        break;
    case QVariant::ByteArray:
        dbType = "blob";
        break;
    case QVariant::Date:
        dbType = "date";
        break;
    case QVariant::DateTime:
        dbType = "datetime";
        break;
    case QVariant::Time:
        dbType = "time";
        break;
    case QVariant::Double:
        dbType = "real";
        break;
    case QVariant::Int:
//        if(field->isPrimaryKey)
//            dbType = "INTEGER PRIMARY KEY";
//        else
            dbType = "integer";
        break;
    case QVariant::String:
        if(field->length)
            dbType = QString("varchar(%1)").arg(field->length);
        else
            dbType = "text";
        break;
    default:
        dbType = "";
    }

    return dbType;
}
