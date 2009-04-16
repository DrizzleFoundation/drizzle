/* Convert UTC calendar time to simple time.  Like mktime but assumes UTC.

   Copyright (C) 1994, 1997, 2003, 2004, 2006, 2007 Free Software
   Foundation, Inc.  This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _LIBC
# include <config.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif


#ifndef _LIBC
# undef __gmtime_r
# define __gmtime_r gmtime_r
# define __mktime_internal mktime_internal
time_t __mktime_internal (struct tm *,
			  struct tm * (*) (time_t const *, struct tm *),
			  time_t *);
#endif

time_t
timegm (struct tm *tmp)
{
  static time_t gmtime_offset;
  tmp->tm_isdst = 0;
  return __mktime_internal (tmp, __gmtime_r, &gmtime_offset);
}
