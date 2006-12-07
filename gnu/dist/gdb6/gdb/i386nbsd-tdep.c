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
#include "trad-frame.h"		/* kernel frame support */
#include "frame-unwind.h"	/* kernel frame support */

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

/* Under NetBSD/i386, signal handler invocations can be identified by the
   designated code sequence that is used to return from a signal handler.
   In particular, the return address of a signal handler points to the
   following code sequence:

	leal	0x8c(%esp), %eax
	movl	%eax, 0x4(%esp)
	movl	$0x134, %eax		# setcontext
	int	$0x80

   Each instruction has a unique encoding, so we simply attempt to match
   the instruction the PC is pointing to with any of the above instructions.
   If there is a hit, we know the offset to the start of the designated
   sequence and can then check whether we really are executing in the
   signal trampoline.  If not, -1 is returned, otherwise the offset from the
   start of the return sequence is returned.  */
#define RETCODE_INSN1		0x8d
#define RETCODE_INSN2		0x89
#define RETCODE_INSN3		0xb8
#define RETCODE_INSN4		0xcd

#define RETCODE_INSN2_OFF	7
#define RETCODE_INSN3_OFF	11
#define RETCODE_INSN4_OFF	16

static const unsigned char sigtramp_retcode[] =
{
  RETCODE_INSN1, 0x84, 0x24, 0x8c, 0x00, 0x00, 0x00,
  RETCODE_INSN2, 0x44, 0x24, 0x04,
  RETCODE_INSN3, 0x34, 0x01, 0x00, 0x00,
  RETCODE_INSN4, 0x80,
};

static LONGEST
i386nbsd_sigtramp_offset (struct frame_info *next_frame)
{
  CORE_ADDR pc = frame_pc_unwind (next_frame);
  unsigned char ret[sizeof(sigtramp_retcode)], insn;
  LONGEST off;
  int i;

  if (!safe_frame_unwind_memory (next_frame, pc, &insn, 1))
    return -1;

  switch (insn)
    {
    case RETCODE_INSN1:
      off = 0;
      break;

    case RETCODE_INSN2:
      off = RETCODE_INSN2_OFF;
      break;

    case RETCODE_INSN3:
      off = RETCODE_INSN3_OFF;
      break;

    case RETCODE_INSN4:
      off = RETCODE_INSN4_OFF;
      break;

    default:
      return -1;
    }

  pc -= off;

  if (!safe_frame_unwind_memory (next_frame, pc, ret, sizeof (ret)))
    return -1;

  if (memcmp (ret, sigtramp_retcode, sizeof (ret)) == 0)
    return off;

  return -1;
}

/* Return whether the frame preceding NEXT_FRAME corresponds to a
   NetBSD sigtramp routine.  */

static int
i386nbsd_sigtramp_p (struct frame_info *next_frame)
{
  CORE_ADDR pc = frame_pc_unwind (next_frame);
  char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  return (nbsd_pc_in_sigtramp (pc, name)
	  || i386nbsd_sigtramp_offset (next_frame) >= 0);
}

/* From <sys/ucontext.h> and <machine/mcontext.h>. */
#define MCOFF 36 /* mcontext in ucontext */
#define _REG_GS         0
#define _REG_FS         1
#define _REG_ES         2
#define _REG_DS         3
#define _REG_EDI        4
#define _REG_ESI        5
#define _REG_EBP        6
#define _REG_ESP        7
#define _REG_EBX        8
#define _REG_EDX        9
#define _REG_ECX        10
#define _REG_EAX        11
#define _REG_TRAPNO     12
#define _REG_ERR        13
#define _REG_EIP        14
#define _REG_CS         15
#define _REG_EFL        16
#define _REG_UESP       17
#define _REG_SS         18
int i386nbsd_sc_reg_offset[] =
{
  MCOFF + _REG_EAX * 4,		/* %eax */
  MCOFF + _REG_ECX * 4,		/* %ecx */
  MCOFF + _REG_EDX * 4,		/* %edx */
  MCOFF + _REG_EBX * 4,		/* %ebx */
  MCOFF + _REG_ESP * 4,		/* %esp */
  MCOFF + _REG_EBP * 4,		/* %ebp */
  MCOFF + _REG_ESI * 4,		/* %esi */
  MCOFF + _REG_EDI * 4,		/* %edi */
  MCOFF + _REG_EIP * 4,		/* %eip */
  MCOFF + _REG_EFL * 4,		/* %eflags */
  MCOFF + _REG_CS * 4,		/* %cs */
  MCOFF + _REG_SS * 4,		/* %ss */
  MCOFF + _REG_DS * 4,		/* %ds */
  MCOFF + _REG_ES * 4,		/* %es */
  MCOFF + _REG_FS * 4,		/* %fs */
  MCOFF + _REG_GS * 4		/* %gs */
};

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

  /* NetBSD has different signal trampoline conventions.  */
  tdep->sigtramp_start = 0;
  tdep->sigtramp_end = 0;
  tdep->sigtramp_p = i386nbsd_sigtramp_p;

  /* NetBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  /* NetBSD has a `struct sigcontext' that's different from the
     original 4.3 BSD.  */
  tdep->sc_reg_offset = i386nbsd_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386nbsd_sc_reg_offset);

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
