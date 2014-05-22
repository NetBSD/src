/* Target-dependent code for GNU/Linux UltraSPARC.

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
#include "dwarf2-frame.h"
#include "regset.h"
#include "regcache.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "xml-syscall.h"
#include "linux-tdep.h"

/* The syscall's XML filename for sparc 64-bit.  */
#define XML_SYSCALL_FILENAME_SPARC64 "syscalls/sparc64-linux.xml"

#include "sparc64-tdep.h"

/* Signal trampoline support.  */

static void sparc64_linux_sigframe_init (const struct tramp_frame *self,
					 struct frame_info *this_frame,
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
			     struct frame_info *this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  CORE_ADDR base, addr, sp_addr;
  int regnum;

  base = get_frame_register_unsigned (this_frame, SPARC_O1_REGNUM);
  base += 128;

  /* Offsets from <bits/sigcontext.h>.  */

  /* Since %g0 is always zero, keep the identity encoding.  */
  addr = base + 8;
  sp_addr = base + ((SPARC_SP_REGNUM - SPARC_G0_REGNUM) * 8);
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

  base = get_frame_register_unsigned (this_frame, SPARC_SP_REGNUM);
  if (base & 1)
    base += BIAS;

  addr = get_frame_memory_unsigned (this_frame, sp_addr, 8);
  if (addr & 1)
    addr += BIAS;

  for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 8;
    }
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

/* Return the address of a system call's alternative return
   address.  */

static CORE_ADDR
sparc64_linux_step_trap (struct frame_info *frame, unsigned long insn)
{
  if (insn == 0x91d0206d)
    {
      struct gdbarch *gdbarch = get_frame_arch (frame);
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      ULONGEST sp = get_frame_register_unsigned (frame, SPARC_SP_REGNUM);
      if (sp & 1)
	sp += BIAS;

      /* The kernel puts the sigreturn registers on the stack,
	 and this is where the signal unwinding state is take from
	 when returning from a signal.

	 A siginfo_t sits 192 bytes from the base of the stack.  This
	 siginfo_t is 128 bytes, and is followed by the sigreturn
	 register save area.  The saved PC sits at a 136 byte offset
	 into there.  */

      return read_memory_unsigned_integer (sp + 192 + 128 + 136,
					   8, byte_order);
    }

  return 0;
}


const struct sparc_gregset sparc64_linux_core_gregset =
{
  32 * 8,			/* %tstate */
  33 * 8,			/* %tpc */
  34 * 8,			/* %tnpc */
  35 * 8,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  1 * 8,			/* %g1 */
  16 * 8,			/* %l0 */
  8,				/* y size */
};


static void
sparc64_linux_supply_core_gregset (const struct regset *regset,
				   struct regcache *regcache,
				   int regnum, const void *gregs, size_t len)
{
  sparc64_supply_gregset (&sparc64_linux_core_gregset,
			  regcache, regnum, gregs);
}

static void
sparc64_linux_collect_core_gregset (const struct regset *regset,
				    const struct regcache *regcache,
				    int regnum, void *gregs, size_t len)
{
  sparc64_collect_gregset (&sparc64_linux_core_gregset,
			   regcache, regnum, gregs);
}

static void
sparc64_linux_supply_core_fpregset (const struct regset *regset,
				    struct regcache *regcache,
				    int regnum, const void *fpregs, size_t len)
{
  sparc64_supply_fpregset (&sparc64_bsd_fpregset, regcache, regnum, fpregs);
}

static void
sparc64_linux_collect_core_fpregset (const struct regset *regset,
				     const struct regcache *regcache,
				     int regnum, void *fpregs, size_t len)
{
  sparc64_collect_fpregset (&sparc64_bsd_fpregset, regcache, regnum, fpregs);
}

/* Set the program counter for process PTID to PC.  */

#define TSTATE_SYSCALL	0x0000000000000020ULL

static void
sparc64_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  ULONGEST state;

  regcache_cooked_write_unsigned (regcache, tdep->pc_regnum, pc);
  regcache_cooked_write_unsigned (regcache, tdep->npc_regnum, pc + 4);

  /* Clear the "in syscall" bit to prevent the kernel from
     messing with the PCs we just installed, if we happen to be
     within an interrupted system call that the kernel wants to
     restart.

     Note that after we return from the dummy call, the TSTATE et al.
     registers will be automatically restored, and the kernel
     continues to restart the system call at this point.  */
  regcache_cooked_read_unsigned (regcache, SPARC64_STATE_REGNUM, &state);
  state &= ~TSTATE_SYSCALL;
  regcache_cooked_write_unsigned (regcache, SPARC64_STATE_REGNUM, state);
}

static LONGEST
sparc64_linux_get_syscall_number (struct gdbarch *gdbarch,
				  ptid_t ptid)
{
  struct regcache *regcache = get_thread_regcache (ptid);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The content of a register.  */
  gdb_byte buf[8];
  /* The result.  */
  LONGEST ret;

  /* Getting the system call number from the register.
     When dealing with the sparc architecture, this information
     is stored at the %g1 register.  */
  regcache_cooked_read (regcache, SPARC_G1_REGNUM, buf);

  ret = extract_signed_integer (buf, 8, byte_order);

  return ret;
}



static void
sparc64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  linux_init_abi (info, gdbarch);

  tdep->gregset = regset_alloc (gdbarch, sparc64_linux_supply_core_gregset,
				sparc64_linux_collect_core_gregset);
  tdep->sizeof_gregset = 288;

  tdep->fpregset = regset_alloc (gdbarch, sparc64_linux_supply_core_fpregset,
				 sparc64_linux_collect_core_fpregset);
  tdep->sizeof_fpregset = 280;

  tramp_frame_prepend_unwinder (gdbarch, &sparc64_linux_rt_sigframe);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_append_unwinders (gdbarch);

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

  /* Make sure we can single-step over signal return system calls.  */
  tdep->step_trap = sparc64_linux_step_trap;

  set_gdbarch_write_pc (gdbarch, sparc64_linux_write_pc);

  /* Functions for 'catch syscall'.  */
  set_xml_syscall_file_name (XML_SYSCALL_FILENAME_SPARC64);
  set_gdbarch_get_syscall_number (gdbarch,
                                  sparc64_linux_get_syscall_number);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
extern void _initialize_sparc64_linux_tdep (void);

void
_initialize_sparc64_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_sparc, bfd_mach_sparc_v9,
			  GDB_OSABI_LINUX, sparc64_linux_init_abi);
}
