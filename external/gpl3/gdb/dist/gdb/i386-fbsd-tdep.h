/* Target-dependent code for FreeBSD x86.

   Copyright (C) 2015-2017 Free Software Foundation, Inc.

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

#ifndef I386_FBSD_TDEP_H
#define I386_FBSD_TDEP_H

/* Get XSAVE extended state xcr0 from core dump.  */
extern uint64_t i386fbsd_core_read_xcr0 (bfd *abfd);

/* The format of the XSAVE extended area is determined by hardware.
   Cores store the XSAVE extended area in a NT_X86_XSTATE note that
   matches the layout on Linux.  */
#define I386_FBSD_XSAVE_XCR0_OFFSET 464

#endif /* i386-fbsd-tdep.h */
