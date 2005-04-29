/* Source file positions.
   Copyright (C) 1995-1998, 2000-2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _POS_H
#define _POS_H

/* Get size_t.  */
#include <stddef.h>

/* Position of a message within a source file.
   Used for error reporting purposes.  */
typedef struct lex_pos_ty lex_pos_ty;
struct lex_pos_ty
{
  char *file_name;
  size_t line_number;
};

#endif /* _POS_H */
