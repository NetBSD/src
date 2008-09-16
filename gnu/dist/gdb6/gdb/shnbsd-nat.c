/* Native-dependent code for SuperH running NetBSD, for GDB.

   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
   Contributed by Wasabi Systems, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "defs.h"
#include "inferior.h"
#include "regcache.h"

#include "sh-tdep.h"
#include "shnbsd-tdep.h"
#include "inf-ptrace.h"

#include "nbsd-nat.h"

/* Determine if PT_GETREGS fetches this register. */
#define GETREGS_SUPPLIES(regno) \
  (((regno) >= R0_REGNUM && (regno) <= (R0_REGNUM + 15)) \
|| (regno) == PC_REGNUM || (regno) == PR_REGNUM \
|| (regno) == MACH_REGNUM || (regno) == MACL_REGNUM \
|| (regno) == SR_REGNUM)

void
fetch_inferior_registers (int regno)
{
  if (regno == -1 || GETREGS_SUPPLIES (regno))
    {
      struct reg inferior_registers;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));

      shnbsd_supply_reg (current_regcache, (char *) &inferior_registers, regno);

      if (regno != -1)
	return;
    }
}

void
store_inferior_registers (int regno)
{
  if (regno == -1 || GETREGS_SUPPLIES (regno))
    {
      struct reg inferior_registers;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));

      shnbsd_fill_reg (current_regcache, (char *) &inferior_registers, regno);

      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't set registers"));

      if (regno != -1)
	return;
    }
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_shnbsd_nat (void);

void
_initialize_shnbsd_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = fetch_inferior_registers;
  t->to_store_registers = store_inferior_registers;
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  add_target (t);
}
