/* Header for skipping over uninteresting files and functions when debugging.

   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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

#if !defined (SKIP_H)
#define SKIP_H

struct symtab_and_line;

/* Returns 1 if the given FUNCTION_NAME is marked for skip and shouldn't be
   stepped into.  Otherwise, returns 0.  */
int function_name_is_marked_for_skip (const char *function_name,
				    const struct symtab_and_line *function_sal);

#endif /* !defined (SKIP_H) */
