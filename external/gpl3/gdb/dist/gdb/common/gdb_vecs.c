/* Some commonly-used VEC types.

   Copyright (C) 2012-2013 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#endif

#include "gdb_vecs.h"
#include "host-defs.h"

/* Call xfree for each element of CHAR_PTR_VEC and final VEC_free for
   CHAR_PTR_VEC itself.

   You must not modify CHAR_PTR_VEC after it got registered with this function
   by make_cleanup as the CHAR_PTR_VEC base address may change on its updates.
   Contrary to VEC_free this function does not (cannot) clear the pointer.  */

void
free_char_ptr_vec (VEC (char_ptr) *char_ptr_vec)
{
  int ix;
  char *name;

  for (ix = 0; VEC_iterate (char_ptr, char_ptr_vec, ix, name); ++ix)
    xfree (name);
  VEC_free (char_ptr, char_ptr_vec);
}

/* Extended version of dirnames_to_char_ptr_vec - additionally if *VECP is
   non-NULL the new list elements from DIRNAMES are appended to the existing
   *VECP list of entries.  *VECP address will be updated by this call.  */

void
dirnames_to_char_ptr_vec_append (VEC (char_ptr) **vecp, const char *dirnames)
{
  do
    {
      size_t this_len;
      char *next_dir, *this_dir;

      next_dir = strchr (dirnames, DIRNAME_SEPARATOR);
      if (next_dir == NULL)
	this_len = strlen (dirnames);
      else
	{
	  this_len = next_dir - dirnames;
	  next_dir++;
	}

      this_dir = xmalloc (this_len + 1);
      memcpy (this_dir, dirnames, this_len);
      this_dir[this_len] = '\0';
      VEC_safe_push (char_ptr, *vecp, this_dir);

      dirnames = next_dir;
    }
  while (dirnames != NULL);
}

/* Split DIRNAMES by DIRNAME_SEPARATOR delimiter and return a list of all the
   elements in their original order.  For empty string ("") DIRNAMES return
   list of one empty string ("") element.
   
   You may modify the returned strings.
   Read free_char_ptr_vec for its cleanup.  */

VEC (char_ptr) *
dirnames_to_char_ptr_vec (const char *dirnames)
{
  VEC (char_ptr) *retval = NULL;
  
  dirnames_to_char_ptr_vec_append (&retval, dirnames);

  return retval;
}
