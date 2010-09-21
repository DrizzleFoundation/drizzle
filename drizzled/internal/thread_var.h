/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* Defines to make different thread packages compatible */

#ifndef DRIZZLED_INTERNAL_THREAD_VAR_H
#define DRIZZLED_INTERNAL_THREAD_VAR_H

#include <pthread.h>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace drizzled
{
namespace internal
{

static pthread_t wrapper_pthread_self()
{
  return pthread_self();
}

struct st_my_thread_var
{
  pthread_cond_t suspend;
  pthread_mutex_t mutex;
  pthread_mutex_t * volatile current_mutex;
  pthread_cond_t * volatile current_cond;
  pthread_t pthread_self;
  uint64_t id;
  int volatile abort;
  struct st_my_thread_var *next,**prev;
  void *opt_info;

  st_my_thread_var() :
    current_mutex(0),
    current_cond(0),
    id(0),
    abort(false),
    next(0),
    prev(0),
    opt_info(0)
  { 
    pthread_self= wrapper_pthread_self();
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&suspend, NULL);
  }

  ~st_my_thread_var()
  {
    pthread_cond_destroy(&suspend);
    pthread_mutex_destroy(&mutex);
  }
};

extern struct st_my_thread_var *_my_thread_var(void);
#define my_thread_var (::drizzled::internal::_my_thread_var())

} /* namespace internal */
} /* namespace drizzled */

#endif /* DRIZZLED_INTERNAL_THREAD_VAR_H */
