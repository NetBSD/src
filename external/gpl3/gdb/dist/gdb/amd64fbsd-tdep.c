/* Target-dependent code for FreeBSD/amd64.

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
#include "regcache.h"
#include "osabi.h"
#include "regset.h"
#include "i386fbsd-tdep.h"
#include "x86-xstate.h"

#include "amd64-tdep.h"
#include "bsd-uthread.h"
#include "fbsd-tdep.h"
#include "solib-svr4.h"

/* Support for signal handlers.  */

/* Return whether THIS_FRAME corresponds to a FreeBSD sigtramp
   routine.  */

static const gdb_byte amd64fbsd_sigtramp_code[] =
{
  0x48, 0x8d, 0x7c, 0x24, 0x10, /* lea     SIGF_UC(%rsp),%rdi */
  0x6a, 0x00,			/* pushq   $0 */
  0x48, 0xc7, 0xc0, 0xa1, 0x01, 0x00, 0x00,
				/* movq    $SYS_sigreturn,%rax */
  0x0f, 0x05                    /* syscall */
};

static int
amd64fbsd_sigtramp_p (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[sizeof amd64fbsd_sigtramp_code];

  if (!safe_frame_unwind_memory (this_frame, pc, buf, sizeof buf))
    return 0;
  if (memcmp (buf, amd64fbsd_sigtramp_code, sizeof amd64fbsd_sigtramp_code)
      != 0)
    return 0;

  return 1;
}

/* Assuming THIS_FRAME is for a BSD sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
amd64fbsd_sigcontext_addr (struct frame_info *this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp;
  gdb_byte buf[8];

  /* The `struct sigcontext' (which really is an `ucontext_t' on
     FreeBSD/amd64) lives at a fixed offset in the signal frame.  See
     <machine/sigframe.h>.  */
  get_frame_register (this_frame, AMD64_RSP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 8, byte_order);
  return sp + 16;
}

/* FreeBSD 5.1-RELEASE or later.  */

/* Mapping between the general-purpose registers in `struct reg'
   format and GDB's register cache layout.

   Note that some registers are 32-bit, but since we're little-endian
   we get away with that.  */

/* From <machine/reg.h>.  */
static int amd64fbsd_r_reg_offset[] =
{
  14 * 8,			/* %rax */
  11 * 8,			/* %rbx */
  13 * 8,			/* %rcx */
  12 * 8,			/* %rdx */
  9 * 8,			/* %rsi */
  8 * 8,			/* %rdi */
  10 * 8,			/* %rbp */
  20 * 8,			/* %rsp */
  7 * 8,			/* %r8 ...  */
  6 * 8,
  5 * 8,
  4 * 8,
  3 * 8,
  2 * 8,
  1 * 8,
  0 * 8,			/* ... %r15 */
  17 * 8,			/* %rip */
  19 * 8,			/* %eflags */
  18 * 8,			/* %cs */
  21 * 8,			/* %ss */
  -1,				/* %ds */
  -1,				/* %es */
  -1,				/* %fs */
  -1				/* %gs */
};

/* Location of the signal trampoline.  */
CORE_ADDR amd64fbsd_sigtramp_start_addr;
CORE_ADDR amd64fbsd_sigtramp_end_addr;

/* From <machine/signal.h>.  */
int amd64fbsd_sc_reg_offset[] =
{
  24 + 6 * 8,			/* %rax */
  24 + 7 * 8,			/* %rbx */
  24 + 3 * 8,			/* %rcx */
  24 + 2 * 8,			/* %rdx */
  24 + 1 * 8,			/* %rsi */
  24 + 0 * 8,			/* %rdi */
  24 + 8 * 8,			/* %rbp */
  24 + 22 * 8,			/* %rsp */
  24 + 4 * 8,			/* %r8 ...  */
  24 + 5 * 8,
  24 + 9 * 8,
  24 + 10 * 8,
  24 + 11 * 8,
  24 + 12 * 8,
  24 + 13 * 8,
  24 + 14 * 8,			/* ... %r15 */
  24 + 19 * 8,			/* %rip */
  24 + 21 * 8,			/* %eflags */
  24 + 20 * 8,			/* %cs */
  24 + 23 * 8,			/* %ss */
  -1,				/* %ds */
  -1,				/* %es */
  -1,				/* %fs */
  -1				/* %gs */
};

/* From /usr/src/lib/libc/amd64/gen/_setjmp.S.  */
static int amd64fbsd_jmp_buf_reg_offset[] =
{
  -1,				/* %rax */
  1 * 8,			/* %rbx */
  -1,				/* %rcx */
  -1,				/* %rdx */
  -1,				/* %rsi */
  -1,				/* %rdi */
  3 * 8,			/* %rbp */
  2 * 8,			/* %rsp */
  -1,				/* %r8 ...  */
  -1,
  -1,
  -1,				/* ... %r11 */
  4 * 8,			/* %r12 ...  */
  5 * 8,
  6 * 8,
  7 * 8,			/* ... %r15 */
  0 * 8				/* %rip */
};

