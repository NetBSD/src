/* Target-specific definition for the Mitsubishi D10V
   Copyright (C) 1996,1999 Free Software Foundation, Inc.

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

/* Contributed by Martin Hunt, hunt@cygnus.com */

#define GDB_MULTI_ARCH 1

extern int d10v_register_sim_regno (int reg);
#define REGISTER_SIM_REGNO(NR) d10v_register_sim_regno((NR))

extern CORE_ADDR d10v_stack_align (CORE_ADDR size);
#define STACK_ALIGN(SIZE) (d10v_stack_align (SIZE))

#define NO_EXTRA_ALIGNMENT_NEEDED 1
