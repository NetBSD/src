/* Target-dependent code for NetBSD/i386.

   Copyright (C) 1988, 1989, 1991, 1992, 1994, 1996, 2000, 2001, 2002,
   2003, 2004
   Free Software Foundation, Inc.

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
#include "regcache.h"
#include "regset.h"
#include "osabi.h"
#include "symtab.h"

#include "gdb_assert.h"
#include "gdb_string.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "nbsd-tdep.h"
#include "solib-svr4.h"
#include "elf-bfd.h"		/* for header hack */
#include "trad-frame.h"		/* signal trampoline/kernel frame support */
#include "frame-unwind.h"	/* kernel frame support */
#include "tramp-frame.h"	/* signal trampoline/kernel frame support

/* From <machine/reg.h>.  */
static int i386nbsd_r_reg_offset[] =
{
  0 * 4,			/* %eax */
  1 * 4,			/* %ecx */
  2 * 4,			/* %edx */
  3 * 4,			/* %ebx */
  4 * 4,			/* %esp */
  5 * 4,			/* %ebp */
  6 * 4,			/* %esi */
  7 * 4,			/* %edi */
  8 * 4,			/* %eip */
  9 * 4,			/* %eflags */
  10 * 4,			/* %cs */
  11 * 4,			/* %ss */
  12 * 4,			/* %ds */
  13 * 4,			/* %es */
  14 * 4,			/* %fs */
  15 * 4			/* %gs */
};

static void
i386nbsd_aout_supply_regset (const struct regset *regset,
			     struct regcache *regcache, int regnum,
			     const void *regs, size_t len)
{
  const struct gdbarch_tdep *tdep = gdbarch_tdep (regset->arch);

  gdb_assert (len >= tdep->sizeof_gregset + I387_SIZEOF_FSAVE);

  i386_supply_gregset (regset, regcache, regnum, regs, tdep->sizeof_gregset);
  i387_supply_fsave (regcache, regnum, (char *) regs + tdep->sizeof_gregset);
}

static const struct regset *
i386nbsd_aout_regset_from_core_section (struct gdbarch *gdbarch,
					const char *sect_name,
					size_t sect_size)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* NetBSD a.out core dumps don't use seperate register sets for the
     general-purpose and floating-point registers.  */

  if (strcmp (sect_name, ".reg") == 0
      && sect_size >= tdep->sizeof_gregset + I387_SIZEOF_FSAVE)
    {
      if (tdep->gregset == NULL)
        tdep->gregset =
	  regset_alloc (gdbarch, i386nbsd_aout_supply_regset, NULL);
      return tdep->gregset;
    }

  return NULL;
}

