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

#include "mysys_priv.h"
#include <mystrings/m_string.h>

#define DELIM ':'

bool init_tmpdir(MY_TMPDIR *tmpdir, const char *pathlist)
{
  const char *end;
  char *copy;
  char buff[FN_REFLEN];

  pthread_mutex_init(&tmpdir->mutex, MY_MUTEX_INIT_FAST);
  if (my_init_dynamic_array(&tmpdir->full_list, sizeof(char*), 1, 5))
    goto err;
  if (!pathlist || !pathlist[0])
  {
    /* Get default temporary directory */
    pathlist=getenv("TMPDIR");	/* Use this if possible */
    if (!pathlist || !pathlist[0])
      pathlist=(char*) P_tmpdir;
  }
  do {
    uint32_t length;
    end= strrchr(pathlist, DELIM);
    if (end == NULL)
      end= pathlist+strlen(pathlist);
    strmake(buff, pathlist, (uint) (end-pathlist));
    length= cleanup_dirname(buff, buff);
    if (!(copy= my_strndup(buff, length, MYF(MY_WME))) ||
        insert_dynamic(&tmpdir->full_list, (unsigned char*) &copy))
      return(true);
    pathlist=end+1;
  } while (*end != '\0');
  freeze_size(&tmpdir->full_list);
  tmpdir->list=(char **)tmpdir->full_list.buffer;
  tmpdir->max=tmpdir->full_list.elements-1;
  tmpdir->cur=0;
  return(false);

err:
  delete_dynamic(&tmpdir->full_list);           /* Safe to free */
  pthread_mutex_destroy(&tmpdir->mutex);
  return(true);
}


char *my_tmpdir(MY_TMPDIR *tmpdir)
{
  char *dir;
  if (!tmpdir->max)
    return tmpdir->list[0];
  pthread_mutex_lock(&tmpdir->mutex);
  dir=tmpdir->list[tmpdir->cur];
  tmpdir->cur= (tmpdir->cur == tmpdir->max) ? 0 : tmpdir->cur+1;
  pthread_mutex_unlock(&tmpdir->mutex);
  return dir;
}

void free_tmpdir(MY_TMPDIR *tmpdir)
{
  uint32_t i;
  for (i=0; i<=tmpdir->max; i++)
    free(tmpdir->list[i]);
  delete_dynamic(&tmpdir->full_list);
  pthread_mutex_destroy(&tmpdir->mutex);
}
