/* Target-dependent code for NetBSD/i386.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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

#include "arch-utils.h"
#include "frame.h"
#include "gdbcore.h"
#include "regcache.h"
#include "regset.h"
#include "osabi.h"
#include "symtab.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "frame-unwind.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "netbsd-tdep.h"
#include "solib-svr4.h"

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

/* From <machine/signal.h>.  */
static int i386nbsd_sc_reg_offset[] =
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
static int i386nbsd_mc_reg_offset[] =
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

static void i386nbsd_sigtramp_cache_init (const struct tramp_frame *,
					  const frame_info_ptr &,
					  struct trad_frame_cache *,
					  CORE_ADDR);

static const struct tramp_frame i386nbsd_sigtramp_sc16 =
{
  SIGTRAMP_FRAME,
  1,
  {
   /* leal  0x10(%esp), %eax */
   { 0x8d, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x10, ULONGEST_MAX },

   /* pushl %eax */
   { 0x50, ULONGEST_MAX },

   /* pushl %eax */
   { 0x50, ULONGEST_MAX },

   /* movl  $0x127, %eax		# __sigreturn14 */
   { 0xb8, ULONGEST_MAX },
   { 0x27, ULONGEST_MAX },
   {0x01, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },

   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},

   /* movl  $0x1, %eax		# exit */
   { 0xb8, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },

   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},

   { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_sc2 =
{
  SIGTRAMP_FRAME,
  1,
  {
   /* leal  0x0c(%esp), %eax */
   { 0x8d, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x0c, ULONGEST_MAX },
   /* movl  %eax, 0x4(%esp) */
   { 0x89, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   /* movl  $0x127, %eax		# __sigreturn14 */
   { 0xb8, ULONGEST_MAX },
   { 0x27, ULONGEST_MAX },
   {0x01, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},
   /* movl  %eax, 0x4(%esp) */
   { 0x89, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   /* movl  $0x1, %eax */
   { 0xb8, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},
   { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_si2 =
{
  SIGTRAMP_FRAME,
  1,
  {
   /* movl  8(%esp),%eax */
   { 0x8b, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x08, ULONGEST_MAX },
   /* movl  %eax, 0x4(%esp) */
   { 0x89, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   /* movl  $0x134, %eax            # setcontext */
   { 0xb8, ULONGEST_MAX },
   { 0x34, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX },
   /* movl  %eax, 0x4(%esp) */
   { 0x89, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   /* movl  $0x1, %eax */
   { 0xb8, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX },
   { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_si31 =
{
  SIGTRAMP_FRAME,
  1,
  {
   /* leal  0x8c(%esp), %eax */
   { 0x8d, ULONGEST_MAX },
   { 0x84, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x8c, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   /* movl  %eax, 0x4(%esp) */
   { 0x89, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   /* movl  $0x134, %eax            # setcontext */
   { 0xb8, ULONGEST_MAX },
   { 0x34, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},
   /* movl  %eax, 0x4(%esp) */
   { 0x89, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   /* movl  $0x1, %eax */
   { 0xb8, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},
   { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  i386nbsd_sigtramp_cache_init
};

static const struct tramp_frame i386nbsd_sigtramp_si4 =
{
  SIGTRAMP_FRAME,
  1,
  {
   /* leal  0x8c(%esp), %eax */
   { 0x8d, ULONGEST_MAX },
   { 0x84, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x8c, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   /* movl  %eax, 0x4(%esp) */
   { 0x89, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   /* movl  $0x134, %eax            # setcontext */
   { 0xb8, ULONGEST_MAX },
   { 0x34, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   { 0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},
   /* movl   $0xffffffff,0x4(%esp) */
   { 0xc7, ULONGEST_MAX },
   { 0x44, ULONGEST_MAX },
   { 0x24, ULONGEST_MAX },
   { 0x04, ULONGEST_MAX },
   { 0xff, ULONGEST_MAX },
   { 0xff, ULONGEST_MAX },
   { 0xff, ULONGEST_MAX },
   { 0xff, ULONGEST_MAX },
   /* movl  $0x1, %eax */
   { 0xb8, ULONGEST_MAX },
   { 0x01, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   {0x00, ULONGEST_MAX },
   /* int   $0x80 */
   { 0xcd, ULONGEST_MAX },
   { 0x80, ULONGEST_MAX},
   { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  i386nbsd_sigtramp_cache_init
};

static void
i386nbsd_sigtramp_cache_init (const struct tramp_frame *self,
			      const frame_info_ptr &this_frame,
			      struct trad_frame_cache *this_cache,
			      CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, I386_ESP_REGNUM);
  CORE_ADDR base;
  int *reg_offset;
  int num_regs;
  int i;

  if (self == &i386nbsd_sigtramp_sc16 || self == &i386nbsd_sigtramp_sc2)
    {
      reg_offset = i386nbsd_sc_reg_offset;
      num_regs = ARRAY_SIZE (i386nbsd_sc_reg_offset);

      /* Read in the sigcontext address.  */
      base = read_memory_unsigned_integer (sp + 8, 4, byte_order);
    }
  else
    {
      reg_offset = i386nbsd_mc_reg_offset;
      num_regs = ARRAY_SIZE (i386nbsd_mc_reg_offset);

      /* Read in the ucontext address.  */
      base = read_memory_unsigned_integer (sp + 8, 4, byte_order);
      /* offsetof(ucontext_t, uc_mcontext) == 36 */
      base += 36;
    }

  for (i = 0; i < num_regs; i++)
    if (reg_offset[i] != -1)
      trad_frame_set_reg_addr (this_cache, i, base + reg_offset[i]);

  /* Construct the frame ID using the function start.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}


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
   0 * 4			/* %gs */
};
 
static struct trad_frame_cache *
i386nbsd_trapframe_cache(frame_info_ptr this_frame, void **this_cache)
{
  struct trad_frame_cache *cache;
  CORE_ADDR func, sp, addr, tmp;
  ULONGEST cs;
  const char *name;
  int i;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (*this_cache)
    return (struct trad_frame_cache *)*this_cache;

  cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = cache;

  func = get_frame_func (this_frame);
  sp = get_frame_register_unsigned (this_frame, I386_ESP_REGNUM);

  find_pc_partial_function (func, &name, NULL, NULL);
  if (name && (strncmp (name, "Xintr", 5) == 0 ||
               strncmp (name, "Xhandle", 7) == 0))
    {
      /* It's an interrupt frame. */
      tmp = read_memory_unsigned_integer (sp + 4, 4, byte_order);
      if (tmp < 15)
        {
          /* Reasonable value for 'ppl': already on interrupt stack. */
          addr = sp + 8;
        }
      else
        {
          /* Switch to previous stack. */
          addr = tmp + 4;
        }
    }
  else
    {
      /* It's a trap frame. */
      addr = sp + 4;
    }

  for (i = 0; i < ARRAY_SIZE (i386nbsd_tf_reg_offset); i++)
    if (i386nbsd_tf_reg_offset[i] != -1)
      trad_frame_set_reg_addr (cache, i, addr + i386nbsd_tf_reg_offset[i]);

  /* Read %cs from trap frame.  */
  addr += i386nbsd_tf_reg_offset[I386_CS_REGNUM];
  cs = read_memory_unsigned_integer (addr, 4, byte_order); 
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    {
      /* Trap from user space; terminate backtrace.  */
      trad_frame_set_id (cache, outer_frame_id);
    }
  else
    {
      /* Construct the frame ID using the function start.  */
      trad_frame_set_id (cache, frame_id_build (sp + 8, func));
    }

  return cache;
}

static void
i386nbsd_trapframe_this_id (const frame_info_ptr& this_frame,
			    void **this_cache, struct frame_id *this_id)
{
  struct trad_frame_cache *cache =
    i386nbsd_trapframe_cache (this_frame, this_cache);
  
  trad_frame_get_id (cache, this_id);
}

static struct value *
i386nbsd_trapframe_prev_register (const frame_info_ptr& this_frame,
				  void **this_cache, int regnum)
{
  struct trad_frame_cache *cache =
    i386nbsd_trapframe_cache (this_frame, this_cache);

  return trad_frame_get_register (cache, this_frame, regnum);
}

static int
i386nbsd_trapframe_sniffer (const struct frame_unwind *self,
			    const frame_info_ptr& this_frame,
			    void **this_prologue_cache)
{
  ULONGEST cs;
  const char *name;

  /* Check Current Privilege Level and bail out if we're not executing
     in kernel space.  */
  cs = get_frame_register_unsigned (this_frame, I386_CS_REGNUM);
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    return 0;


  find_pc_partial_function (get_frame_pc (this_frame), &name, NULL, NULL);
  return (name && ((strcmp (name, "alltraps") == 0)
	        || (strcmp (name, "calltrap") == 0)
		|| (strcmp (name, "syscall1") == 0)
		|| (strcmp (name, "Xdoreti") == 0)
		|| (strncmp (name, "Xintr", 5) == 0)
		|| (strncmp (name, "Xhandle", 7) == 0)
		|| (strncmp (name, "Xpreempt", 8) == 0)
		|| (strncmp (name, "Xrecurse", 8) == 0)
		|| (strncmp (name, "Xresume", 7) == 0)
		|| (strncmp (name, "Xsoft", 5) == 0)
		|| (strncmp (name, "Xstray", 6) == 0)
		|| (strncmp (name, "Xsyscall", 8) == 0)
		|| (strncmp (name, "Xtrap", 5) == 0)
	    ));
}

const struct frame_unwind i386nbsd_trapframe_unwind = {
  /* FIXME: kettenis/20051219: This really is more like an interrupt
     frame, but SIGTRAMP_FRAME would print <signal handler called>,
     which really is not what we want here.  */
  "i386 trapframe",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  i386nbsd_trapframe_this_id,
  i386nbsd_trapframe_prev_register,
  NULL,
  i386nbsd_trapframe_sniffer
};

static void 
i386nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* Obviously NetBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  nbsd_init_abi (info, gdbarch);

  /* NetBSD has a different `struct reg'.  */
  tdep->gregset_reg_offset = i386nbsd_r_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386nbsd_r_reg_offset);
  tdep->sizeof_gregset = 16 * 4;

  /* NetBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  /* NetBSD uses tramp_frame sniffers for signal trampolines.  */
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

/* NetBSD ELF.  */

static void
i386nbsdelf_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

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

void _initialize_i386nbsd_tdep ();
void
_initialize_i386nbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_NETBSD,
			  i386nbsdelf_init_abi);
}