/* From <machine/signal.h>.  */
int i386nbsd_sc_reg_offset[] =
{
  10 * 4,			/* %eax */
  9 * 4,			/* %ecx */
  8 * 4,			/* %edx */
  7 * 4,			/* %ebx */
  14 * 4,			/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  11 * 4,			/* %eip */
  13 * 4,			/* %eflags */
  12 * 4,			/* %cs */
  15 * 4,			/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

/* From <machine/mcontext.h>.  */
int i386nbsd_mc_reg_offset[] =
{
  11 * 4,			/* %eax */
  10 * 4,			/* %ecx */
  9 * 4,			/* %edx */
  8 * 4,			/* %ebx */
  7 * 4,			/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  14 * 4,			/* %eip */
  16 * 4,			/* %eflags */
  15 * 4,			/* %cs */
  18 * 4,			/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

static void
i386nbsd_sigtramp_cache_init (const struct tramp_frame *,
			     struct frame_info *,
			     struct trad_frame_cache *,
			     CORE_ADDR);

static const struct tramp_frame i386nbsd_sigtramp_sc16 =
{
  SIGTRAMP_FRAME,
  1,
  {
    { 0x8d, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x10, -1 },
			/* leal  0x10(%esp), %eax */
    { 0x50, -1 },	/* pushl %eax */
    { 0x50, -1 },	/* pushl %eax */
    { 0xb8, -1 }, { 0x27, -1 }, {0x01, -1 }, {0x00, -1 }, {0x00, -1 },
			/* movl  $0x127, %eax		# __sigreturn14 */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { 0xb8, -1 }, { 0x01, -1 }, {0x00, -1 }, {0x00, -1 }, {0x00, -1 },
			/* movl  $0x1, %eax		# exit */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_sc2 =
{
  SIGTRAMP_FRAME,
  1,
  {
    { 0x8d, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x0c, -1 },
			/* leal  0x0c(%esp), %eax */
    { 0x89, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
			/* movl  %eax, 0x4(%esp) */
    { 0xb8, -1 }, { 0x27, -1 }, {0x01, -1 }, {0x00, -1 }, {0x00, -1 },
			/* movl  $0x127, %eax		# __sigreturn14 */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { 0x89, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
			/* movl  %eax, 0x4(%esp) */
    { 0xb8, -1 }, { 0x01, -1 }, {0x00, -1 }, {0x00, -1 }, {0x00, -1 },
			/* movl  $0x1, %eax */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_si2 =
{
  SIGTRAMP_FRAME,
  1,
  {
    { 0x8b, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x08, -1 },
			/* movl  8(%esp),%eax */
    { 0x89, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
			/* movl  %eax, 0x4(%esp) */
    { 0xb8, -1 }, { 0x34, -1 }, { 0x01, -1 }, { 0x00, -1 }, { 0x00, -1 },
			/* movl  $0x134, %eax            # setcontext */
    { 0xcd, -1 }, { 0x80, -1 },
			/* int   $0x80 */
    { 0x89, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
			/* movl  %eax, 0x4(%esp) */
    { 0xb8, -1 }, { 0x01, -1 }, { 0x00, -1 }, { 0x00, -1 }, { 0x00, -1 },
			/* movl  $0x1, %eax */
    { 0xcd, -1 }, { 0x80, -1 },
			/* int   $0x80 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_si31 =
{
  SIGTRAMP_FRAME,
  1,
  {
    { 0x8d, -1 }, { 0x84, -1 }, { 0x24, -1 },
        { 0x8c, -1 }, { 0x00, -1 }, { 0x00, -1 }, { 0x00, -1 },
			/* leal  0x8c(%esp), %eax */
    { 0x89, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
			/* movl  %eax, 0x4(%esp) */
    { 0xb8, -1 }, { 0x34, -1 }, { 0x01, -1 }, { 0x00, -1 }, { 0x00, -1 },
			/* movl  $0x134, %eax            # setcontext */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { 0x89, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
			/* movl  %eax, 0x4(%esp) */
    { 0xb8, -1 }, { 0x01, -1 }, {0x00, -1 }, {0x00, -1 }, {0x00, -1 },
			/* movl  $0x1, %eax */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_si4 =
{
  SIGTRAMP_FRAME,
  1,
  {
    { 0x8d, -1 }, { 0x84, -1 }, { 0x24, -1 },
        { 0x8c, -1 }, { 0x00, -1 }, { 0x00, -1 }, { 0x00, -1 },
			/* leal  0x8c(%esp), %eax */
    { 0x89, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
			/* movl  %eax, 0x4(%esp) */
    { 0xb8, -1 }, { 0x34, -1 }, { 0x01, -1 }, { 0x00, -1 }, { 0x00, -1 },
			/* movl  $0x134, %eax            # setcontext */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { 0xc7, -1 }, { 0x44, -1 }, { 0x24, -1 }, { 0x04, -1 },
        { 0xff, -1 }, { 0xff, -1 }, { 0xff, -1 }, { 0xff, -1 },
			/* movl   $0xffffffff,0x4(%esp) */
    { 0xb8, -1 }, { 0x01, -1 }, {0x00, -1 }, {0x00, -1 }, {0x00, -1 },
			/* movl  $0x1, %eax */
    { 0xcd, -1 }, { 0x80, -1},
			/* int   $0x80 */
    { TRAMP_SENTINEL_INSN, -1 }
  },
  i386nbsd_sigtramp_cache_init
};


static void
i386nbsd_sigtramp_cache_init (const struct tramp_frame *self,
			     struct frame_info *next_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (next_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  CORE_ADDR sp = frame_unwind_register_unsigned (next_frame, I386_ESP_REGNUM);
  CORE_ADDR base;
  int *reg_offset;
  int num_regs;
  int i;

  if (self == &i386nbsd_sigtramp_sc16 || self == &i386nbsd_sigtramp_sc2)
    {
      reg_offset = i386nbsd_sc_reg_offset;
      num_regs = ARRAY_SIZE (i386nbsd_sc_reg_offset);

      /* Read in the sigcontext address */
      base = read_memory_unsigned_integer (sp + 8, 4);
    }
  else
    {
      reg_offset = i386nbsd_mc_reg_offset;
      num_regs = ARRAY_SIZE (i386nbsd_mc_reg_offset);

      /* Read in the ucontext address */
      base = read_memory_unsigned_integer (sp + 8, 4);
      /* offsetof(ucontext_t, uc_mcontext) == 36 */
      base += 36;
    }

  for (i = 0; i < num_regs; i++)
    if (reg_offset[i] != -1)
      trad_frame_set_reg_addr (this_cache, i, base + reg_offset[i]);

  /* Construct the frame ID using the function start.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

/* Kernel debugging support */

/* From <machine/frame.h>.  Note that %esp and %ess are only saved in
   a trap frame when entering the kernel from user space.  */
static int i386nbsd_tf_reg_offset[] =
{
  10 * 4,			/* %eax */
  9 * 4,			/* %ecx */
  8 * 4,			/* %edx */
  7 * 4,			/* %ebx */
  -1,				/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  13 * 4,			/* %eip */
  15 * 4,			/* %eflags */
  14 * 4,			/* %cs */
  -1,				/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

static struct trad_frame_cache *
i386nbsd_trapframe_cache(struct frame_info *next_frame, void **this_cache)
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
  sp = frame_unwind_register_unsigned (next_frame, I386_ESP_REGNUM);

  find_pc_partial_function (func, &name, NULL, NULL);
  if (name && strncmp (name, "Xintr", 5) == 0)
    addr = sp + 8;		/* It's an interrupt frame.  */
  else
    addr = sp + 4;

  for (i = 0; i < ARRAY_SIZE (i386nbsd_tf_reg_offset); i++)
    if (i386nbsd_tf_reg_offset[i] != -1)
      trad_frame_set_reg_addr (cache, i, addr + i386nbsd_tf_reg_offset[i]);

  /* Read %cs from trap frame.  */
  addr += i386nbsd_tf_reg_offset[I386_CS_REGNUM];
  cs = read_memory_unsigned_integer (addr, 4); 
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    {
      /* Trap from user space; terminate backtrace.  */
      trad_frame_set_id (cache, null_frame_id);
    }
  else
    {
      /* Construct the frame ID using the function start.  */
      trad_frame_set_id (cache, frame_id_build (sp + 8, func));
    }

  return cache;
}

static void
i386nbsd_trapframe_this_id (struct frame_info *next_frame,
			    void **this_cache, struct frame_id *this_id)
{
  struct trad_frame_cache *cache =
    i386nbsd_trapframe_cache (next_frame, this_cache);
  
  trad_frame_get_id (cache, this_id);
}

static void
i386nbsd_trapframe_prev_register (struct frame_info *next_frame,
				  void **this_cache, int regnum,
				  int *optimizedp, enum lval_type *lvalp,
				  CORE_ADDR *addrp, int *realnump,
				  gdb_byte *valuep)
{
  struct trad_frame_cache *cache =
    i386nbsd_trapframe_cache (next_frame, this_cache);

  trad_frame_get_register (cache, next_frame, regnum,
			   optimizedp, lvalp, addrp, realnump, valuep);
}

static int
i386nbsd_trapframe_sniffer (const struct frame_unwind *self,
			    struct frame_info *next_frame,
			    void **this_prologue_cache)
{
  ULONGEST cs;
  char *name;

  /* Check Current Privilege Level and bail out if we're not executing
     in kernel space.  */
  cs = frame_unwind_register_unsigned (next_frame, I386_CS_REGNUM);
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    return 0;


  find_pc_partial_function (frame_pc_unwind (next_frame), &name, NULL, NULL);
  return (name && (strcmp (name, "calltrap") == 0
		   || strcmp (name, "syscall1") == 0
		   || strncmp (name, "Xintr", 5) == 0
		   || strncmp (name, "Xsoft", 5) == 0));
}

const struct frame_unwind i386nbsd_trapframe_unwind = {
  /* FIXME: kettenis/20051219: This really is more like an interrupt
     frame, but SIGTRAMP_FRAME would print <signal handler called>,
     which really is not what we want here.  */
  NORMAL_FRAME,
  i386nbsd_trapframe_this_id,
  i386nbsd_trapframe_prev_register,
  NULL,
  i386nbsd_trapframe_sniffer
};

static void 
i386nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Obviously NetBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  /* NetBSD has a different `struct reg'.  */
  tdep->gregset_reg_offset = i386nbsd_r_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386nbsd_r_reg_offset);
  tdep->sizeof_gregset = 16 * 4;

  /* NetBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  /* NetBSD uses tramp_frame sniffers for signal trampolines. */
  tdep->sigcontext_addr= 0;
  tdep->sigtramp_start = 0;
  tdep->sigtramp_end = 0;
  tdep->sigtramp_p = 0;
  tdep->sc_reg_offset = 0;
  tdep->sc_num_regs = 0;

  tramp_frame_prepend_unwinder (gdbarch, &i386nbsd_sigtramp_sc16);
  tramp_frame_prepend_unwinder (gdbarch, &i386nbsd_sigtramp_sc2);
  tramp_frame_prepend_unwinder (gdbarch, &i386nbsd_sigtramp_si2);
  tramp_frame_prepend_unwinder (gdbarch, &i386nbsd_sigtramp_si31);
  tramp_frame_prepend_unwinder (gdbarch, &i386nbsd_sigtramp_si4);

  /* Unwind kernel trap frames correctly.  */
  frame_unwind_prepend_unwinder (gdbarch, &i386nbsd_trapframe_unwind);
}

/* NetBSD a.out.  */

static void
i386nbsdaout_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386nbsd_init_abi (info, gdbarch);

  /* NetBSD a.out has a single register set.  */
  set_gdbarch_regset_from_core_section
    (gdbarch, i386nbsd_aout_regset_from_core_section);
}

/* NetBSD ELF.  */

static void
i386nbsdelf_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* It's still NetBSD.  */
  i386nbsd_init_abi (info, gdbarch);

  /* But ELF-based.  */
  i386_elf_init_abi (info, gdbarch);

  /* NetBSD ELF uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* NetBSD ELF uses -fpcc-struct-return by default.  */
  tdep->struct_return = pcc_struct_return;
}

static enum gdb_osabi
i386nbsd_elf_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "elf32-i386") != 0)
    return GDB_OSABI_UNKNOWN;
  /* disgusting hack since kernels don't have a PT_NOTE section */
  if ((unsigned long)elf_elfheader (abfd)->e_entry != (unsigned long)0xc0100000)
    return GDB_OSABI_UNKNOWN;

  return GDB_OSABI_NETBSD_ELF;
}


void
_initialize_i386nbsd_tdep (void)
{
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_elf_flavour,
				  i386nbsd_elf_osabi_sniffer);
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_NETBSD_AOUT,
			  i386nbsdaout_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_NETBSD_ELF,
			  i386nbsdelf_init_abi);
}
