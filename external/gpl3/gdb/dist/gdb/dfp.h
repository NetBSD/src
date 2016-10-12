/* Decimal floating point support for GDB.

   Copyright (C) 2007-2016 Free Software Foundation, Inc.

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

/* Decimal floating point is one of the extension to IEEE 754, which is
   described in http://grouper.ieee.org/groups/754/revision.html and
   http://www2.hursley.ibm.com/decimal/.  It completes binary floating
   point by representing floating point more exactly.  */

#ifndef DFP_H
#define DFP_H

/* When using decimal128, this is the maximum string length + 1
 * (value comes from libdecnumber's DECIMAL128_String constant).  */
#define MAX_DECIMAL_STRING  43

extern void decimal_to_string (const gdb_byte *, int, enum bfd_endian, char *);
extern int decimal_from_string (gdb_byte *, int, enum bfd_endian,
				const char *);
extern void decimal_from_integral (struct value *from, gdb_byte *to,
				   int len, enum bfd_endian byte_order);
extern void decimal_from_floating (struct value *from, gdb_byte *to,
				   int len, enum bfd_endian byte_order);
extern DOUBLEST decimal_to_doublest (const gdb_byte *from, int len,
				     enum bfd_endian byte_order);
extern void decimal_binop (enum exp_opcode,
			   const gdb_byte *, int, enum bfd_endian,
			   const gdb_byte *, int, enum bfd_endian,
			   gdb_byte *, int, enum bfd_endian);
extern int decimal_is_zero (const gdb_byte *, int, enum bfd_endian);
extern int decimal_compare (const gdb_byte *, int, enum bfd_endian,
			    const gdb_byte *, int, enum bfd_endian);
extern void decimal_convert (const gdb_byte *, int, enum bfd_endian,
			     gdb_byte *, int, enum bfd_endian);

#endif
