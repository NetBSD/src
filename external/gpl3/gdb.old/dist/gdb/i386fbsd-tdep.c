/* Target-dependent code for FreeBSD/i386.

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
#include "gdbcore.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "i386fbsd-tdep.h"
#include "x86-xstate.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "bsd-uthread.h"
#include "fbsd-tdep.h"
#include "solib-svr4.h"

/* Support for signal handlers.  */

/* Return whether THIS_FRAME corresponds to a FreeBSD sigtramp
   routine.  */

/* FreeBSD/i386 supports three different signal trampolines, one for
   versions before 4.0, a second for 4.x, and a third for 5.0 and
   later.  To complicate matters, FreeBSD/i386 binaries running under
   an amd64 kernel use a different set of trampolines.  These
   trampolines differ from the i386 kernel trampolines in that they
   omit a middle section that conditionally restores %gs.  */

static const gdb_byte i386fbsd_sigtramp_start[] =
{
  0x8d, 0x44, 0x24, 0x20,       /* lea     SIGF_UC(%esp),%eax */
  0x50				/* pushl   %eax */
};

static const gdb_byte i386fbsd_sigtramp_middle[] =
{
  0xf7, 0x40, 0x54, 0x00, 0x00, 0x02, 0x00,
				/* testl   $PSL_VM,UC_EFLAGS(%eax) */
  0x75, 0x03,			/* jne	   +3 */
  0x8e, 0x68, 0x14		/* mov	   UC_GS(%eax),%gs */
};

static const gdb_byte i386fbsd_sigtramp_end[] =
{
  0xb8, 0xa1, 0x01, 0x00, 0x00, /* movl   $SYS_sigreturn,%eax */
  0x50,			/* pushl   %eax */
  0xcd, 0x80			/* int	   $0x80 */
};

static const gdb_byte i386fbsd_freebsd4_sigtramp_start[] =
{
  0x8d, 0x44, 0x24, 0x14,	/* lea	   SIGF_UC4(%esp),%eax */
  0x50				/* pushl   %eax */
};

static const gdb_byte i386fbsd_freebsd4_sigtramp_middle[] =
{
  0xf7, 0x40, 0x54, 0x00, 0x00, 0x02, 0x00,
				/* testl   $PSL_VM,UC4_EFLAGS(%eax) */
  0x75, 0x03,			/* jne	   +3 */
  0x8e, 0x68, 0x14		/* mov	   UC4_GS(%eax),%gs */
};

static const gdb_byte i386fbsd_freebsd4_sigtramp_end[] =
{
  0xb8, 0x58, 0x01, 0x00, 0x00, /* movl    $344,%eax */
  0x50,			/* pushl   %eax */
  0xcd, 0x80			/* int	   $0x80 */
};

static const gdb_byte i386fbsd_osigtramp_start[] =
{
  0x8d, 0x44, 0x24, 0x14,	/* lea	   SIGF_SC(%esp),%eax */
  0x50				/* pushl   %eax */
};

static const gdb_byte i386fbsd_osigtramp_middle[] =
{
  0xf7, 0x40, 0x18, 0x00, 0x00, 0x02, 0x00,
				/* testl   $PSL_VM,SC_PS(%eax) */
  0x75, 0x03,			/* jne	   +3 */
  0x8e, 0x68, 0x44		/* mov	   SC_GS(%eax),%gs */
};

static const gdb_byte i386fbsd_osigtramp_end[] =
{
  0xb8, 0x67, 0x00, 0x00, 0x00, /* movl    $103,%eax */
  0x50,			/* pushl   %eax */
  0xcd, 0x80			/* int	   $0x80 */
};

/* The three different trampolines are all the same size.  */
gdb_static_assert (sizeof i386fbsd_sigtramp_start
		   == sizeof i386fbsd_freebsd4_sigtramp_start);
gdb_static_assert (sizeof i386fbsd_sigtramp_start
		   == sizeof i386fbsd_osigtramp_start);
gdb_static_assert (sizeof i386fbsd_sigtramp_middle
		   == sizeof i386fbsd_freebsd4_sigtramp_middle);
gdb_static_assert (sizeof i386fbsd_sigtramp_middle
		   == sizeof i386fbsd_osigtramp_middle);
