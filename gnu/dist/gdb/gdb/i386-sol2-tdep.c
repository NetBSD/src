/* Target-dependent code for Solaris x86.
   Copyright 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "value.h"

#include "i386-tdep.h"

static int
i386_sol2_pc_in_sigtramp (CORE_ADDR pc, char *name)
{
  /* Signal handler frames under Solaris 2 are recognized by a return
     address of 0xffffffff.  */
  return (pc == 0xffffffff);
}

/* Solaris 2.  */

static void
i386_sol2_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Solaris is SVR4-based.  */
  i386_svr4_init_abi (info, gdbarch);

  /* Signal trampolines are different from SVR4, in fact they're
     rather similar to BSD.  */
  set_gdbarch_pc_in_sigtramp (gdbarch, i386_sol2_pc_in_sigtramp);
  tdep->sigcontext_addr = i386bsd_sigcontext_addr;
  tdep->sc_pc_offset = 36 + 14 * 4;
  tdep->sc_sp_offset = 36 + 17 * 4;

  /* Assume that the prototype flag can be trusted.  */
  set_gdbarch_coerce_float_to_double (gdbarch,
				      standard_coerce_float_to_double);
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

  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_SOLARIS,
			  i386_sol2_init_abi);
}
