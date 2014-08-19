/* Communication between the Local Register Allocator (LRA) and
   the rest of the compiler.
   Copyright (C) 2010-2013 Free Software Foundation, Inc.
   Contributed by Vladimir Makarov <vmakarov@redhat.com>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.	If not see
<http://www.gnu.org/licenses/>.	 */

extern bool lra_simple_p;

/* Return the allocno reg class of REGNO.  If it is a reload pseudo,
   the pseudo should finally get hard register of the allocno
   class.  */
static inline enum reg_class
lra_get_allocno_class (int regno)
{
  resize_reg_info ();
  return reg_allocno_class (regno);
}

extern rtx lra_create_new_reg (enum machine_mode, rtx, enum reg_class,
			       const char *);
extern void lra_init_elimination (void);
extern rtx lra_eliminate_regs (rtx, enum machine_mode, rtx);
extern void lra (FILE *);
extern void lra_init_once (void);
extern void lra_init (void);
extern void lra_finish_once (void);