gdb_static_assert (sizeof i386fbsd_sigtramp_end
		   == sizeof i386fbsd_freebsd4_sigtramp_end);
gdb_static_assert (sizeof i386fbsd_sigtramp_end
		   == sizeof i386fbsd_osigtramp_end);

/* We assume that the middle is the largest chunk below.  */
gdb_static_assert (sizeof i386fbsd_sigtramp_middle
		   > sizeof i386fbsd_sigtramp_start);
gdb_static_assert (sizeof i386fbsd_sigtramp_middle
		   > sizeof i386fbsd_sigtramp_end);

static int
i386fbsd_sigtramp_p (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[sizeof i386fbsd_sigtramp_middle];
  const gdb_byte *middle, *end;

  /* Look for a matching start.  */
  if (!safe_frame_unwind_memory (this_frame, pc, buf,
				 sizeof i386fbsd_sigtramp_start))
    return 0;
  if (memcmp (buf, i386fbsd_sigtramp_start, sizeof i386fbsd_sigtramp_start)
      == 0)
    {
      middle = i386fbsd_sigtramp_middle;
      end = i386fbsd_sigtramp_end;
    }
  else if (memcmp (buf, i386fbsd_freebsd4_sigtramp_start,
		   sizeof i386fbsd_freebsd4_sigtramp_start) == 0)
    {
      middle = i386fbsd_freebsd4_sigtramp_middle;
      end = i386fbsd_freebsd4_sigtramp_end;
    }
  else if (memcmp (buf, i386fbsd_osigtramp_start,
		   sizeof i386fbsd_osigtramp_start) == 0)
    {
      middle = i386fbsd_osigtramp_middle;
      end = i386fbsd_osigtramp_end;
    }
  else
    return 0;

  /* Since the end is shorter than the middle, check for a matching end
     next.  */
  pc += sizeof i386fbsd_sigtramp_start;
  if (!safe_frame_unwind_memory (this_frame, pc, buf,
				 sizeof i386fbsd_sigtramp_end))
    return 0;
  if (memcmp (buf, end, sizeof i386fbsd_sigtramp_end) == 0)
    return 1;

  /* If the end didn't match, check for a matching middle.  */
  if (!safe_frame_unwind_memory (this_frame, pc, buf,
				 sizeof i386fbsd_sigtramp_middle))
    return 0;
  if (memcmp (buf, middle, sizeof i386fbsd_sigtramp_middle) != 0)
    return 0;

  /* The middle matched, check for a matching end.  */
  pc += sizeof i386fbsd_sigtramp_middle;
  if (!safe_frame_unwind_memory (this_frame, pc, buf,
				 sizeof i386fbsd_sigtramp_end))
    return 0;
  if (memcmp (buf, end, sizeof i386fbsd_sigtramp_end) != 0)
    return 0;

  return 1;
}

/* FreeBSD 3.0-RELEASE or later.  */

/* From <machine/reg.h>.  */
static int i386fbsd_r_reg_offset[] =
{
  9 * 4, 8 * 4, 7 * 4, 6 * 4,	/* %eax, %ecx, %edx, %ebx */
  15 * 4, 4 * 4,		/* %esp, %ebp */
  3 * 4, 2 * 4,			/* %esi, %edi */
  12 * 4, 14 * 4,		/* %eip, %eflags */
  13 * 4, 16 * 4,		/* %cs, %ss */
  1 * 4, 0 * 4, -1, -1		/* %ds, %es, %fs, %gs */
};

/* Sigtramp routine location.  */
CORE_ADDR i386fbsd_sigtramp_start_addr;
CORE_ADDR i386fbsd_sigtramp_end_addr;

/* From <machine/signal.h>.  */
int i386fbsd_sc_reg_offset[] =
{
  8 + 14 * 4,			/* %eax */
  8 + 13 * 4,			/* %ecx */
  8 + 12 * 4,			/* %edx */
  8 + 11 * 4,			/* %ebx */
  8 + 0 * 4,                    /* %esp */
  8 + 1 * 4,                    /* %ebp */
  8 + 10 * 4,                   /* %esi */
  8 + 9 * 4,                    /* %edi */
  8 + 3 * 4,                    /* %eip */
  8 + 4 * 4,                    /* %eflags */
  8 + 7 * 4,                    /* %cs */
  8 + 8 * 4,                    /* %ss */
  8 + 6 * 4,                    /* %ds */
  8 + 5 * 4,                    /* %es */
  8 + 15 * 4,			/* %fs */
  8 + 16 * 4			/* %gs */
};

