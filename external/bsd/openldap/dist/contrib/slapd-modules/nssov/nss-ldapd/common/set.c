/*
   set.c - set functions
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
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "set.h"
#include "dict.h"

SET *set_new(void)
{
  return (SET *)dict_new();
}

int set_add(SET *set,const char *value)
{
  return dict_put((DICT *)set,value,set);
}

int set_contains(SET *set,const char *value)
{
  return dict_get((DICT *)set,value)!=NULL;
}

void set_free(SET *set)
{
  dict_free((DICT *)set);
}

void set_loop_first(SET *set)
{
  dict_loop_first((DICT *)set);
}

const char *set_loop_next(SET *set)
{
  const char *value=NULL;
  if (dict_loop_next((DICT *)set,&value,NULL)==NULL)
    return NULL;
  return value;
}
