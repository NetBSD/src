/* GNU gettext - internationalization aids
   Copyright (C) 1996, 1998 Free Software Foundation, Inc.

   This file was written by Peter Miller <millerp@canb.auug.org.au>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#include "system.h"
#include "dir-list.h"
#include "str-list.h"

static string_list_ty *directory;


void
dir_list_append (s)
     const char *s;
{
  if (directory == NULL)
    directory = string_list_alloc ();
  string_list_append_unique (directory, s);
}


const char *
dir_list_nth (n)
     int n;
{
  if (directory == NULL)
    dir_list_append (".");
  if (n < 0 || n >= directory->nitems)
    return NULL;
  return directory->item[n];
}
