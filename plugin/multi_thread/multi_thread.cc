/* Copyright (C) 2006 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <drizzled/server_includes.h>
#include <drizzled/gettext.h>
#include <drizzled/error.h>
#include <drizzled/plugin_scheduling.h>
#include <drizzled/connect.h>
#include <drizzled/sql_parse.h>
#include <drizzled/session.h>
#include <drizzled/connect.h>
#include <string>
using namespace std;

pthread_attr_t multi_thread_attrib;
static uint32_t max_threads;
static volatile uint32_t thread_count;

static bool add_connection(Session *session)
{
  int error;

  safe_mutex_assert_owner(&LOCK_thread_count);
  thread_count++;
  (void) pthread_mutex_unlock(&LOCK_thread_count);

  if ((error= pthread_create(&session->real_id, &multi_thread_attrib, handle_one_connection, (void*) session)))
    return true;

  return false;
}


/*
  End connection, in case when we are using 'no-threads'
*/

static bool end_thread(Session *session, bool)
{
  unlink_session(session);   /* locks LOCK_thread_count and deletes session */
  thread_count--;
  pthread_mutex_unlock(&LOCK_thread_count);

  pthread_exit(0);

  return true; // We should never reach this point
}

static uint32_t count_of_threads(void)
{
  return thread_count;
}

static int init(void *p)
{
  scheduling_st* func= (scheduling_st *)p;

  func->max_threads= max_threads; /* This will create an upper limit on max connections */
  func->add_connection= add_connection;
  func->end_thread= end_thread;
  func->count= count_of_threads;

  /* Parameter for threads created for connections */
  (void) pthread_attr_init(&multi_thread_attrib);
  (void) pthread_attr_setdetachstate(&multi_thread_attrib,
				     PTHREAD_CREATE_DETACHED);
  pthread_attr_setscope(&multi_thread_attrib, PTHREAD_SCOPE_SYSTEM);
  {
    struct sched_param tmp_sched_param;

    memset(&tmp_sched_param, 0, sizeof(tmp_sched_param));
    tmp_sched_param.sched_priority= WAIT_PRIOR;
    (void)pthread_attr_setschedparam(&multi_thread_attrib, &tmp_sched_param);
  }

  return 0;
}

static int deinit(void *)
{
  (void) pthread_mutex_lock(&LOCK_thread_count);
  while (thread_count)
  {
    pthread_cond_wait(&COND_thread_count, &LOCK_thread_count);
  }
  (void) pthread_mutex_unlock(&LOCK_thread_count);

  pthread_attr_destroy(&multi_thread_attrib);

  return 0;
}

static DRIZZLE_SYSVAR_UINT(max_threads, max_threads,
                           PLUGIN_VAR_RQCMDARG,
                           N_("Maximum number of user threads available."),
                           NULL, NULL, 2048, 1, 4048, 0);

static struct st_mysql_sys_var* system_variables[]= {
  DRIZZLE_SYSVAR(max_threads),
  NULL
};

drizzle_declare_plugin(multi_thread)
{
  DRIZZLE_SCHEDULING_PLUGIN,
  "multi_thread",
  "0.1",
  "Brian Aker",
  "One Thread Perl Session Scheduler",
  PLUGIN_LICENSE_GPL,
  init, /* Plugin Init */
  deinit, /* Plugin Deinit */
  NULL,   /* status variables */
  system_variables,   /* system variables */
  NULL    /* config options */
}
drizzle_declare_plugin_end;