/* Target-dependent code for GNU/Linux SPARC.

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
#include "floatformat.h"
#include "frame.h"
#include "frame-unwind.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "osabi.h"
#include "regcache.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "trad-frame.h"
#include "tramp-frame.h"

#include "sparc-tdep.h"

/* Signal trampoline support.  */

static void sparc32_linux_sigframe_init (const struct tramp_frame *self,
					 struct frame_info *next_frame,
					 struct trad_frame_cache *this_cache,
					 CORE_ADDR func);

/* GNU/Linux has two flavors of signals.  Normal signal handlers, and
   "realtime" (RT) signals.  The RT signals can provide additional
   information to the signal handler if the SA_SIGINFO flag is set
   when establishing a signal handler using `sigaction'.  It is not
   unlikely that future versions of GNU/Linux will support SA_SIGINFO
   for normal signals too.  */

/* When the sparc Linux kernel calls a signal handler and the
   SA_RESTORER flag isn't set, the return address points to a bit of
   code on the stack.  This code checks whether the PC appears to be
   within this bit of code.

   The instruction sequence for normal signals is encoded below.
   Checking for the code sequence should be somewhat reliable, because
   the effect is to call the system call sigreturn.  This is unlikely
   to occur anywhere other than a signal trampoline.  */

static const struct tramp_frame sparc32_linux_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x821020d8, -1 },		/* mov __NR_sugreturn, %g1 */
    { 0x91d02010, -1 },		/* ta  0x10 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  sparc32_linux_sigframe_init
};

/* The instruction sequence for RT signals is slightly different.  The
   effect is to call the system call rt_sigreturn.  */

static const struct tramp_frame sparc32_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x82102065, -1 },		/* mov __NR_rt_sigreturn, %g1 */
    { 0x91d02010, -1 },		/* ta  0x10 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  sparc32_linux_sigframe_init
};

static void
sparc32_linux_sigframe_init (const struct tramp_frame *self,
			     struct frame_info *next_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  CORE_ADDR base, addr;
  int regnum;

  base = frame_unwind_register_unsigned (next_frame, SPARC_O1_REGNUM);
  if (self == &sparc32_linux_rt_sigframe)
    base += 128;

  /* Offsets from <bits/sigcontext.h>.  */

  trad_frame_set_reg_addr (this_cache, SPARC32_PSR_REGNUM, base + 0);
  trad_frame_set_reg_addr (this_cache, SPARC32_PC_REGNUM, base + 4);
  trad_frame_set_reg_addr (this_cache, SPARC32_NPC_REGNUM, base + 8);
  trad_frame_set_reg_addr (this_cache, SPARC32_Y_REGNUM, base + 12);

  /* Since %g0 is always zero, keep the identity encoding.  */
  addr = base + 20;
  for (regnum = SPARC_G1_REGNUM; regnum <= SPARC_O7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 4;
    }

  base = addr = frame_unwind_register_unsigned (next_frame, SPARC_SP_REGNUM);
  for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 4;
    }
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}


static void
sparc32_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tramp_frame_prepend_unwinder (gdbarch, &sparc32_linux_sigframe);
  tramp_frame_prepend_unwinder (gdbarch, &sparc32_linux_rt_sigframe);

  /* GNU/Linux has SVR4-style shared libraries...  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* ...which means that we need some special handling when doing
     prologue analysis.  */
  tdep->plt_entry_size = 12;

  /* GNU/Linux doesn't support the 128-bit `long double' from the psABI.  */
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_long_double_format (gdbarch, &floatformat_ieee_double_big);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern void _initialize_sparc_linux_tdep (void);

void
_initialize_sparc_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_sparc, 0, GDB_OSABI_LINUX,
			  sparc32_linux_init_abi);
}
