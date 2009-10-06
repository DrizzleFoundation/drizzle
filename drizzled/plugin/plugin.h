/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2009 Sun Microsystems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
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

#ifndef DRIZZLED_PLUGIN_PLUGIN_H
#define DRIZZLED_PLUGIN_PLUGIN_H

#include <string>
#include <vector>

namespace drizzled
{
namespace plugin
{

class Handle;

class Plugin
{
private:
  const std::string name;
  std::vector<std::string> aliases;
  bool active;
  Handle *handle;

  Plugin();
  Plugin(const Plugin&);
  Plugin& operator=(const Plugin &);
public:
  explicit Plugin(std::string in_name)
    : name(in_name),
      aliases(),
      active(false),
      handle(NULL)
  {}
  virtual ~Plugin() {}

  void activate()
  {
    active= true;
  }
 
  void deactivate()
  {
    active= false;
  }
 
  bool isActive() const
  {
    return active;
  }

  const std::string &getName() const
  {
    return name;
  } 

  const std::vector<std::string>& getAliases() const
  {
    return aliases;
  }

  void addAlias(std::string alias)
  {
    aliases.push_back(alias);
  }
 
  void setHandle(Handle *handle_arg)
  {
    handle= handle_arg;
  }

};
} /* end namespace plugin */
} /* end namespace drizzled */

#endif /* DRIZZLED_PLUGIN_PLUGIN_H */