/* Implement the core_read_description gdbarch method.  */

static const struct target_desc *
amd64fbsd_core_read_description (struct gdbarch *gdbarch,
				 struct target_ops *target,
				 bfd *abfd)
{
  return amd64_target_description (i386fbsd_core_read_xcr0 (abfd));
}

/* Similar to amd64_supply_fpregset, but use XSAVE extended state.  */

static void
amd64fbsd_supply_xstateregset (const struct regset *regset,
			       struct regcache *regcache, int regnum,
			       const void *xstateregs, size_t len)
{
  amd64_supply_xsave (regcache, regnum, xstateregs);
}

/* Similar to amd64_collect_fpregset, but use XSAVE extended state.  */

static void
amd64fbsd_collect_xstateregset (const struct regset *regset,
				const struct regcache *regcache,
				int regnum, void *xstateregs, size_t len)
{
  amd64_collect_xsave (regcache, regnum, xstateregs, 1);
}

static const struct regset amd64fbsd_xstateregset =
  {
    NULL,
    amd64fbsd_supply_xstateregset,
    amd64fbsd_collect_xstateregset
  };

/* Iterate over core file register note sections.  */

static void
amd64fbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  cb (".reg", tdep->sizeof_gregset, &i386_gregset, NULL, cb_data);
  cb (".reg2", tdep->sizeof_fpregset, &amd64_fpregset, NULL, cb_data);
  cb (".reg-xstate", X86_XSTATE_SIZE(tdep->xcr0),
      &amd64fbsd_xstateregset, "XSAVE extended state", cb_data);
}

static void
amd64fbsd_supply_uthread (struct regcache *regcache,
			  int regnum, CORE_ADDR addr)
{
  gdb_byte buf[8];
  int i;

  gdb_assert (regnum >= -1);

  for (i = 0; i < ARRAY_SIZE (amd64fbsd_jmp_buf_reg_offset); i++)
    {
      if (amd64fbsd_jmp_buf_reg_offset[i] != -1
	  && (regnum == -1 || regnum == i))
	{
	  read_memory (addr + amd64fbsd_jmp_buf_reg_offset[i], buf, 8);
	  regcache_raw_supply (regcache, i, buf);
	}
    }
}

static void
amd64fbsd_collect_uthread (const struct regcache *regcache,
			   int regnum, CORE_ADDR addr)
{
  gdb_byte buf[8];
  int i;

  gdb_assert (regnum >= -1);

  for (i = 0; i < ARRAY_SIZE (amd64fbsd_jmp_buf_reg_offset); i++)
    {
      if (amd64fbsd_jmp_buf_reg_offset[i] != -1
	  && (regnum == -1 || regnum == i))
	{
	  regcache_raw_collect (regcache, i, buf);
	  write_memory (addr + amd64fbsd_jmp_buf_reg_offset[i], buf, 8);
	}
    }
}

static void
amd64fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Generic FreeBSD support. */
  fbsd_init_abi (info, gdbarch);

  /* Obviously FreeBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  tdep->gregset_reg_offset = amd64fbsd_r_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (amd64fbsd_r_reg_offset);
  tdep->sizeof_gregset = 22 * 8;

  amd64_init_abi (info, gdbarch);

  tdep->sigtramp_p = amd64fbsd_sigtramp_p;
  tdep->sigtramp_start = amd64fbsd_sigtramp_start_addr;
  tdep->sigtramp_end = amd64fbsd_sigtramp_end_addr;
  tdep->sigcontext_addr = amd64fbsd_sigcontext_addr;
  tdep->sc_reg_offset = amd64fbsd_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (amd64fbsd_sc_reg_offset);

  tdep->xsave_xcr0_offset = I386_FBSD_XSAVE_XCR0_OFFSET;

  /* Iterate over core file register note sections.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, amd64fbsd_iterate_over_regset_sections);

  set_gdbarch_core_read_description (gdbarch,
				     amd64fbsd_core_read_description);

  /* FreeBSD provides a user-level threads implementation.  */
  bsd_uthread_set_supply_uthread (gdbarch, amd64fbsd_supply_uthread);
  bsd_uthread_set_collect_uthread (gdbarch, amd64fbsd_collect_uthread);

  /* FreeBSD uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_amd64fbsd_tdep (void);

void
_initialize_amd64fbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64,
			  GDB_OSABI_FREEBSD_ELF, amd64fbsd_init_abi);
}
