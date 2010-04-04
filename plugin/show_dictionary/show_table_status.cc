/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2010 Brian Aker
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

#include "config.h"

#include "plugin/show_dictionary/dictionary.h"
#include "drizzled/pthread_globals.h"
#include "drizzled/my_hash.h"

using namespace drizzled;
using namespace std;

ShowTableStatus::ShowTableStatus() :
  plugin::TableFunction("DATA_DICTIONARY", "SHOW_TABLE_STATUS")
{
  add_field("Session", plugin::TableFunction::NUMBER);
  add_field("Schema");
  add_field("Name");
  add_field("Type");
  add_field("Engine");
  add_field("Version");
  add_field("Rows");
  add_field("Avg_row_length");
  add_field("Table_size");
  add_field("Auto_increment");
}

ShowTableStatus::Generator::Generator(drizzled::Field **arg) :
  drizzled::plugin::TableFunction::Generator(arg),
  is_primed(false)
{
  statement::Select *select= static_cast<statement::Select *>(getSession().lex->statement);

  schema_predicate.append(select->getShowSchema());

  pthread_mutex_lock(&LOCK_open); /* Optionally lock for remove tables from open_cahe if not in use */

  drizzled::HASH *open_cache=
    get_open_cache();

  for (uint32_t idx= 0; idx < open_cache->records; idx++ )
  {
    table= (Table*) hash_element(open_cache, idx);
    table_list.push_back(table);
  }

  for (table= getSession().temporary_tables; table; table= table->next)
  {
    if (table->getShare())
    {
      table_list.push_back(table);
    }
  }
  std::sort(table_list.begin(), table_list.end(), Table::compare);
}

ShowTableStatus::Generator::~Generator()
{
  pthread_mutex_unlock(&LOCK_open); /* Optionally lock for remove tables from open_cahe if not in use */
}

bool ShowTableStatus::Generator::nextCore()
{
  if (is_primed)
  {
    table_list_iterator++;
  }
  else
  {
    is_primed= true;
    table_list_iterator= table_list.begin();
  }

  if (table_list_iterator == table_list.end())
    return false;

  table= *table_list_iterator;

  if (checkSchemaName())
    return false;

  return true;
}

bool ShowTableStatus::Generator::next()
{
  while (not nextCore())
  {
    if (table_list_iterator != table_list.end())
      continue;

    return false;
  }

  return true;
}

bool ShowTableStatus::Generator::checkSchemaName()
{
  if (not schema_predicate.empty() && schema_predicate.compare(schema_name()))
    return true;

  return false;
}

const char *ShowTableStatus::Generator::schema_name()
{
  return table->getShare()->getSchemaName();
}

bool ShowTableStatus::Generator::populate()
{
  if (not next())
    return false;
  
  fill();

  return true;
}

void ShowTableStatus::Generator::fill()
{
  /**
    For test cases use:
    --replace_column 1 #  6 # 7 # 8 # 9 # 10 #
  */

  /* Session 1 */
  if (table->getSession())
    push(table->getSession()->getSessionId());
  else
    push(static_cast<int64_t>(0));

  /* Schema 2 */
  push(table->getShare()->getSchemaName());

  /* Name  3 */
  push(table->getShare()->getTableName());

  /* Type  4 */
  push(table->getShare()->getTableTypeAsString());

  /* Engine 5 */
  push(table->getEngine()->getName());

  /* Version 6 */
  push(static_cast<int64_t>(table->getShare()->version));

  /* Rows 7 */
  push(static_cast<uint64_t>(table->getCursor().records()));

  /* Avg_row_length 8 */
  push(table->getCursor().rowSize());

  /* Table_size 9 */
  push(table->getCursor().tableSize());

  /* Auto_increment 10 */
  push(table->getCursor().getNextInsertId());
}