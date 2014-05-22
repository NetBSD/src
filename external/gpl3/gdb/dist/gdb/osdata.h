/* Routines for handling XML generic OS data provided by target.

   Copyright (C) 2008-2013 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef OSDATA_H
#define OSDATA_H

#include "vec.h"

typedef struct osdata_column
{
  char *name;
  char *value;
} osdata_column_s;
DEF_VEC_O(osdata_column_s);

typedef struct osdata_item
{
  VEC(osdata_column_s) *columns;
} osdata_item_s;
DEF_VEC_O(osdata_item_s);

struct osdata
{
  char *type;

  VEC(osdata_item_s) *items;
};
typedef struct osdata *osdata_p;
DEF_VEC_P(osdata_p);

struct osdata *osdata_parse (const char *xml);
void osdata_free (struct osdata *);
struct cleanup *make_cleanup_osdata_free (struct osdata *data);
struct osdata *get_osdata (const char *type);
const char *get_osdata_column (struct osdata_item *item, const char *name);
void info_osdata_command (char *type, int from_tty);

#endif /* OSDATA_H */
