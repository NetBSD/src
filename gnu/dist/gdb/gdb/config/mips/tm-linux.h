/* Target-dependent definitions for GNU/Linux MIPS.

   Copyright 2001, 2002 Free Software Foundation, Inc.

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

#ifndef TM_MIPSLINUX_H
#define TM_MIPSLINUX_H

#include "mips/tm-mips.h"

/* We don't want to inherit tm-mips.h's shared library trampoline code.  */

#undef IN_SOLIB_CALL_TRAMPOLINE
#undef IN_SOLIB_RETURN_TRAMPOLINE
#undef SKIP_TRAMPOLINE_CODE
#undef IGNORE_HELPER_CALL

/* GNU/Linux MIPS has __SIGRTMAX == 127.  */

#ifndef REALTIME_LO
#define REALTIME_LO 32
#define REALTIME_HI 128
#endif

#include "config/tm-linux.h"

/* Use target_specific function to define link map offsets.  */

extern struct link_map_offsets *mips_linux_svr4_fetch_link_map_offsets (void);
#define SVR4_FETCH_LINK_MAP_OFFSETS() \
  mips_linux_svr4_fetch_link_map_offsets ()

/* Details about jmp_buf.  */

#define MIPS_LINUX_JB_ELEMENT_SIZE 4
#define MIPS_LINUX_JB_PC 0

/* Figure out where the longjmp will land.  Slurp the arguments out of the
   stack.  We expect the first arg to be a pointer to the jmp_buf structure
   from which we extract the pc (JB_PC) that we will land at.  The pc is
   copied into ADDR.  This routine returns 1 on success.  */

#define GET_LONGJMP_TARGET(ADDR) mips_linux_get_longjmp_target(ADDR)
extern int mips_linux_get_longjmp_target (CORE_ADDR *);

/* We do single stepping in software.  */

#define SOFTWARE_SINGLE_STEP_P() 1
#define SOFTWARE_SINGLE_STEP(sig,bp_p) mips_software_single_step (sig, bp_p)

/* FIXME: This still needs to be implemented.  */

#undef  IN_SIGTRAMP
#define IN_SIGTRAMP(pc, name)	(0)

#endif /* TM_MIPSLINUX_H */
