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

#ifndef PLUGIN_SHOW_DICTIONARY_SHOW_TABLES_H
#define PLUGIN_SHOW_DICTIONARY_SHOW_TABLES_H

class ShowTables : public drizzled::plugin::TableFunction
{
public:
  ShowTables();

  class Generator : public drizzled::plugin::TableFunction::Generator
  {
    bool is_primed;
    drizzled::TableIdentifiers set_of_identifiers;
    drizzled::TableIdentifiers::iterator table_iterator;
    std::string schema_name;

    void fill();
    bool next();
    bool nextCore();

  public:
    Generator(drizzled::Field **arg);

    bool populate();
  };

  Generator *generator(drizzled::Field **arg)
  {
    return new Generator(arg);
  }
};

#endif /* PLUGIN_SHOW_DICTIONARY_SHOW_TABLES_H */