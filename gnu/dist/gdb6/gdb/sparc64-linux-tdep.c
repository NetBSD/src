/* Target-dependent code for GNU/Linux UltraSPARC.

   Copyright 2003, 2004, 2005 Free Software Foundation, Inc.

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

#include "defs.h"
#include "frame.h"
#include "frame-unwind.h"
#include "gdbarch.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "trad-frame.h"
#include "tramp-frame.h"

#include "sparc64-tdep.h"

/* Signal trampoline support.  */

static void sparc64_linux_sigframe_init (const struct tramp_frame *self,
					 struct frame_info *next_frame,
					 struct trad_frame_cache *this_cache,
					 CORE_ADDR func);

/* See sparc-linux-tdep.c for details.  Note that 64-bit binaries only
   use RT signals.  */

static const struct tramp_frame sparc64_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x82102065, -1 },		/* mov __NR_rt_sigreturn, %g1 */
    { 0x91d0206d, -1 },		/* ta  0x6d */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  sparc64_linux_sigframe_init
};

static void
sparc64_linux_sigframe_init (const struct tramp_frame *self,
			     struct frame_info *next_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  CORE_ADDR base, addr;
  int regnum;

  base = frame_unwind_register_unsigned (next_frame, SPARC_O1_REGNUM);
  base += 128;

  /* Offsets from <bits/sigcontext.h>.  */

  /* Since %g0 is always zero, keep the identity encoding.  */
  addr = base + 8;
  for (regnum = SPARC_G1_REGNUM; regnum <= SPARC_O7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 8;
    }

  trad_frame_set_reg_addr (this_cache, SPARC64_STATE_REGNUM, addr + 0);
  trad_frame_set_reg_addr (this_cache, SPARC64_PC_REGNUM, addr + 8);
  trad_frame_set_reg_addr (this_cache, SPARC64_NPC_REGNUM, addr + 16);
  trad_frame_set_reg_addr (this_cache, SPARC64_Y_REGNUM, addr + 24);
  trad_frame_set_reg_addr (this_cache, SPARC64_FPRS_REGNUM, addr + 28);

  addr = frame_unwind_register_unsigned (next_frame, SPARC_SP_REGNUM);
  if (addr & 1)
    addr += BIAS;

  base = addr;
  for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 8;
    }
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}


static void
sparc64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tramp_frame_prepend_unwinder (gdbarch, &sparc64_linux_rt_sigframe);

  sparc64_init_abi (info, gdbarch);

  /* GNU/Linux has SVR4-style shared libraries...  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);

  /* ...which means that we need some special handling when doing
     prologue analysis.  */
  tdep->plt_entry_size = 16;

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
extern void _initialize_sparc64_linux_tdep (void);

void
_initialize_sparc64_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_sparc, bfd_mach_sparc_v9,
			  GDB_OSABI_LINUX, sparc64_linux_init_abi);
}