/* From /usr/src/lib/libc/i386/gen/_setjmp.S.  */
static int i386fbsd_jmp_buf_reg_offset[] =
{
  -1,				/* %eax */
  -1,				/* %ecx */
  -1,				/* %edx */
  1 * 4,			/* %ebx */
  2 * 4,			/* %esp */
  3 * 4,			/* %ebp */
  4 * 4,			/* %esi */
  5 * 4,			/* %edi */
  0 * 4				/* %eip */
};

/* Get XSAVE extended state xcr0 from core dump.  */

uint64_t
i386fbsd_core_read_xcr0 (bfd *abfd)
{
  asection *xstate = bfd_get_section_by_name (abfd, ".reg-xstate");
  uint64_t xcr0;

  if (xstate)
    {
      size_t size = bfd_section_size (abfd, xstate);

      /* Check extended state size.  */
      if (size < X86_XSTATE_AVX_SIZE)
	xcr0 = X86_XSTATE_SSE_MASK;
      else
	{
	  char contents[8];

	  if (! bfd_get_section_contents (abfd, xstate, contents,
					  I386_FBSD_XSAVE_XCR0_OFFSET,
					  8))
	    {
	      warning (_("Couldn't read `xcr0' bytes from "
			 "`.reg-xstate' section in core file."));
	      return 0;
	    }

	  xcr0 = bfd_get_64 (abfd, contents);
	}
    }
  else
    xcr0 = 0;

  return xcr0;
}

/* Implement the core_read_description gdbarch method.  */

static const struct target_desc *
i386fbsd_core_read_description (struct gdbarch *gdbarch,
				struct target_ops *target,
				bfd *abfd)
{
  return i386_target_description (i386fbsd_core_read_xcr0 (abfd));
}

/* Similar to i386_supply_fpregset, but use XSAVE extended state.  */

static void
i386fbsd_supply_xstateregset (const struct regset *regset,
			      struct regcache *regcache, int regnum,
			      const void *xstateregs, size_t len)
{
  i387_supply_xsave (regcache, regnum, xstateregs);
}

/* Similar to i386_collect_fpregset, but use XSAVE extended state.  */

static void
i386fbsd_collect_xstateregset (const struct regset *regset,
			       const struct regcache *regcache,
			       int regnum, void *xstateregs, size_t len)
{
  i387_collect_xsave (regcache, regnum, xstateregs, 1);
}

/* Register set definitions.  */

static const struct regset i386fbsd_xstateregset =
  {
    NULL,
    i386fbsd_supply_xstateregset,
    i386fbsd_collect_xstateregset
  };

/* Iterate over core file register note sections.  */

static void
i386fbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  cb (".reg", tdep->sizeof_gregset, &i386_gregset, NULL, cb_data);
  cb (".reg2", tdep->sizeof_fpregset, &i386_fpregset, NULL, cb_data);

  if (tdep->xcr0 & X86_XSTATE_AVX)
    cb (".reg-xstate", X86_XSTATE_SIZE(tdep->xcr0),
	&i386fbsd_xstateregset, "XSAVE extended state", cb_data);
}

static void
i386fbsd_supply_uthread (struct regcache *regcache,
			 int regnum, CORE_ADDR addr)
{
  gdb_byte buf[4];
  int i;

  gdb_assert (regnum >= -1);

  for (i = 0; i < ARRAY_SIZE (i386fbsd_jmp_buf_reg_offset); i++)
    {
      if (i386fbsd_jmp_buf_reg_offset[i] != -1
	  && (regnum == -1 || regnum == i))
	{
	  read_memory (addr + i386fbsd_jmp_buf_reg_offset[i], buf, 4);
	  regcache_raw_supply (regcache, i, buf);
	}
    }
}

static void
i386fbsd_collect_uthread (const struct regcache *regcache,
			  int regnum, CORE_ADDR addr)
{
  gdb_byte buf[4];
  int i;

  gdb_assert (regnum >= -1);

  for (i = 0; i < ARRAY_SIZE (i386fbsd_jmp_buf_reg_offset); i++)
    {
      if (i386fbsd_jmp_buf_reg_offset[i] != -1
	  && (regnum == -1 || regnum == i))
	{
	  regcache_raw_collect (regcache, i, buf);
	  write_memory (addr + i386fbsd_jmp_buf_reg_offset[i], buf, 4);
	}
    }
}

