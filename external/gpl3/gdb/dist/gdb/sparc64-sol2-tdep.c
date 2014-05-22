/* Target-dependent code for Solaris UltraSPARC.

   Copyright (C) 2003-2013 Free Software Foundation, Inc.

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
#include "frame.h"
#include "frame-unwind.h"
#include "gdbarch.h"
#include "symtab.h"
#include "objfiles.h"
#include "osabi.h"
#include "trad-frame.h"

#include "gdb_assert.h"

#include "sol2-tdep.h"
#include "sparc64-tdep.h"
#include "solib-svr4.h"

/* From <sys/regset.h>.  */
const struct sparc_gregset sparc64_sol2_gregset =
{
  32 * 8,			/* "tstate" */
  33 * 8,			/* %pc */
  34 * 8,			/* %npc */
  35 * 8,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  1 * 8,			/* %g1 */
  16 * 8,			/* %l0 */
  8				/* sizeof (%y) */
};

const struct sparc_fpregset sparc64_sol2_fpregset =
{
  0 * 8,			/* %f0 */
  33 * 8,			/* %fsr */
};


static struct sparc_frame_cache *
sparc64_sol2_sigtramp_frame_cache (struct frame_info *this_frame,
				   void **this_cache)
{
  struct sparc_frame_cache *cache;
  CORE_ADDR mcontext_addr, addr;
  int regnum;

  if (*this_cache)
    return *this_cache;

  cache = sparc_frame_cache (this_frame, this_cache);
  gdb_assert (cache == *this_cache);

  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* The third argument is a pointer to an instance of `ucontext_t',
     which has a member `uc_mcontext' that contains the saved
     registers.  */
  regnum =
    (cache->copied_regs_mask & 0x04) ? SPARC_I2_REGNUM : SPARC_O2_REGNUM;
  mcontext_addr = get_frame_register_unsigned (this_frame, regnum) + 64;

  cache->saved_regs[SPARC64_CCR_REGNUM].addr = mcontext_addr + 0 * 8;
  cache->saved_regs[SPARC64_PC_REGNUM].addr = mcontext_addr + 1 * 8;
  cache->saved_regs[SPARC64_NPC_REGNUM].addr = mcontext_addr + 2 * 8;
  cache->saved_regs[SPARC64_Y_REGNUM].addr = mcontext_addr + 3 * 8;
  cache->saved_regs[SPARC64_ASI_REGNUM].addr = mcontext_addr + 19 * 8; 
  cache->saved_regs[SPARC64_FPRS_REGNUM].addr = mcontext_addr + 20 * 8;

  /* Since %g0 is always zero, keep the identity encoding.  */
  for (regnum = SPARC_G1_REGNUM, addr = mcontext_addr + 4 * 8;
       regnum <= SPARC_O7_REGNUM; regnum++, addr += 8)
    cache->saved_regs[regnum].addr = addr;

  if (get_frame_memory_unsigned (this_frame, mcontext_addr + 21 * 8, 8))
    {
      /* The register windows haven't been flushed.  */
      for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
	trad_frame_set_unknown (cache->saved_regs, regnum);
    }
  else
    {
      CORE_ADDR sp;

      addr = cache->saved_regs[SPARC_SP_REGNUM].addr;
      sp = get_frame_memory_unsigned (this_frame, addr, 8);
      for (regnum = SPARC_L0_REGNUM, addr = sp + BIAS;
	   regnum <= SPARC_I7_REGNUM; regnum++, addr += 8)
	cache->saved_regs[regnum].addr = addr;
    }

  return cache;
}

static void
sparc64_sol2_sigtramp_frame_this_id (struct frame_info *this_frame,
				     void **this_cache,
				     struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc64_sol2_sigtramp_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc64_sol2_sigtramp_frame_prev_register (struct frame_info *this_frame,
					   void **this_cache,
					   int regnum)
{
  struct sparc_frame_cache *cache =
    sparc64_sol2_sigtramp_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static int
sparc64_sol2_sigtramp_frame_sniffer (const struct frame_unwind *self,
				     struct frame_info *this_frame,
				     void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  if (sparc_sol2_pc_in_sigtramp (pc, name))
    return 1;

  return 0;
}
static const struct frame_unwind sparc64_sol2_sigtramp_frame_unwind =
{
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  sparc64_sol2_sigtramp_frame_this_id,
  sparc64_sol2_sigtramp_frame_prev_register,
  NULL,
  sparc64_sol2_sigtramp_frame_sniffer
};



void
sparc64_sol2_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  frame_unwind_append_unwinder (gdbarch, &sparc64_sol2_sigtramp_frame_unwind);

  sparc64_init_abi (info, gdbarch);

  /* The Sun compilers (Sun ONE Studio, Forte Developer, Sun WorkShop, SunPRO)
     compiler puts out 0 instead of the address in N_SO stabs.  Starting with
     SunPRO 3.0, the compiler does this for N_FUN stabs too.  */
  set_gdbarch_sofun_address_maybe_missing (gdbarch, 1);

  /* The Sun compilers also do "globalization"; see the comment in
     sparc_sol2_static_transform_name for more information.  */
  set_gdbarch_static_transform_name
    (gdbarch, sparc_sol2_static_transform_name);

  /* Solaris has SVR4-style shared libraries...  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, sol2_skip_solib_resolver);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);

  /* ...which means that we need some special handling when doing
     prologue analysis.  */
  tdep->plt_entry_size = 16;

  /* Solaris has kernel-assisted single-stepping support.  */
  set_gdbarch_software_single_step (gdbarch, NULL);

  /* How to print LWP PTIDs from core files.  */
  set_gdbarch_core_pid_to_str (gdbarch, sol2_core_pid_to_str);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_sparc64_sol2_tdep (void);

void
_initialize_sparc64_sol2_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_sparc, bfd_mach_sparc_v9,
			  GDB_OSABI_SOLARIS, sparc64_sol2_init_abi);
}
