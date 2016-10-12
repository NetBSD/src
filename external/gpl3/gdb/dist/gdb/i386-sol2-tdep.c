/* Target-dependent code for Solaris x86.

   Copyright (C) 2002-2016 Free Software Foundation, Inc.

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
#include "value.h"
#include "osabi.h"

#include "sol2-tdep.h"
#include "i386-tdep.h"
#include "solib-svr4.h"

/* From <ia32/sys/reg.h>.  */
static int i386_sol2_gregset_reg_offset[] =
{
  11 * 4,			/* %eax */
  10 * 4,			/* %ecx */
  9 * 4,			/* %edx */
  8 * 4,			/* %ebx */
  17 * 4,			/* %esp */
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

/* Return whether THIS_FRAME corresponds to a Solaris sigtramp
   routine.  */

static int
i386_sol2_sigtramp_p (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  return (name && (strcmp ("sigacthandler", name) == 0
		   || strcmp (name, "ucbsigvechandler") == 0));
}

/* Solaris doesn't have a `struct sigcontext', but it does have a
   `mcontext_t' that contains the saved set of machine registers.  */

static CORE_ADDR
i386_sol2_mcontext_addr (struct frame_info *this_frame)
{
  CORE_ADDR sp, ucontext_addr;

  sp = get_frame_register_unsigned (this_frame, I386_ESP_REGNUM);
  ucontext_addr = get_frame_memory_unsigned (this_frame, sp + 8, 4);

  return ucontext_addr + 36;
}

/* SunPRO encodes the static variables.  This is not related to C++
   mangling, it is done for C too.  */

static const char *
i386_sol2_static_transform_name (const char *name)
{
  if (name[0] == '.')
    {
      const char *p;

      /* For file-local statics there will be a period, a bunch of
         junk (the contents of which match a string given in the
         N_OPT), a period and the name.  For function-local statics
         there will be a bunch of junk (which seems to change the
         second character from 'A' to 'B'), a period, the name of the
         function, and the name.  So just skip everything before the
         last period.  */
      p = strrchr (name, '.');
      if (p != NULL)
        name = p + 1;
    }
  return name;
}

/* Solaris 2.  */

static void
i386_sol2_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Solaris is SVR4-based.  */
  i386_svr4_init_abi (info, gdbarch);

  /* The SunPRO compiler puts out 0 instead of the address in N_SO symbols,
     and for SunPRO 3.0, N_FUN symbols too.  */
  set_gdbarch_sofun_address_maybe_missing (gdbarch, 1);

  /* Handle SunPRO encoding of static symbols.  */
  set_gdbarch_static_transform_name (gdbarch, i386_sol2_static_transform_name);

  /* Solaris reserves space for its FPU emulator in `fpregset_t'.
     There is also some space reserved for the registers of a Weitek
     math coprocessor.  */
  tdep->gregset_reg_offset = i386_sol2_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386_sol2_gregset_reg_offset);
  tdep->sizeof_gregset = 19 * 4;
  tdep->sizeof_fpregset = 380;

  /* Signal trampolines are slightly different from SVR4.  */
  tdep->sigtramp_p = i386_sol2_sigtramp_p;
  tdep->sigcontext_addr = i386_sol2_mcontext_addr;
  tdep->sc_reg_offset = tdep->gregset_reg_offset;
  tdep->sc_num_regs = tdep->gregset_num_regs;

  /* Solaris has SVR4-style shared libraries.  */
  set_gdbarch_skip_solib_resolver (gdbarch, sol2_skip_solib_resolver);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* How to print LWP PTIDs from core files.  */
  set_gdbarch_core_pid_to_str (gdbarch, sol2_core_pid_to_str);
}


static enum gdb_osabi
i386_sol2_osabi_sniffer (bfd *abfd)
{
  /* If we have a section named .SUNW_version, then it is almost
     certainly Solaris 2.  */
  if (bfd_get_section_by_name (abfd, ".SUNW_version"))
    return GDB_OSABI_SOLARIS;

  return GDB_OSABI_UNKNOWN;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386_sol2_tdep (void);

void
_initialize_i386_sol2_tdep (void)
{
  /* Register an ELF OS ABI sniffer for Solaris 2 binaries.  */
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_elf_flavour,
				  i386_sol2_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_SOLARIS,
			  i386_sol2_init_abi);
}
