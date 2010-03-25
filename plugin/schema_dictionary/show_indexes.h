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

#ifndef PLUGIN_SCHEMA_DICTIONARY_SHOW_INDEXES_H
#define PLUGIN_SCHEMA_DICTIONARY_SHOW_INDEXES_H

class ShowIndexes : public drizzled::plugin::TableFunction
{
public:
  ShowIndexes();

  class Generator : public drizzled::plugin::TableFunction::Generator 
  {
    bool is_tables_primed;
    bool is_index_primed;
    bool is_index_part_primed;

    int32_t index_iterator;
    int32_t index_part_iterator;

    drizzled::message::Table table_proto;
    drizzled::message::Table::Index index;
    drizzled::message::Table::Index::IndexPart index_part;

    std::string table_name;

    const drizzled::message::Table& getTableProto()
    {
      return table_proto;
    }

    const drizzled::message::Table::Index& getIndex()
    {
      return index;
    }

    const drizzled::message::Table::Index::IndexPart& getIndexPart()
    {
      return index_part;
    }

    bool isTablesPrimed()
    {
      return is_tables_primed;
    }

    bool isIndexesPrimed()
    {
      return is_index_primed;
    }

    bool nextIndex();
    bool nextIndexCore();
    bool nextIndexParts();
    bool nextIndexPartsCore();

    const std::string &getTableName()
    {
      return table_name;
    }

    void fill();

  public:
    Generator(drizzled::Field **arg);
    bool populate();

  };

  Generator *generator(drizzled::Field **arg)
  {
    return new Generator(arg);
  }
};

#endif /* PLUGIN_SCHEMA_DICTIONARY_SHOW_INDEXES_H */