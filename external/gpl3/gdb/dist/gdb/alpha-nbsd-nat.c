/* Native-dependent code for NetBSD/alpha.

   Copyright (C) 2018 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "regcache.h"

#include "alpha-tdep.h"
#include "alpha-bsd-tdep.h"
#include "inf-ptrace.h"

#include "nbsd-nat.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_alphanbsd_nat (void);

void
_initialize_alphanbsd_nat (void)
{
  struct target_ops *t;

  t = alphabsd_target ();
  nbsd_nat_add_target (t);
}
