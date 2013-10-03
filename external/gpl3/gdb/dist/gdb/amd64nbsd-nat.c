/* Native-dependent code for NetBSD/amd64.

   Copyright (C) 2003-2013 Free Software Foundation, Inc.

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
#include "target.h"

#include "gdb_assert.h"

#include "nbsd-nat.h"
#include "amd64-tdep.h"
#include "amd64-nat.h"

/* Mapping between the general-purpose registers in NetBSD/amd64
   `struct reg' format and GDB's register cache layout for
   NetBSD/i386.

   Note that most (if not all) NetBSD/amd64 registers are 64-bit,
   while the NetBSD/i386 registers are all 32-bit, but since we're
   little-endian we get away with that.  */

/* From <machine/reg.h>.  */
static int amd64nbsd32_r_reg_offset[] =
{
  14 * 8,			/* %eax */
  3 * 8,			/* %ecx */
  2 * 8,			/* %edx */
  13 * 8,			/* %ebx */
  24 * 8,			/* %esp */
  12 * 8,			/* %ebp */
  1 * 8,			/* %esi */
  0 * 8,			/* %edi */
  21 * 8,			/* %eip */
  23 * 8,			/* %eflags */
  22 * 8,			/* %cs */
  25 * 8,			/* %ss */
  18 * 8,			/* %ds */
  17 * 8,			/* %es */
  16 * 8,			/* %fs */
  15 * 8			/* %gs */
};


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_amd64nbsd_nat (void);

void
_initialize_amd64nbsd_nat (void)
{
  struct target_ops *t;

  amd64_native_gregset32_reg_offset = amd64nbsd32_r_reg_offset;
  amd64_native_gregset32_num_regs = ARRAY_SIZE (amd64nbsd32_r_reg_offset);
  amd64_native_gregset64_reg_offset = amd64nbsd_r_reg_offset;

  /* Add some extra features to the common *BSD/amd64 target.  */
  t = amd64bsd_target ();
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  add_target (t);
}
