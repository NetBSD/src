/* Builtin registers, for GDB, the GNU debugger.

   Copyright 2002 Free Software Foundation, Inc.

   Contributed by Red Hat.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef BUILTIN_REGS_H
#define BUILTIN_REGS_H

extern int builtin_reg_map_name_to_regnum (const char *str, int len);

extern const char *builtin_reg_map_regnum_to_name (int regnum);

extern struct value *value_of_builtin_reg (int regnum,
					   struct frame_info *frame);

extern void add_builtin_reg (const char *name,
			     struct value *(value) (struct frame_info * frame));

#endif
