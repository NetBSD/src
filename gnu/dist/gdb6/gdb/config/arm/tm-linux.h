/* Target definitions for GNU/Linux on ARM, for GDB.
   Copyright 1999, 2000, 2001, 2002, 2003, 2004, 2005
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

#ifndef TM_ARMLINUX_H
#define TM_ARMLINUX_H

/* Include the common ARM target definitions.  */
#include "arm/tm-arm.h"

#include "config/tm-linux.h"

/* We've multi-arched this.  */
#undef SKIP_TRAMPOLINE_CODE

/* When we call a function in a shared library, and the PLT sends us
   into the dynamic linker to find the function's real address, we
   need to skip over the dynamic linker call.  This function decides
   when to skip, and where to skip to.  See the comments for
   SKIP_SOLIB_RESOLVER at the top of infrun.c.  */
#if 0   
#undef IN_SOLIB_DYNSYM_RESOLVE_CODE
extern CORE_ADDR arm_in_solib_dynsym_resolve_code (CORE_ADDR pc, char *name);
#define IN_SOLIB_DYNSYM_RESOLVE_CODE  arm_in_solib_dynsym_resolve_code
/* ScottB: Current definition is 
extern CORE_ADDR in_svr4_dynsym_resolve_code (CORE_ADDR pc, char *name);
#define IN_SOLIB_DYNSYM_RESOLVE_CODE  in_svr4_dynsym_resolve_code */
#endif

#endif /* TM_ARMLINUX_H */
