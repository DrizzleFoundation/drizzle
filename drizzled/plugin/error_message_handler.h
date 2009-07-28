/*
 -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:

 *  Definitions required for Error Message plugin

 *  Copyright (C) 2008 Sun Microsystems
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

#ifndef DRIZZLED_PLUGIN_ERRMSG_H
#define DRIZZLED_PLUGIN_ERRMSG_H

#include <stdarg.h>
#include <string>

class Error_message_handler
{
  std::string name;
public:
  Error_message_handler(std::string name_arg): name(name_arg) {}
  Error_message_handler(const char *name_arg): name(name_arg) {}
  virtual ~Error_message_handler() {}

  std::string getName() { return name; }

  virtual bool errmsg(Session *session, int priority,
                      const char *format, va_list ap)=0;
};

#endif /* DRIZZLED_PLUGIN_ERRMSG_H */