/* Native-dependent code for GNU/Linux UltraSPARC.

   Copyright (C) 2003, 2005, 2006 Free Software Foundation, Inc.

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
#include "regcache.h"

#include <sys/procfs.h>
#include "gregset.h"

#include "sparc64-tdep.h"
#include "sparc-tdep.h"
#include "sparc-nat.h"
#include "inferior.h"
#include "target.h"
#include "linux-nat.h"

static const struct sparc_gregset sparc64_linux_ptrace_gregset =
{
  16 * 8,			/* "tstate" */
  17 * 8,			/* %pc */
  18 * 8,			/* %npc */
  19 * 8,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  0 * 8,			/* %g1 */
  -1,				/* %l0 */
  4				/* sizeof (%y) */
};


void
supply_gregset (prgregset_t *gregs)
{
  sparc64_supply_gregset (sparc_gregset, current_regcache, -1, gregs);
}

void
supply_fpregset (prfpregset_t *fpregs)
{
  sparc64_supply_fpregset (current_regcache, -1, fpregs);
}

void
fill_gregset (prgregset_t *gregs, int regnum)
{
  sparc64_collect_gregset (sparc_gregset, current_regcache, regnum, gregs);
}

void
fill_fpregset (prfpregset_t *fpregs, int regnum)
{
  sparc64_collect_fpregset (current_regcache, regnum, fpregs);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_sparc64_linux_nat (void);

void
_initialize_sparc64_linux_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  /* Add our register access methods.  */
  t->to_fetch_registers = fetch_inferior_registers;
  t->to_store_registers = store_inferior_registers;

  /* Register the target.  */
  linux_nat_add_target (t);

  sparc_gregset = &sparc64_linux_ptrace_gregset;
}
