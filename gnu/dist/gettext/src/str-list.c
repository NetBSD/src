/* GNU gettext - internationalization aids
   Copyright (C) 1995, 1998 Free Software Foundation, Inc.

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
# include "config.h"
#endif

#include <stdio.h>

#include "system.h"
#include "str-list.h"


string_list_ty *
string_list_alloc ()
{
  string_list_ty *slp;

  slp = (string_list_ty *) xmalloc (sizeof (*slp));
  slp->item = NULL;
  slp->nitems = 0;
  slp->nitems_max = 0;

  return slp;
}


void
string_list_append (slp, s)
     string_list_ty *slp;
     const char *s;
{
  /* Grow the list.  */
  if (slp->nitems >= slp->nitems_max)
    {
      size_t nbytes;

      slp->nitems_max = slp->nitems_max * 2 + 4;
      nbytes = slp->nitems_max * sizeof (slp->item[0]);
      slp->item = (const char **) xrealloc (slp->item, nbytes);
    }

  /* Add a copy of the string to the end of the list.  */
  slp->item[slp->nitems++] = xstrdup (s);
}


void
string_list_append_unique (slp, s)
     string_list_ty *slp;
     const char *s;
{
  size_t j;

  /* Do not if the string is already in the list.  */
  for (j = 0; j < slp->nitems; ++j)
    if (strcmp (slp->item[j], s) == 0)
      return;

  /* Grow the list.  */
  if (slp->nitems >= slp->nitems_max)
    {
      slp->nitems_max = slp->nitems_max * 2 + 4;
      slp->item = (const char **) xrealloc (slp->item,
					    slp->nitems_max
					    * sizeof (slp->item[0]));
    }

  /* Add a copy of the string to the end of the list.  */
  slp->item[slp->nitems++] = xstrdup (s);
}


void
string_list_free (slp)
     string_list_ty *slp;
{
  size_t j;

  for (j = 0; j < slp->nitems; ++j)
    free ((char *) slp->item[j]);
  if (slp->item != NULL)
    free (slp->item);
  free (slp);
}


char *
string_list_join (slp)
     const string_list_ty *slp;
{
  size_t len;
  size_t j;
  char *result;
  size_t pos;

  len = 1;
  for (j = 0; j < slp->nitems; ++j)
    {
      if (j)
	++len;
      len += strlen (slp->item[j]);
    }
  result = xmalloc (len);
  pos = 0;
  for (j = 0; j < slp->nitems; ++j)
    {
      if (j)
	result[pos++] = ' ';
      len = strlen (slp->item[j]);
      memcpy (result + pos, slp->item[j], len);
      pos += len;
    }
  result[pos] = 0;
  return result;
}


int
string_list_member (slp, s)
     const string_list_ty *slp;
     const char *s;
{
 size_t j;

  for (j = 0; j < slp->nitems; ++j)
    if (strcmp (slp->item[j], s) == 0)
      return 1;
  return 0;
}
