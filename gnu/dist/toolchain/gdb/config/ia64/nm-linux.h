/* Native support for GNU/Linux, for GDB, the GNU debugger.
   Copyright (C) 1999
   Free Software Foundation, Inc.

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

#ifndef NM_LINUX_H
#define NM_LINUX_H

#include "nm-linux.h"

/* Note:  It seems likely that we'll have to eventually define
   FETCH_INFERIOR_REGISTERS.  But until that time, we'll make do
   with the following. */

#define CANNOT_FETCH_REGISTER(regno) ia64_cannot_fetch_register(regno)
extern int ia64_cannot_fetch_register (int regno);

#define CANNOT_STORE_REGISTER(regno) ia64_cannot_store_register(regno)
extern int ia64_cannot_store_register (int regno);

#ifdef GDBSERVER
#define REGISTER_U_ADDR(addr, blockend, regno) \
	(addr) = ia64_register_u_addr ((blockend),(regno));

extern int ia64_register_u_addr(int, int);
#endif /* GDBSERVER */

#define PTRACE_ARG3_TYPE long
#define PTRACE_XFER_TYPE long

#endif /* #ifndef NM_LINUX_H */
