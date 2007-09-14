/* Target-dependent code for NetBSD/amd64.

   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "defs.h"
#include "arch-utils.h"
#include "frame.h"
#include "gdbcore.h"
#include "osabi.h"
#include "symtab.h"

#include "gdb_assert.h"

#include "amd64-tdep.h"
#include "nbsd-tdep.h"
#include "solib-svr4.h"
#include "trad-frame.h"
#include "frame-unwind.h"

/* Support for signal handlers.  */

/* Return whether the frame preceding NEXT_FRAME corresponds to a
   NetBSD sigtramp routine.  */

static int
amd64nbsd_sigtramp_p (struct frame_info *next_frame)
{
  CORE_ADDR pc = frame_pc_unwind (next_frame);
  char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  return nbsd_pc_in_sigtramp (pc, name);
}

/* Assuming NEXT_FRAME is preceded by a frame corresponding to a
   NetBSD sigtramp routine, return the address of the associated
   mcontext structure.  */

static CORE_ADDR
amd64nbsd_mcontext_addr (struct frame_info *next_frame)
{
  CORE_ADDR addr;

  /* The register %r15 points at `struct ucontext' upon entry of a
     signal trampoline.  */
  addr = frame_unwind_register_unsigned (next_frame, AMD64_R15_REGNUM);

  /* The mcontext structure lives as offset 56 in `struct ucontext'.  */
  return addr + 56;
}

/* NetBSD 2.0 or later.  */

/* Mapping between the general-purpose registers in `struct reg'
   format and GDB's register cache layout.  */

/* From <machine/reg.h>.  */
int amd64nbsd_r_reg_offset[] =
{
  14 * 8,			/* %rax */
  13 * 8,			/* %rbx */
  3 * 8,			/* %rcx */
  2 * 8,			/* %rdx */
  1 * 8,			/* %rsi */
  0 * 8,			/* %rdi */
  12 * 8,			/* %rbp */
  24 * 8,			/* %rsp */
  4 * 8,			/* %r8 .. */
  5 * 8,
  6 * 8,
  7 * 8,
  8 * 8,
  9 * 8,
  10 * 8,
  11 * 8,			/* ... %r15 */
  21 * 8,			/* %rip */
  23 * 8,			/* %eflags */
  22 * 8,			/* %cs */
  25 * 8,			/* %ss */
  18 * 8,			/* %ds */
  17 * 8,			/* %es */
  16 * 8,			/* %fs */
  15 * 8			/* %gs */
};

/* Kernel debuging support */

#define amd64nbsd_tf_reg_offset amd64nbsd_r_reg_offset

static struct trad_frame_cache *
amd64nbsd_trapframe_cache(struct frame_info *next_frame, void **this_cache)
{
  struct trad_frame_cache *cache;
  CORE_ADDR func, sp, addr;
  ULONGEST cs;
  char *name;
  int i;

  if (*this_cache)
    return *this_cache;

  cache = trad_frame_cache_zalloc (next_frame);
  *this_cache = cache;

  func = frame_func_unwind (next_frame);
  sp = frame_unwind_register_unsigned (next_frame, AMD64_RSP_REGNUM);

  find_pc_partial_function (func, &name, NULL, NULL);
  if (name && strncmp (name, "Xintr", 5) == 0)
    addr = sp + 8;		/* It's an interrupt frame.  */
  else
    addr = sp;

  for (i = 0; i < ARRAY_SIZE (amd64nbsd_tf_reg_offset); i++)
    if (amd64nbsd_tf_reg_offset[i] != -1)
      trad_frame_set_reg_addr (cache, i, addr + amd64nbsd_tf_reg_offset[i]);

  /* Read %cs from trap frame.  */
  addr += amd64nbsd_tf_reg_offset[AMD64_CS_REGNUM];
  cs = read_memory_unsigned_integer (addr, 8); 
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    {
      /* Trap from user space; terminate backtrace.  */
      trad_frame_set_id (cache, null_frame_id);
    }
  else
    {
      /* Construct the frame ID using the function start.  */
      trad_frame_set_id (cache, frame_id_build (sp + 16, func));
    }

  return cache;
}

