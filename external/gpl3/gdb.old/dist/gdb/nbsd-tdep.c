/* Common target-dependent code for NetBSD systems.

   Copyright (C) 2002-2017 Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.

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

#include "defs.h"
#include "objfiles.h"
#include "solib-svr4.h"
#include "nbsd-tdep.h"

/* FIXME: kettenis/20060115: We should really eliminate the next two
   functions completely.  */

struct link_map_offsets *
nbsd_ilp32_solib_svr4_fetch_link_map_offsets (void)
{
  return svr4_ilp32_fetch_link_map_offsets ();
}

struct link_map_offsets *
nbsd_lp64_solib_svr4_fetch_link_map_offsets (void)
{
  return svr4_lp64_fetch_link_map_offsets ();
}

int
nbsd_pc_in_sigtramp (CORE_ADDR pc, const char *func_name)
{
  /* Check for libc-provided signal trampoline.  All such trampolines
     have function names which begin with "__sigtramp".  */

  return (func_name != NULL
	  && startswith (func_name, "__sigtramp"));
}

CORE_ADDR
nbsd_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol("_rtld_bind_start", NULL, NULL);
  if (msym.minsym && BMSYMBOL_VALUE_ADDRESS (msym) == pc)
    return frame_unwind_caller_pc (get_current_frame ());
  else
    return find_solib_trampoline_target (get_current_frame (), pc);
}

