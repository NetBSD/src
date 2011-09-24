/* Header for GDB line completion.
   Copyright (C) 2000, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

#if !defined (LINESPEC_H)
#define LINESPEC_H 1

struct symtab;

/* An instance of this may be filled in by decode_line_1.  The caller
   must call init_linespec_result to initialize it.  */

struct linespec_result
{
  /* If non-zero, the linespec should be displayed to the user.  This
     is used by "unusual" linespecs where the ordinary `info break'
     display mechanism would do the wrong thing.  */
  int special_display;

  /* If non-NULL, an array of canonical names for returned
     symtab_and_line objects.  The array has as many elements as the
     `nelts' field in the symtabs_and_line returned by decode_line_1.
     An element in the array may be NULL.  The array and each non-NULL
     element in it are allocated with xmalloc and must be freed by the
     caller.  */
  char **canonical;
};

/* Initialize a linespec_result.  */

extern void init_linespec_result (struct linespec_result *);

extern struct symtabs_and_lines
	decode_line_1 (char **argptr, int funfirstline,
		       struct symtab *default_symtab, int default_line,
		       struct linespec_result *canonical);

#endif /* defined (LINESPEC_H) */
