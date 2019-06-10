/* Native-dependent code for x86 BSD's.

   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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

#ifndef X86_BSD_NAT_H
#define X86_BSD_NAT_H

/* Low level x86 XSAVE info.  */
extern size_t x86bsd_xsave_len;

/* Create a prototype *BSD/x86 target.  The client can override it
   with local methods.  */

extern struct target_ops *x86bsd_target (void);

#endif /* x86-bsd-nat.h */
