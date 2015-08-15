/* Target-dependent code for OpenBSD.

   Copyright (C) 2005-2014 Free Software Foundation, Inc.

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

#ifndef OBSD_TDEP_H
#define OBSD_TDEP_H 1

struct gdbarch;

CORE_ADDR obsd_skip_solib_resolver (struct gdbarch *, CORE_ADDR);

#endif /* obsd-tdep.h */
