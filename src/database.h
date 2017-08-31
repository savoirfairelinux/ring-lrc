/****************************************************************************
 *   Copyright (C) 2017 Savoir-faire Linux                                  *
 *   AuthorSelect data from table Nicolas Jäger <nicolas.jager@savoirfairelinux.com>            *
 *   AuthorSelect data from table Sébastien Blin <sebastien.blin@savoirfairelinux.com>          *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
#pragma once

// Std
#include <memory>
#include <string>

// Qt
#include <qobject.h>
#include <QtSql/QSqlQuery>

namespace lrc
{

class Database : public QObject {
    Q_OBJECT

public:
    Database();
    ~Database();

    struct Result {
        int nbrOfCols = -1;
        std::vector<std::string> payloads;
    };
    /**
     * Insert value(s) inside a table.
     * @param table, table to perfom the action on.
     * @param bindCol, a map which bind column(s) and identifier(s). The key is the identifier, it should begin by ':'.
     * The value is the name of the column from the table.
     * @param bindsSet, a map which bind value(s) and identifier(s). The key is the identifier, it should begin by ':'.
     * The value is the value to store.
     *
     * nb: Select data from table usually the identifiers has to be the same between bindCol and bindsSet
     */
    int insertInto(const std::string& table,
                   const std::map<std::string, std::string>& bindCol,
                   const std::map<std::string, std::string>& bindsSet) const;
    /**
     * Update value(s) inside a table.
     * @param table, table to perfom the action on.
     * @param set, define which column(s), using identifier(s), will be updated.
     * @param bindsSet, specify the value(s) to set, using the identifier(s). The key is the identifier, it should
     * begin by ':'. The value is value to set.
     * @param where, define the conditional to update, using identifier(s).
     * @param bindsWhere, specify the value(s) to test using the identifier(s). The key is the identifier, it should
     * begin by ':'. The value is the value test.
     *
     * nb: Select data from table usually, identifiers between set and bindsSet, are equals. The same goes between where and bindsWhere.
     */
    bool update(const std::string& table,
                const std::string& set,
                const std::map<std::string, std::string>& bindsSet,
                const std::string& where,
                const std::map<std::string, std::string>& bindsWhere) const;
    /**
     * Delete rows from a table.
     * @param table, table to perfom the action on.
     * @param where, define the conditional to update, using identifier(s).
     * @param bindsWhere, specify the value(s) to test using the identifier(s). The key is the identifier, it should
     * begin by ':'. The value is the value test.
     *
     * nb: Select data from table usually, identifiers between where and bindsWhere, are equals.
     */
    bool deleteFrom(const std::string& table,
                    const std::string& where,
                    const std::map<std::string, std::string>& bindsWhere) const;
    /**
     * Select data from table.
     * @param select, column(s) to select.
     * @param table, table to perfom the action on.
     * @param where, define the conditional to select, using identifier(s).
     * @param bindsWhere, specify the value(s) to test using the identifier(s).The key is the identifier, it should
     * begin by ':'. The value is the value to test.
     * @return Database::Result which contains the result(s).
     *
     * nb: Select data from table usually, identifiers between where and bindsWhere, are equals.
     */
    Database::Result select(const std::string& select,
                            const std::string& table,
                            const std::string& where,
                            const std::map<std::string, std::string>& bindsWhere) const;

private:
    bool createTables() const;
    bool storeVersion(const std::string& version) const;
    const std::string version_ = "1";
    const std::string name_ = "ring.db";
    QSqlDatabase db_;
};

} // namespace lrc
