/* S-record download support for GDB, the GNU debugger.
   Copyright (C) 1995-2014 Free Software Foundation, Inc.

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

struct serial;

void load_srec (struct serial *desc, const char *file, bfd_vma load_offset,
		int maxrecsize, int flags, int hashmark,
		int (*waitack) (void));

/* S-record capability flags */

/* Which record types are supported */
#define SREC_2_BYTE_ADDR 0x00000001
#define SREC_3_BYTE_ADDR 0x00000002
#define SREC_4_BYTE_ADDR 0x00000004
#define SREC_TERM_SHIFT 3

#define SREC_ALL (SREC_2_BYTE_ADDR | SREC_3_BYTE_ADDR | SREC_4_BYTE_ADDR \
		  | ((SREC_2_BYTE_ADDR | SREC_3_BYTE_ADDR | SREC_4_BYTE_ADDR) \
		     << SREC_TERM_SHIFT))
