/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
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

#ifndef DRIZZLED_ERRMSG_H
#define DRIZZLED_ERRMSG_H

#include <drizzled/plugin/error_message_handler.h>

// need stdarg for va_list
#include <stdarg.h>

void add_errmsg_handler(Error_message_handler *handler);
void remove_errmsg_handler(Error_message_handler *handler);

bool errmsg_vprintf (Session *session, int priority, char const *format, va_list ap);

#endif /* DRIZZLED_ERRMSG_H */