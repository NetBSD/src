/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This program is intended to be started outside of gdb, then
   manually stopped via a signal.  */

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* Red Hat BZ PR 197584.c */

void *func (void *arg)
{
  sleep (10000);  /* Ridiculous time, but we will eventually kill it.  */
  sleep (10000);	/* RHEL3U8 kernel-2.4.21-47.EL will cut the sleep time.  */

  return NULL;	/* thread-sleep */
}

int main ()
{
  pthread_t t1,t2;
  int ret;

  ret = pthread_create (&t1, NULL, func, NULL);
  if (ret)
    fprintf(stderr, "pthread_create(): %s", strerror (ret));
  ret = pthread_create (&t2, NULL, func, NULL);
  if (ret)
    fprintf(stderr, "pthread_create(): %s", strerror (ret));

  ret = pthread_join (t1, NULL);
  if (ret)	/* first-join */
    fprintf(stderr, "pthread_join(): %s", strerror (ret));
  ret = pthread_join (t2, NULL);
  if (ret)
    fprintf(stderr, "pthread_join(): %s", strerror (ret));

  return 0;
}
