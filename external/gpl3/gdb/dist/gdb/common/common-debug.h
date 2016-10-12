/* Declarations for debug printing functions.

   Copyright (C) 2014-2016 Free Software Foundation, Inc.

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

#ifndef COMMON_DEBUG_H
#define COMMON_DEBUG_H

/* Set to nonzero to enable debugging of hardware breakpoint/
   watchpoint support code.  */

extern int show_debug_regs;

/* Print a formatted message to the appropriate channel for
   debugging output for the client.  */

extern void debug_printf (const char *format, ...)
     ATTRIBUTE_PRINTF (1, 2);

/* Print a formatted message to the appropriate channel for
   debugging output for the client.  This function must be
   provided by the client.  */

extern void debug_vprintf (const char *format, va_list ap)
     ATTRIBUTE_PRINTF (1, 0);

#endif /* COMMON_DEBUG_H */
