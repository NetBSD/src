/* Target-dependent code for i386 BSD's.
   Copyright 2001, 2002 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "frame.h"
#include "gdbcore.h"
#include "regcache.h"

#include "gdb_string.h"

#include "i386-tdep.h"

/* Support for signal handlers.  */

/* Return whether PC is in a BSD sigtramp routine.  */

static int
i386bsd_pc_in_sigtramp (CORE_ADDR pc, char *name)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);

  return (pc >= tdep->sigtramp_start && pc < tdep->sigtramp_end);
}

/* Assuming FRAME is for a BSD sigtramp routine, return the address of
   the associated sigcontext structure.

   Note: This function is used for Solaris 2 too, so don't make it
   static.  */

CORE_ADDR
i386bsd_sigcontext_addr (struct frame_info *frame)
{
  if (frame->next)
    /* If this isn't the top frame, the next frame must be for the
       signal handler itself.  A pointer to the sigcontext structure
       is passed as the third argument to the signal handler.  */
    return read_memory_unsigned_integer (frame->next->frame + 16, 4);

  /* This is the top frame.  We'll have to find the address of the
     sigcontext structure by looking at the stack pointer.  */
  return read_memory_unsigned_integer (read_register (SP_REGNUM) + 8, 4);
}

/* Return the start address of the sigtramp routine.  */

CORE_ADDR
i386bsd_sigtramp_start (CORE_ADDR pc)
{
  return gdbarch_tdep (current_gdbarch)->sigtramp_start;
}

/* Return the end address of the sigtramp routine.  */

CORE_ADDR
i386bsd_sigtramp_end (CORE_ADDR pc)
{
  return gdbarch_tdep (current_gdbarch)->sigtramp_end;
}


/* Support for shared libraries.  */

/* Return non-zero if we are in a shared library trampoline code stub.  */

int
i386bsd_aout_in_solib_call_trampoline (CORE_ADDR pc, char *name)
{
  return (name && !strcmp (name, "_DYNAMIC"));
}

/* Traditional BSD (4.3 BSD, still used for BSDI and 386BSD).  */

/* From <machine/signal.h>.  */
int i386bsd_sc_pc_offset = 20;
int i386bsd_sc_sp_offset = 8;

void
i386bsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  set_gdbarch_pc_in_sigtramp (gdbarch, i386bsd_pc_in_sigtramp);

  /* Assume SunOS-style shared libraries.  */
  set_gdbarch_in_solib_call_trampoline (gdbarch,
					i386bsd_aout_in_solib_call_trampoline);

  tdep->jb_pc_offset = 0;

  tdep->sigtramp_start = 0xfdbfdfc0;
  tdep->sigtramp_end = 0xfdbfe000;
  tdep->sigcontext_addr = i386bsd_sigcontext_addr;
  tdep->sc_pc_offset = i386bsd_sc_pc_offset;
  tdep->sc_sp_offset = i386bsd_sc_sp_offset;
}

/* FreeBSD 3.0-RELEASE or later.  */

CORE_ADDR i386fbsd_sigtramp_start = 0xbfbfdf20;
CORE_ADDR i386fbsd_sigtramp_end = 0xbfbfdff0;

static void
i386fbsdaout_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Obviously FreeBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  /* FreeBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  /* FreeBSD uses a different memory layout.  */
  tdep->sigtramp_start = i386fbsd_sigtramp_start;
  tdep->sigtramp_end = i386fbsd_sigtramp_end;
}

static void
i386fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* It's almost identical to FreeBSD a.out.  */
  i386fbsdaout_init_abi (info, gdbarch);

  /* Except that it uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  /* FreeBSD ELF uses SVR4-style shared libraries.  */
  set_gdbarch_in_solib_call_trampoline (gdbarch,
					generic_in_solib_call_trampoline);
}

/* FreeBSD 4.0-RELEASE or later.  */

/* From <machine/signal.h>.  */
int i386fbsd4_sc_pc_offset = 76;
int i386fbsd4_sc_sp_offset = 88;

static void
i386fbsd4_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Inherit stuff from older releases.  We assume that FreeBSD
     4.0-RELEASE always uses ELF.  */
  i386fbsd_init_abi (info, gdbarch);

  /* FreeBSD 4.0 introduced a new `struct sigcontext'.  */
  tdep->sc_pc_offset = i386fbsd4_sc_pc_offset;
  tdep->sc_sp_offset = i386fbsd4_sc_sp_offset;
}


static enum gdb_osabi
i386bsd_aout_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "a.out-i386-netbsd") == 0)
    return GDB_OSABI_NETBSD_AOUT;

  if (strcmp (bfd_get_target (abfd), "a.out-i386-freebsd") == 0)
    return GDB_OSABI_FREEBSD_AOUT;

  return GDB_OSABI_UNKNOWN;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386bsd_tdep (void);

void
_initialize_i386bsd_tdep (void)
{
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_aout_flavour,
				  i386bsd_aout_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_FREEBSD_AOUT,
			  i386fbsdaout_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_FREEBSD_ELF,
			  i386fbsd4_init_abi);
}
