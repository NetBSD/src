/* Target-dependent definitions for sparc running NetBSD, for GDB.
   Copyright (C) 2017 Free Software Foundation, Inc.

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

#ifndef SPARC_NBSD_TDEP_H
#define SPARC_NBSD_TDEP_H

/* Register offsets for NetBSD.  */
extern const struct sparc_gregmap sparc32nbsd_gregmap;

/* Return the address of a system call's alternative return
   address.  */
extern CORE_ADDR sparcnbsd_step_trap (struct frame_info *frame,
				      unsigned long insn);

extern struct trad_frame_saved_reg *
  sparc32nbsd_sigcontext_saved_regs (struct frame_info *next_frame);

#endif /* SPARC_NBSD_TDEP_H */
