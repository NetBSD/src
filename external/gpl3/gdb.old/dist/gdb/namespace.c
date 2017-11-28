/* Code dealing with "using" directives for GDB.
   Copyright (C) 2003-2016 Free Software Foundation, Inc.

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

#include "defs.h"
#include "namespace.h"

/* Add a using directive to USING_DIRECTIVES.  If the using directive
   in question has already been added, don't add it twice.

   Create a new struct using_direct which imports the namespace SRC
   into the scope DEST.  ALIAS is the name of the imported namespace
   in the current scope.  If ALIAS is NULL then the namespace is known
   by its original name.  DECLARATION is the name if the imported
   varable if this is a declaration import (Eg. using A::x), otherwise
   it is NULL.  EXCLUDES is a list of names not to import from an
   imported module or NULL.  If COPY_NAMES is non-zero, then the
   arguments are copied into newly allocated memory so they can be
   temporaries.  For EXCLUDES the VEC pointers are copied but the
   pointed to characters are not copied.  */

void
add_using_directive (struct using_direct **using_directives,
		     const char *dest,
		     const char *src,
		     const char *alias,
		     const char *declaration,
		     VEC (const_char_ptr) *excludes,
		     int copy_names,
		     struct obstack *obstack)
{
  struct using_direct *current;
  struct using_direct *newobj;
  int alloc_len;

  /* Has it already been added?  */

  for (current = *using_directives; current != NULL; current = current->next)
    {
      int ix;
      const char *param;

      if (strcmp (current->import_src, src) != 0)
	continue;
      if (strcmp (current->import_dest, dest) != 0)
	continue;
      if ((alias == NULL && current->alias != NULL)
	  || (alias != NULL && current->alias == NULL)
	  || (alias != NULL && current->alias != NULL
	      && strcmp (alias, current->alias) != 0))
	continue;
      if ((declaration == NULL && current->declaration != NULL)
	  || (declaration != NULL && current->declaration == NULL)
	  || (declaration != NULL && current->declaration != NULL
	      && strcmp (declaration, current->declaration) != 0))
	continue;

      /* Compare the contents of EXCLUDES.  */
      for (ix = 0; VEC_iterate (const_char_ptr, excludes, ix, param); ix++)
	if (current->excludes[ix] == NULL
	    || strcmp (param, current->excludes[ix]) != 0)
	  break;
      if (ix < VEC_length (const_char_ptr, excludes)
	  || current->excludes[ix] != NULL)
	continue;

      /* Parameters exactly match CURRENT.  */
      return;
    }

  alloc_len = (sizeof(*newobj)
	       + (VEC_length (const_char_ptr, excludes)
		  * sizeof(*newobj->excludes)));
  newobj = (struct using_direct *) obstack_alloc (obstack, alloc_len);
  memset (newobj, 0, sizeof (*newobj));

  if (copy_names)
    {
      newobj->import_src
	= (const char *) obstack_copy0 (obstack, src, strlen (src));
      newobj->import_dest
	= (const char *) obstack_copy0 (obstack, dest, strlen (dest));
    }
  else
    {
      newobj->import_src = src;
      newobj->import_dest = dest;
    }

  if (alias != NULL && copy_names)
    newobj->alias
      = (const char *) obstack_copy0 (obstack, alias, strlen (alias));
  else
    newobj->alias = alias;

  if (declaration != NULL && copy_names)
    newobj->declaration
      = (const char *) obstack_copy0 (obstack, declaration,
				      strlen (declaration));
  else
    newobj->declaration = declaration;

  memcpy (newobj->excludes, VEC_address (const_char_ptr, excludes),
	  VEC_length (const_char_ptr, excludes) * sizeof (*newobj->excludes));
  newobj->excludes[VEC_length (const_char_ptr, excludes)] = NULL;

  newobj->next = *using_directives;
  *using_directives = newobj;
}
