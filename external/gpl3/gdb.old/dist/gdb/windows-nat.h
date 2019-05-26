/* Copyright (C) 2008-2017 Free Software Foundation, Inc.

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

#ifndef WINDOWS_NAT_H
#define WINDOWS_NAT_H

extern void windows_set_context_register_offsets (const int *offsets);

/* A pointer to a function that should return non-zero iff REGNUM
   corresponds to one of the segment registers.  */
typedef int (segment_register_p_ftype) (int regnum);

/* Set the function that should be used by this module to determine
   whether a given register is a segment register or not.  */
extern void windows_set_segment_register_p (segment_register_p_ftype *fun);

#endif

