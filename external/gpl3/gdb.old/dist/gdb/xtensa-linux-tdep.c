/* Target-dependent code for GNU/Linux on Xtensa processors.

   Copyright (C) 2007-2015 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "linux-tdep.h"
#include "solib-svr4.h"
#include "symtab.h"

/* This enum represents the signals' numbers on the Xtensa
   architecture.  It just contains the signal definitions which are
   different from the generic implementation.

   It is derived from the file <arch/xtensa/include/uapi/asm/signal.h>,
   from the Linux kernel tree.  */

enum
  {
    XTENSA_LINUX_SIGRTMIN = 32,
    XTENSA_LINUX_SIGRTMAX = 63,
  };

/* Implementation of `gdbarch_gdb_signal_from_target', as defined in
   gdbarch.h.  */

static enum gdb_signal
xtensa_linux_gdb_signal_from_target (struct gdbarch *gdbarch,
				   int signal)
{
  if (signal >= XTENSA_LINUX_SIGRTMIN && signal <= XTENSA_LINUX_SIGRTMAX)
    {
      int offset = signal - XTENSA_LINUX_SIGRTMIN;

      if (offset == 0)
	return GDB_SIGNAL_REALTIME_32;
      else
	return (enum gdb_signal) (offset - 1
				  + (int) GDB_SIGNAL_REALTIME_33);
    }
  else if (signal > XTENSA_LINUX_SIGRTMAX)
    return GDB_SIGNAL_UNKNOWN;

  return linux_gdb_signal_from_target (gdbarch, signal);
}

/* Implementation of `gdbarch_gdb_signal_to_target', as defined in
   gdbarch.h.  */

static int
xtensa_linux_gdb_signal_to_target (struct gdbarch *gdbarch,
				   enum gdb_signal signal)
{
  switch (signal)
    {
    /* GDB_SIGNAL_REALTIME_32 is not continuous in <gdb/signals.def>,
       therefore we have to handle it here.  */
    case GDB_SIGNAL_REALTIME_32:
      return XTENSA_LINUX_SIGRTMIN;

    /* GDB_SIGNAL_REALTIME_64 is not valid on Xtensa.  */
    case GDB_SIGNAL_REALTIME_64:
      return -1;
    }

  /* GDB_SIGNAL_REALTIME_33 to _63 are continuous.

     Xtensa does not have _64.  */
  if (signal >= GDB_SIGNAL_REALTIME_33
      && signal <= GDB_SIGNAL_REALTIME_63)
    {
      int offset = signal - GDB_SIGNAL_REALTIME_33;

      return XTENSA_LINUX_SIGRTMIN + 1 + offset;
    }

  return linux_gdb_signal_to_target (gdbarch, signal);
}

/* OS specific initialization of gdbarch.  */

static void
xtensa_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  set_gdbarch_gdb_signal_from_target (gdbarch,
				      xtensa_linux_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch,
				    xtensa_linux_gdb_signal_to_target);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_xtensa_linux_tdep;

void
_initialize_xtensa_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_xtensa, bfd_mach_xtensa, GDB_OSABI_LINUX,
			  xtensa_linux_init_abi);
}