static void
amd64nbsd_trapframe_this_id (struct frame_info *next_frame,
			     void **this_cache, struct frame_id *this_id)
{
  struct trad_frame_cache *cache =
    amd64nbsd_trapframe_cache (next_frame, this_cache);
  
  trad_frame_get_id (cache, this_id);
}

static void
amd64nbsd_trapframe_prev_register (struct frame_info *next_frame,
				   void **this_cache, int regnum,
				   int *optimizedp, enum lval_type *lvalp,
				   CORE_ADDR *addrp, int *realnump,
				   gdb_byte *valuep)
{
  struct trad_frame_cache *cache =
    amd64nbsd_trapframe_cache (next_frame, this_cache);

  trad_frame_get_register (cache, next_frame, regnum,
			   optimizedp, lvalp, addrp, realnump, valuep);
}

static int
amd64nbsd_trapframe_sniffer (const struct frame_unwind *self,
			     struct frame_info *next_frame,
			     void **this_prologue_cache)
{
  ULONGEST cs;
  char *name;

  /* Check Current Privilege Level and bail out if we're not executing
     in kernel space.  */
  cs = frame_unwind_register_unsigned (next_frame, AMD64_CS_REGNUM);
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    return 0;

  find_pc_partial_function (frame_pc_unwind (next_frame), &name, NULL, NULL);
  return (name && ((strcmp (name, "alltraps") == 0)
		   || (strcmp (name, "calltrap") == 0)
		   || (strncmp (name, "Xtrap", 5) == 0)
		   || (strcmp (name, "osyscall1") == 0)
		   || (strcmp (name, "Xsyscall") == 0)
		   || (strncmp (name, "Xintr", 5) == 0)
		   || (strncmp (name, "Xresume", 7) == 0)
		   || (strncmp (name, "Xstray", 6) == 0)
		   || (strncmp (name, "Xrecurse", 8) == 0)
		   || (strncmp (name, "Xsoft", 5) == 0)
		   || (strcmp (name, "Xdoreti", 5) == 0)));
}

static const struct frame_unwind amd64nbsd_trapframe_unwind = {
  /* FIXME: kettenis/20051219: This really is more like an interrupt
     frame, but SIGTRAMP_FRAME would print <signal handler called>,
     which really is not what we want here.  */
  NORMAL_FRAME,
  amd64nbsd_trapframe_this_id,
  amd64nbsd_trapframe_prev_register,
  NULL,
  amd64nbsd_trapframe_sniffer
};


static void
amd64nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Initialize general-purpose register set details first.  */
  tdep->gregset_reg_offset = amd64nbsd_r_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (amd64nbsd_r_reg_offset);
  tdep->sizeof_gregset = 26 * 8;

  amd64_init_abi (info, gdbarch);

  tdep->jb_pc_offset = 7 * 8;

  /* NetBSD has its own convention for signal trampolines.  */
  tdep->sigtramp_p = amd64nbsd_sigtramp_p;
  tdep->sigcontext_addr = amd64nbsd_mcontext_addr;
  tdep->sc_reg_offset = amd64nbsd_r_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (amd64nbsd_r_reg_offset);

  /* NetBSD uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);

  /* Unwind kernel trap frames correctly.  */
  frame_unwind_prepend_unwinder (gdbarch, &amd64nbsd_trapframe_unwind);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_amd64nbsd_tdep (void);

void
_initialize_amd64nbsd_ndep (void)
{
  /* The NetBSD/amd64 native dependent code makes this assumption.  */
  gdb_assert (ARRAY_SIZE (amd64nbsd_r_reg_offset) == AMD64_NUM_GREGS);

  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64,
			  GDB_OSABI_NETBSD_ELF, amd64nbsd_init_abi);
}
