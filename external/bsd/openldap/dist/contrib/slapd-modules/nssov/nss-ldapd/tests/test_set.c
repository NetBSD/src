/*
   test_set.c - simple test for the set module
   This file is part of the nss-ldapd library.

   Copyright (C) 2008 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "common/set.h"
#include "compat/attrs.h"

/* the main program... */
int main(int UNUSED(argc),char UNUSED(*argv[]))
{
  SET *set;
  const char *val;

  /* initialize */
  set=set_new();

  /* store some entries */
  set_add(set,"key1");
  set_add(set,"key2");
  set_add(set,"key3");
  set_add(set,"KEY2");

  /* check set contents */
  assert(set_contains(set,"KeY1"));
  assert(set_contains(set,"kEy2"));
  assert(set_contains(set,"KeY3"));
  assert(!set_contains(set,"key4"));

  /* loop over set contents */
  set_loop_first(set);
  while ((val=set_loop_next(set))!=NULL)
  {
    assert( (strcasecmp(val,"key1")==0) ||
            (strcasecmp(val,"key2")==0) ||
            (strcasecmp(val,"key3")==0) );
  }

  /* free set */
  set_free(set);

  return 0;
}
