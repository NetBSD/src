/* Native-dependent code for modern i386 BSD's.

   Copyright (C) 2004 Free Software Foundation, Inc.

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

#ifndef I386BSD_NAT_H
#define I386BSD_NAT_H

/* Create a prototype *BSD/i386 target.  The client can override it
   with local methods.  */

extern struct target_ops *i386bsd_target (void);

#endif /* i386bsd-nat.h */
