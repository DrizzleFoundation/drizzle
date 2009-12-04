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

#include <drizzled/server_includes.h>
#include <drizzled/show.h>
#include <drizzled/session.h>
#include <drizzled/statement/create_schema.h>

#include <string>

using std::string;

namespace drizzled
{

bool statement::CreateSchema::execute()
{
  string database_name(session->lex->name.str);
  NonNormalisedDatabaseName non_normalised_database_name(database_name);
  NormalisedDatabaseName normalised_database_name(non_normalised_database_name);


  if (! session->endActiveTransaction())
  {
    return true;
  }
  if (! session->lex->name.str ||
      ! normalised_database_name.is_valid())
  {
    my_error(ER_WRONG_DB_NAME, MYF(0), session->lex->name.str);
    return false;
  }
  bool res= mysql_create_db(session, normalised_database_name, &create_info, is_if_not_exists);
  return res;
}

} /* namespace drizzled */

