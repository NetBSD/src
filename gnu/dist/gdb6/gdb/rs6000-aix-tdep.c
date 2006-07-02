/* Native support code for PPC AIX, for GDB the GNU debugger.

   Copyright (C) 2006 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "osabi.h"
#include "rs6000-tdep.h"

static enum gdb_osabi
rs6000_aix_osabi_sniffer (bfd *abfd)
{
  
  if (bfd_get_flavour (abfd) == bfd_target_xcoff_flavour);
    return GDB_OSABI_AIX;

  return GDB_OSABI_UNKNOWN;
}

static void
rs6000_aix_init_osabi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* RS6000/AIX does not support PT_STEP.  Has to be simulated.  */
  set_gdbarch_software_single_step (gdbarch, rs6000_software_single_step);
}

void
_initialize_rs6000_aix_tdep (void)
{
  gdbarch_register_osabi_sniffer (bfd_arch_rs6000,
                                  bfd_target_xcoff_flavour,
                                  rs6000_aix_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_rs6000, 0, GDB_OSABI_AIX,
                          rs6000_aix_init_osabi);
}

