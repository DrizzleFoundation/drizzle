/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
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

/**
 * @file
 *
 * Defines the API of a simple filtered replicator.
 *
 * @see drizzled/plugin/replicator.h
 * @see drizzled/plugin/applier.h
 */

#ifndef DRIZZLE_PLUGIN_FILTERED_REPLICATOR_H
#define DRIZZLE_PLUGIN_FILTERED_REPLICATOR_H

#include <drizzled/server_includes.h>
#include <drizzled/atomics.h>
#include <drizzled/plugin/command_replicator.h>
#include <drizzled/plugin/command_applier.h>

#include PCRE_HEADER

#include <vector>
#include <string>

class FilteredReplicator: public drizzled::plugin::CommandReplicator
{
public:
  FilteredReplicator(std::string name_arg,
                     const char *in_sch_filters,
                     const char *in_tab_filters);

  /** Destructor */
  ~FilteredReplicator() 
  {
    if (sch_re)
    {
      pcre_free(sch_re);
    }
    if (tab_re)
    {
      pcre_free(tab_re);
    }

    pthread_mutex_destroy(&sch_vector_lock);
    pthread_mutex_destroy(&tab_vector_lock);
    pthread_mutex_destroy(&sysvar_sch_lock);
    pthread_mutex_destroy(&sysvar_tab_lock);
  }

  /**
   * Returns whether the replicator is enabled
   */
  virtual bool isEnabled() const;

  /**
   * Replicate a Command message to an Applier.
   *
   * @note
   *
   * It is important to note that memory allocation for the 
   * supplied pointer is not guaranteed after the completion 
   * of this function -- meaning the caller can dispose of the
   * supplied message.  Therefore, replicators and appliers 
   * implementing an asynchronous replication system must copy
   * the supplied message to their own controlled memory storage
   * area.
   *
   * @param Command message to be replicated
   */
  void replicate(drizzled::plugin::CommandApplier *in_applier, 
                 drizzled::message::Command &to_replicate);
  
  /**
   * Populate the vector of schemas to filter from the
   * comma-separated list of schemas given. This method
   * clears the vector first.
   *
   * @param[in] input comma-separated filter to use
   */
  void setSchemaFilter(const std::string &input);

  /**
   * @return string of comma-separated list of schemas to filter
   */
  const std::string &getSchemaFilter() const
  {
    return sch_filter_string;
  }

  /**
   * Populate the vector of tables to filter from the
   * comma-separated list of tables given. This method
   * clears the vector first.
   *
   * @param[in] input comma-separated filter to use
   */
  void setTableFilter(const std::string &input);

  /**
   * @return string of comma-separated list of tables to filter
   */
  const std::string &getTableFilter() const
  {
    return tab_filter_string;
  }

  /**
   * Update the given system variable and release the mutex
   * associated with this system variable.
   *
   * @param[out] var_ptr the system variable to update
   */
  void updateTableSysvar(const char **var_ptr)
  {
    *var_ptr= tab_filter_string.c_str();
    pthread_mutex_unlock(&sysvar_tab_lock);
  }

  /**
   * Update the given system variable and release the mutex
   * associated with this system variable.
   *
   * @param[out] var_ptr the system variable to update
   */
  void updateSchemaSysvar(const char **var_ptr)
  {
    *var_ptr= sch_filter_string.c_str();
    pthread_mutex_unlock(&sysvar_sch_lock);
  }

private:
 
  /**
   * Given a comma-separated string, parse that string to obtain
   * each entry and add each entry to the supplied vector.
   *
   * @param[in] input a comma-separated string of entries
   * @param[out] filter a std::vector to be populated with the entries
   *                    from the input string
   */
  void populateFilter(std::string input,
                      std::vector<std::string> &filter);

  /**
   * Search the vector of schemas to filter to determine whether
   * the given schema should be filtered or not. The parameter
   * is obtained from the Command message passed to the replicator.
   *
   * @param[in] schema_name name of schema to search for
   * @return true if the given schema should be filtered; false otherwise
   */
  bool isSchemaFiltered(const std::string &schema_name);

  /**
   * Search the vector of tables to filter to determine whether
   * the given table should be filtered or not. The parameter
   * is obtained from the Command message passed to the replicator.
   *
   * @param[in] table_name name of table to search for
   * @return true if the given table should be filtered; false otherwise
   */
  bool isTableFiltered(const std::string &table_name);

  /**
   * If the command message consists of raw SQL, this method parses
   * a string representation of the raw SQL and extracts the schema
   * name and table name from that raw SQL.
   *
   * @param[in] sql std::string representation of the raw SQL
   * @param[out] schema_name parameter to be populated with the 
   *                         schema name from the parsed SQL
   * @param[out] table_name parameter to be populated with the table
   *                        name from the parsed SQL
   */
  void parseQuery(const std::string &sql,
                  std::string &schema_name,
                  std::string &table_name);

  /*
   * Vectors of the tables and schemas to filter.
   */
  std::vector<std::string> schemas_to_filter;
  std::vector<std::string> tables_to_filter;

  /*
   * Variables to contain the string representation of the
   * comma-separated lists of schemas and tables to filter.
   */
  std::string sch_filter_string;
  std::string tab_filter_string;

  /*
   * We need locks to protect the vectors when they are
   * being updated and accessed. It would be nice to use
   * r/w locks here since the vectors will mostly be 
   * accessed in a read-only fashion and will be only updated
   * rarely.
   */
  pthread_mutex_t sch_vector_lock;
  pthread_mutex_t tab_vector_lock;

  /*
   * We need a lock to protect the system variables
   * that can be updated. We have a lock for each 
   * system variable.
   */
  pthread_mutex_t sysvar_sch_lock;
  pthread_mutex_t sysvar_tab_lock;

  bool sch_regex_enabled;
  bool tab_regex_enabled;

  pcre *sch_re;
  pcre *tab_re;
};

#endif /* DRIZZLE_PLUGIN_FILTERED_REPLICATOR_H */
