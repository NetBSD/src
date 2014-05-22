/* Interface for functions using gregset and fpregset types.
   Copyright (C) 2000-2013 Free Software Foundation, Inc.

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

#ifndef GREGSET_H
#define GREGSET_H

#ifdef HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif

#ifndef GDB_GREGSET_T
#define GDB_GREGSET_T gregset_t
#endif

#ifndef GDB_FPREGSET_T
#define GDB_FPREGSET_T fpregset_t
#endif

typedef GDB_GREGSET_T gdb_gregset_t;
typedef GDB_FPREGSET_T gdb_fpregset_t;

struct regcache;

/* A gregset is a data structure supplied by the native OS containing
   the general register values of the debugged process.  Usually this
   includes integer registers and control registers.  An fpregset is a
   data structure containing the floating point registers.  These data
   structures were originally a part of the /proc interface, but have
   been borrowed or copied by other GDB targets, eg. GNU/Linux.  */

/* Copy register values from the native target gregset/fpregset
   into GDB's internal register cache.  */

extern void supply_gregset (struct regcache *regcache,
			    const gdb_gregset_t *gregs);
extern void supply_fpregset (struct regcache *regcache,
			     const gdb_fpregset_t *fpregs);

/* Copy register values from GDB's register cache into
   the native target gregset/fpregset.  If regno is -1, 
   copy all the registers.  */

extern void fill_gregset (const struct regcache *regcache,
			  gdb_gregset_t *gregs, int regno);
extern void fill_fpregset (const struct regcache *regcache,
			   gdb_fpregset_t *fpregs, int regno);

#endif
