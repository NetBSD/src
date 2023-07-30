/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.
 
    */


/* Frustrating header junk */

#include "build-config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "ansidecl.h"
#include "filter_filename.h"

extern void error (const char *msg, ...)
  ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);

#define ASSERT(EXPRESSION) \
do { \
  if (!(EXPRESSION)) { \
    error("%s:%d: assertion failed - %s\n", \
	  filter_filename (__FILE__), __LINE__, #EXPRESSION); \
  } \
} while (0)

#define ZALLOC(TYPE) (TYPE*)zalloc(sizeof(TYPE))
#define NZALLOC(TYPE,N) ((TYPE*) zalloc (sizeof(TYPE) * (N)))

extern void *zalloc
(long size);

extern void dumpf (int indent, const char *msg, ...)
  ATTRIBUTE_PRINTF (2, 3);

extern unsigned target_a2i
(int ms_bit_nr,
 const char *a);

extern unsigned i2target
(int ms_bit_nr,
 unsigned bit);

extern unsigned a2i
(const char *a);

/* Try looking for name in the map table (returning the corresponding
   integer value).  If that fails, try converting the name into an
   integer */

typedef struct _name_map {
  const char *name;
  int i;
} name_map;

extern int name2i
(const char *name,
 const name_map *map);

extern const char *i2name
(const int i,
 const name_map *map);
