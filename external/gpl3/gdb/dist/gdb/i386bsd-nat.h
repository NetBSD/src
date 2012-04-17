/* Native-dependent code for modern i386 BSD's.

   Copyright (C) 2004, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

#ifndef I386BSD_NAT_H
#define I386BSD_NAT_H

/* Create a prototype *BSD/i386 target.  The client can override it
   with local methods.  */

extern struct target_ops *i386bsd_target (void);

/* low level i386 debug register functions used in i386fbsd-nat.c.  */

extern void i386bsd_dr_set_control (unsigned long control);

extern void i386bsd_dr_set_addr (int regnum, CORE_ADDR addr);

extern void i386bsd_dr_reset_addr (int regnum);

extern unsigned long i386bsd_dr_get_status (void);

extern void i386bsd_supply_gregset (struct regcache *regcache,
				    const void *gregs);

extern void i386bsd_collect_gregset (const struct regcache *regcache,
				     void *gregs, int regnum);

#endif /* i386bsd-nat.h */
