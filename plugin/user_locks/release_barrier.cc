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
#include "plugin/user_locks/module.h"
#include "plugin/user_locks/barrier_storage.h"

#include <string>

namespace user_locks {
namespace barriers {

int64_t Release::val_int()
{
  drizzled::String *res= args[0]->val_str(&value);

  if (not res)
  {
    null_value= true;
    return 0;
  }
  null_value= false;

  if (not res->length())
    return 0;

  boost::tribool result= Barriers::getInstance().release(Key(getSession().getSecurityContext(), res->c_str()), getSession().getSessionId());

  if (result)
  {
    Storable *list= static_cast<Storable *>(getSession().getProperty(property_key));
    assert(list);
    if (not list) // We should have been the owner if it was passed to us, this should never happen
    {
      my_error(drizzled::ER_USER_LOCKS_NOT_OWNER_OF_BARRIER, MYF(0));
      null_value= true;

      return 0;
    }
    list->erase(Key(getSession().getSecurityContext(), res->c_str()));

    return 1;
  }
  else if (not result)
  {
    my_error(drizzled::ER_USER_LOCKS_NOT_OWNER_OF_BARRIER, MYF(0));
  }
  else
  {
    my_error(drizzled::ER_USER_LOCKS_UNKNOWN_BARRIER, MYF(0));
  }

  null_value= true;

  return 0;
}

} /* namespace barriers */
} /* namespace user_locks */