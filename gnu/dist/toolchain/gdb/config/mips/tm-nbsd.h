/* Macro definitions for Mips running under NetBSD.
   Copyright 1994 Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef TM_NBSD_H
#define TM_NBSD_H

/* Saved Pc.  Get it from sigcontext if within sigtramp.  */
#define SIGCONTEXT_PC_OFFSET 8

/* 
 * This is a generic template for both BE and LE mips targets.
 * The default endianness should be set up before this is included.
 */
#if !defined(TARGET_BYTE_ORDER_DEFAULT)
#error Must define TARGET_BYTE_ORDER_DEFAULT before including tm-nbsd.h
#endif

/* Generic definitions.  */
#include "mips/tm-mips.h"

#undef TARGET_BYTE_ORDER_SELECTABLE_P
#define TARGET_BYTE_ORDER_SELECTABLE_P	1

#undef SOFTWARE_SINGLE_STEP_P
#define SOFTWARE_SINGLE_STEP_P 1
#define SOFTWARE_SINGLE_STEP(sig, bp_p) netbsd_mips_software_single_step (sig, bp_p)

void netbsd_mips_software_single_step (unsigned int, int);

#include <tm-nbsd.h>

#endif /* TM_NBSD_H */
