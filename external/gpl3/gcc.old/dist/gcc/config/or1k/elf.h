/* Definitions for rtems targeting an OpenRisc OR1K using COFF
   ??? this is for OR1K, but the rest of the above seems bogus.
   Copyright (C) 1996, 1997, 2005 Free Software Foundation, Inc.
   Contributed by Joel Sherrill (joel@OARcorp.com).

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Use ELF */
#undef  OBJECT_FORMAT_ELF
#define OBJECT_FORMAT_ELF

/* or1k debug info support is controlled by tm.h header files we include:
   dbxelf.h enables optional stabs debug info.
   elfos.h sets PREFERRED_DEBUGGING_TYPE to DWARF2_DEBUG .  */

#define DRIVER_SELF_SPECS "%{!mno-newlib:-mnewlib}"
