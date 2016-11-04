/* Target-dependent code for NetBSD/amd64.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "frame.h"
#include "gdbcore.h"
#include "osabi.h"
#include "symtab.h"

#include "amd64-tdep.h"
#include "nbsd-tdep.h"
#include "solib-svr4.h"
#include "trad-frame.h"
#include "frame-unwind.h"

/* Support for signal handlers.  */

/* Return whether THIS_FRAME corresponds to a NetBSD sigtramp
   routine.  */

static int
amd64nbsd_sigtramp_p (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  return nbsd_pc_in_sigtramp (pc, name);
}

/* Assuming THIS_FRAME corresponds to a NetBSD sigtramp routine,
   return the address of the associated mcontext structure.  */

static CORE_ADDR
amd64nbsd_mcontext_addr (struct frame_info *this_frame)
{
  CORE_ADDR addr;

  /* The register %r15 points at `struct ucontext' upon entry of a
     signal trampoline.  */
  addr = get_frame_register_unsigned (this_frame, AMD64_R15_REGNUM);

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
  4 * 8,			/* %r8 ..  */
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

/* Kernel debugging support */
static const int amd64nbsd_tf_reg_offset[] =
{
  18 * 8,			/* %rax */
  17 * 8,			/* %rbx */
  10 * 8,			/* %rcx */
  2 * 8,			/* %rdx */
  1 * 8,			/* %rsi */
  0 * 8,			/* %rdi */
  16 * 8,			/* %rbp */
  28 * 8,			/* %rsp */
  4 * 8,			/* %r8 .. */
  5 * 8,			
  3 * 8,			
  11 * 8,			
  12 * 8,			
  13 * 8,			
  14 * 8,			
  15 * 8,			/* ... %r15 */
  25 * 8,			/* %rip */
  27 * 8,			/* %eflags */
  26 * 8,			/* %cs */
  29 * 8,			/* %ss */
  22 * 8,			/* %ds */
  21 * 8,			/* %es */
  20 * 8,			/* %fs */
  19 * 8,			/* %gs */
};

static struct trad_frame_cache *
amd64nbsd_trapframe_cache(struct frame_info *this_frame, void **this_cache)
{
  struct trad_frame_cache *cache;
  CORE_ADDR func, sp, addr;
  ULONGEST cs = 0, rip = 0;
  const char *name;
  int i;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (*this_cache)
    return (struct trad_frame_cache *)*this_cache;

  cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = cache;

  func = get_frame_func (this_frame);
  sp = get_frame_register_unsigned (this_frame, AMD64_RSP_REGNUM);

  find_pc_partial_function (func, &name, NULL, NULL);

  /* There is an extra 'call' in the interrupt sequence - ignore the extra
   * return address */
  if (name && strncmp (name, "Xintr", 5) == 0)
    addr = sp + 8;		/* It's an interrupt frame.  */
  else
    addr = sp;

  for (i = 0; i < ARRAY_SIZE (amd64nbsd_tf_reg_offset); i++)
    if (amd64nbsd_tf_reg_offset[i] != -1)
      trad_frame_set_reg_addr (cache, i, addr + amd64nbsd_tf_reg_offset[i]);

  /* Read %cs and %rip when we have the addresses to hand */
  cs = read_memory_unsigned_integer (addr
    + amd64nbsd_tf_reg_offset[AMD64_CS_REGNUM], 8, byte_order);
  rip = read_memory_unsigned_integer (addr
    + amd64nbsd_tf_reg_offset[AMD64_RIP_REGNUM], 8, byte_order);

  /* The trap frame layout was changed lf the %rip value is less than 2^16 it
   * is almost certainly the %ss of the old format. */
  if (rip < (1 << 16))
    {

      for (i = 0; i < ARRAY_SIZE (amd64nbsd_tf_reg_offset); i++)
        {

          if (amd64nbsd_tf_reg_offset[i] == -1)
            continue;

          trad_frame_set_reg_addr (cache, i, addr + amd64nbsd_r_reg_offset[i]);

          /* Read %cs when we have the address to hand */
          if (i == AMD64_CS_REGNUM)
	    cs = read_memory_unsigned_integer (addr + amd64nbsd_r_reg_offset[i],
	    8, byte_order);
        }
    }

  if ((cs & I386_SEL_RPL) == I386_SEL_UPL ||
	(name && strncmp(name, "Xsoft", 5) == 0))
    {
      /* Trap from user space or soft interrupt; terminate backtrace.  */
      trad_frame_set_id (cache, outer_frame_id);
    }
  else
    {
      /* Construct the frame ID using the function start.  */
      trad_frame_set_id (cache, frame_id_build (sp + 16, func));
    }

  return cache;
}

static void
amd64nbsd_trapframe_this_id (struct frame_info *this_frame,
			     void **this_cache,
			     struct frame_id *this_id)
{
  struct trad_frame_cache *cache =
    amd64nbsd_trapframe_cache (this_frame, this_cache);
  
  trad_frame_get_id (cache, this_id);
}

static struct value *
amd64nbsd_trapframe_prev_register (struct frame_info *this_frame,
				   void **this_cache, int regnum) 
{
  struct trad_frame_cache *cache =
    amd64nbsd_trapframe_cache (this_frame, this_cache);

  return trad_frame_get_register (cache, this_frame, regnum);
}

static int
amd64nbsd_trapframe_sniffer (const struct frame_unwind *self,
			     struct frame_info *this_frame,
			     void **this_prologue_cache)
{
  ULONGEST cs = 0;
  const char *name;
  volatile struct gdb_exception ex;

  TRY
    {
      cs = get_frame_register_unsigned (this_frame, AMD64_CS_REGNUM);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.reason < 0 && ex.error != NOT_AVAILABLE_ERROR)
	throw_exception (ex);
    }
  END_CATCH
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    return 0;

  find_pc_partial_function (get_frame_pc (this_frame), &name, NULL, NULL);
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
		   || (strcmp (name, "Xdoreti") == 0)));
}

static const struct frame_unwind amd64nbsd_trapframe_unwind = {
  /* FIXME: kettenis/20051219: This really is more like an interrupt
     frame, but SIGTRAMP_FRAME would print <signal handler called>,
     which really is not what we want here.  */
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
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
_initialize_amd64nbsd_tdep (void)
{
  /* The NetBSD/amd64 native dependent code makes this assumption.  */
  gdb_assert (ARRAY_SIZE (amd64nbsd_r_reg_offset) == AMD64_NUM_GREGS);

  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64,
			  GDB_OSABI_NETBSD_ELF, amd64nbsd_init_abi);
}
