/****************************************************************************
 *   Copyright (C) 2016 by Savoir-faire Linux                               *
 *   Author : Alexandre Lision <alexandre.lision@savoirfairelinux.com>      *
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

#include <typedefs.h>
#include <itembase.h>

class ProfilePrivate;
class Person;
class Account;

class LIB_EXPORT Profile : public ItemBase
{
    Q_OBJECT
public:
    typedef QVector<Account*> Accounts;

    explicit Profile(CollectionInterface* parent = nullptr, Person* p = nullptr);
    virtual ~Profile();

    Q_PROPERTY( Accounts  accounts   READ accounts WRITE setAccounts )
    Q_PROPERTY( Person    person     READ person                     )

    //Getters
    const Accounts& accounts()  const;
          Person*   person()    const;

    //Setters
    void setAccounts(Accounts);

private:
    ProfilePrivate* d_ptr;
    Q_DECLARE_PRIVATE(Profile)
};

Q_DECLARE_METATYPE(Profile*)
