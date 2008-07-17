/*
   set.h - set functions
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

#ifndef _SET_H
#define _SET_H

#include "compat/attrs.h"

/*
   These functions provide a set of string in an unordered
   collection.
*/
typedef struct set SET;

/* Create a new instance of a set. Returns NULL
   in case of memory allocation errors. */
SET *set_new(void)
  LIKE_MALLOC MUST_USE;

/* Add a string in the set. The value is duplicated
   and can be reused by the caller.
   This function returns non-0 in case of memory allocation
   errors. All value comparisons are case insensitive. */
int set_add(SET *set,const char *value);

/* Return non-zero if the value is in the set.
   All value comparisons are case insensitive. */
int set_contains(SET *set,const char *value)
  MUST_USE;

/* Remove the set from memory. All allocated storage
   for the set and the values is freed. */
void set_free(SET *set);

/* Function for looping over all set values.
   This resets the search to the beginning of the set.
   This is required before calling set_loop_next(); */
void set_loop_first(SET *set);

/* Function for looping over all set values.
   This returns a stored value. NULL is returned when all
   values have been returned. */
const char *set_loop_next(SET *set)
  MUST_USE;

#endif /* _SET_H */
