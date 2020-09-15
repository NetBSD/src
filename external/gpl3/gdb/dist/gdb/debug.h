/* Helpers to format and print debug statements

   Copyright (C) 2020 Free Software Foundation, Inc.

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

#ifndef DEBUG_H
#define DEBUG_H

/* Print a debug statement prefixed with the module and function name, and
   with a newline at the end.  */

void ATTRIBUTE_PRINTF (3, 0)
debug_prefixed_vprintf (const char *module, const char *func, const char *format,
			va_list args);

#endif /* DEBUG_H */


