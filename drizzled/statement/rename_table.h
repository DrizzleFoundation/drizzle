/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2009 Sun Microsystems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DRIZZLED_STATEMENT_RENAME_TABLE_H
#define DRIZZLED_STATEMENT_RENAME_TABLE_H

#include <drizzled/statement.h>

class Session;
class TableList;

namespace drizzled
{
namespace statement
{

class RenameTable : public Statement
{
public:
  RenameTable(Session *in_session)
    :
      Statement(in_session)
  {}

  bool execute();

private:

  bool renameTables(TableList *table_list);
  TableList *reverseTableList(TableList *table_list);
  bool rename(TableList *ren_table,
              const char *new_db,
              const char *new_table_name,
              bool skip_error);
  TableList *renameTablesInList(TableList *table_list,
                                bool skip_error);

};

} /* end namespace statement */

} /* end namespace drizzled */

#endif /* DRIZZLED_STATEMENT_RENAME_TABLE_H */