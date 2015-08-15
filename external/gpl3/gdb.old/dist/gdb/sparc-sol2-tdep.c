/* Target-dependent code for Solaris SPARC.

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "symtab.h"
#include "objfiles.h"
#include "osabi.h"
#include "regcache.h"
#include "target.h"
#include "trad-frame.h"

#include "gdb_assert.h"
#include <string.h>

#include "sol2-tdep.h"
#include "sparc-tdep.h"
#include "solib-svr4.h"

/* From <sys/regset.h>.  */
const struct sparc_gregset sparc32_sol2_gregset =
{
  32 * 4,			/* %psr */
  33 * 4,			/* %pc */
  34 * 4,			/* %npc */
  35 * 4,			/* %y */
  36 * 4,			/* %wim */
  37 * 4,			/* %tbr */
  1 * 4,			/* %g1 */
  16 * 4,			/* %l0 */
};

const struct sparc_fpregset sparc32_sol2_fpregset =
{
  0 * 4,			/* %f0 */
  33 * 4,			/* %fsr */
};


/* The Solaris signal trampolines reside in libc.  For normal signals,
   the function `sigacthandler' is used.  This signal trampoline will
   call the signal handler using the System V calling convention,
   where the third argument is a pointer to an instance of
   `ucontext_t', which has a member `uc_mcontext' that contains the
   saved registers.  Incidentally, the kernel passes the `ucontext_t'
   pointer as the third argument of the signal trampoline too, and
   `sigacthandler' simply passes it on.  However, if you link your
   program with "-L/usr/ucblib -R/usr/ucblib -lucb", the function
   `ucbsigvechandler' will be used, which invokes the using the BSD
   convention, where the third argument is a pointer to an instance of
   `struct sigcontext'.  It is the `ucbsigvechandler' function that
   converts the `ucontext_t' to a `sigcontext', and back.  Unless the
   signal handler modifies the `struct sigcontext' we can safely
   ignore this.  */

int
sparc_sol2_pc_in_sigtramp (CORE_ADDR pc, const char *name)
{
  return (name && (strcmp (name, "sigacthandler") == 0
		   || strcmp (name, "ucbsigvechandler") == 0
		   || strcmp (name, "__sighndlr") == 0));
}

static struct sparc_frame_cache *
sparc32_sol2_sigtramp_frame_cache (struct frame_info *this_frame,
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
  mcontext_addr = get_frame_register_unsigned (this_frame, regnum) + 40;

  cache->saved_regs[SPARC32_PSR_REGNUM].addr = mcontext_addr + 0 * 4;
  cache->saved_regs[SPARC32_PC_REGNUM].addr = mcontext_addr + 1 * 4;
  cache->saved_regs[SPARC32_NPC_REGNUM].addr = mcontext_addr + 2 * 4;
  cache->saved_regs[SPARC32_Y_REGNUM].addr = mcontext_addr + 3 * 4;

  /* Since %g0 is always zero, keep the identity encoding.  */
  for (regnum = SPARC_G1_REGNUM, addr = mcontext_addr + 4 * 4;
       regnum <= SPARC_O7_REGNUM; regnum++, addr += 4)
    cache->saved_regs[regnum].addr = addr;

  if (get_frame_memory_unsigned (this_frame, mcontext_addr + 19 * 4, 4))
    {
      /* The register windows haven't been flushed.  */
      for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
	trad_frame_set_unknown (cache->saved_regs, regnum);
    }
  else
    {
      addr = cache->saved_regs[SPARC_SP_REGNUM].addr;
      addr = get_frame_memory_unsigned (this_frame, addr, 4);
      for (regnum = SPARC_L0_REGNUM;
	   regnum <= SPARC_I7_REGNUM; regnum++, addr += 4)
	cache->saved_regs[regnum].addr = addr;
    }

  return cache;
}

static void
sparc32_sol2_sigtramp_frame_this_id (struct frame_info *this_frame,
				     void **this_cache,
				     struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc32_sol2_sigtramp_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc32_sol2_sigtramp_frame_prev_register (struct frame_info *this_frame,
					   void **this_cache,
					   int regnum)
{
  struct sparc_frame_cache *cache =
    sparc32_sol2_sigtramp_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static int
sparc32_sol2_sigtramp_frame_sniffer (const struct frame_unwind *self,
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

static const struct frame_unwind sparc32_sol2_sigtramp_frame_unwind =
{
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  sparc32_sol2_sigtramp_frame_this_id,
  sparc32_sol2_sigtramp_frame_prev_register,
  NULL,
  sparc32_sol2_sigtramp_frame_sniffer
};

/* Unglobalize NAME.  */

const const char *
sparc_sol2_static_transform_name (const char *name)
{
  /* The Sun compilers (Sun ONE Studio, Forte Developer, Sun WorkShop,
     SunPRO) convert file static variables into global values, a
     process known as globalization.  In order to do this, the
     compiler will create a unique prefix and prepend it to each file
     static variable.  For static variables within a function, this
     globalization prefix is followed by the function name (nested
     static variables within a function are supposed to generate a
     warning message, and are left alone).  The procedure is
     documented in the Stabs Interface Manual, which is distrubuted
     with the compilers, although version 4.0 of the manual seems to
     be incorrect in some places, at least for SPARC.  The
     globalization prefix is encoded into an N_OPT stab, with the form
     "G=<prefix>".  The globalization prefix always seems to start
     with a dollar sign '$'; a dot '.' is used as a seperator.  So we
     simply strip everything up until the last dot.  */

  if (name[0] == '$')
    {
      char *p = strrchr (name, '.');
      if (p)
        return p + 1;
    }

  return name;
}


void
sparc32_sol2_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

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
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* ...which means that we need some special handling when doing
     prologue analysis.  */
  tdep->plt_entry_size = 12;

  /* Solaris has kernel-assisted single-stepping support.  */
  set_gdbarch_software_single_step (gdbarch, NULL);

  frame_unwind_append_unwinder (gdbarch, &sparc32_sol2_sigtramp_frame_unwind);

  /* How to print LWP PTIDs from core files.  */
  set_gdbarch_core_pid_to_str (gdbarch, sol2_core_pid_to_str);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_sparc_sol2_tdep (void);

void
_initialize_sparc_sol2_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_sparc, 0,
			  GDB_OSABI_SOLARIS, sparc32_sol2_init_abi);
}