static void
i386fbsdaout_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Obviously FreeBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  /* FreeBSD has a different `struct reg', and reserves some space for
     its FPU emulator in `struct fpreg'.  */
  tdep->gregset_reg_offset = i386fbsd_r_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386fbsd_r_reg_offset);
  tdep->sizeof_gregset = 18 * 4;
  tdep->sizeof_fpregset = 176;

  /* FreeBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  tdep->sigtramp_p = i386fbsd_sigtramp_p;

  /* FreeBSD uses a different memory layout.  */
  tdep->sigtramp_start = i386fbsd_sigtramp_start_addr;
  tdep->sigtramp_end = i386fbsd_sigtramp_end_addr;

  /* FreeBSD has a more complete `struct sigcontext'.  */
  tdep->sc_reg_offset = i386fbsd_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386fbsd_sc_reg_offset);

  /* FreeBSD provides a user-level threads implementation.  */
  bsd_uthread_set_supply_uthread (gdbarch, i386fbsd_supply_uthread);
  bsd_uthread_set_collect_uthread (gdbarch, i386fbsd_collect_uthread);
}

static void
i386fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* It's almost identical to FreeBSD a.out.  */
  i386fbsdaout_init_abi (info, gdbarch);

  /* Except that it uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  /* FreeBSD ELF uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}

/* FreeBSD 4.0-RELEASE or later.  */

/* From <machine/reg.h>.  */
static int i386fbsd4_r_reg_offset[] =
{
  10 * 4, 9 * 4, 8 * 4, 7 * 4,	/* %eax, %ecx, %edx, %ebx */
  16 * 4, 5 * 4,		/* %esp, %ebp */
  4 * 4, 3 * 4,			/* %esi, %edi */
  13 * 4, 15 * 4,		/* %eip, %eflags */
  14 * 4, 17 * 4,		/* %cs, %ss */
  2 * 4, 1 * 4, 0 * 4, 18 * 4	/* %ds, %es, %fs, %gs */
};

/* From <machine/signal.h>.  */
int i386fbsd4_sc_reg_offset[] =
{
  20 + 11 * 4,			/* %eax */
  20 + 10 * 4,			/* %ecx */
  20 + 9 * 4,			/* %edx */
  20 + 8 * 4,			/* %ebx */
  20 + 17 * 4,			/* %esp */
  20 + 6 * 4,			/* %ebp */
  20 + 5 * 4,			/* %esi */
  20 + 4 * 4,			/* %edi */
  20 + 14 * 4,			/* %eip */
  20 + 16 * 4,			/* %eflags */
  20 + 15 * 4,			/* %cs */
  20 + 18 * 4,			/* %ss */
  20 + 3 * 4,			/* %ds */
  20 + 2 * 4,			/* %es */
  20 + 1 * 4,			/* %fs */
  20 + 0 * 4			/* %gs */
};

static void
i386fbsd4_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Generic FreeBSD support. */
  fbsd_init_abi (info, gdbarch);

  /* Inherit stuff from older releases.  We assume that FreeBSD
     4.0-RELEASE always uses ELF.  */
  i386fbsd_init_abi (info, gdbarch);

  /* FreeBSD 4.0 introduced a new `struct reg'.  */
  tdep->gregset_reg_offset = i386fbsd4_r_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386fbsd4_r_reg_offset);
  tdep->sizeof_gregset = 19 * 4;

  /* FreeBSD 4.0 introduced a new `struct sigcontext'.  */
  tdep->sc_reg_offset = i386fbsd4_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386fbsd4_sc_reg_offset);

  tdep->xsave_xcr0_offset = I386_FBSD_XSAVE_XCR0_OFFSET;

  /* Iterate over core file register note sections.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, i386fbsd_iterate_over_regset_sections);

  set_gdbarch_core_read_description (gdbarch,
				     i386fbsd_core_read_description);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386fbsd_tdep (void);

void
_initialize_i386fbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_FREEBSD_AOUT,
			  i386fbsdaout_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_FREEBSD_ELF,
			  i386fbsd4_init_abi);
}
