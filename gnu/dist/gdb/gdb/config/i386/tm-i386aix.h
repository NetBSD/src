// OBSOLETE /* Macro defintions for IBM AIX PS/2 (i386).
// OBSOLETE    Copyright 1986, 1987, 1989, 1992, 1993, 1994, 1995, 2000
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /* Changes for IBM AIX PS/2 by Minh Tran-Le (tranle@intellicorp.com).  */
// OBSOLETE 
// OBSOLETE #ifndef TM_I386AIX_H
// OBSOLETE #define TM_I386AIX_H 1
// OBSOLETE 
// OBSOLETE #include "i386/tm-i386.h"
// OBSOLETE #include <sys/reg.h>
// OBSOLETE 
// OBSOLETE #ifndef I386
// OBSOLETE #define I386 1
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /* AIX/i386 has FPU support.  However, the native configuration (which
// OBSOLETE    is the only supported configuration) doesn't make the FPU control
// OBSOLETE    registers available.  Override the appropriate symbols such that
// OBSOLETE    only the normal FPU registers are included in GDB's register array.  */
// OBSOLETE 
// OBSOLETE #undef NUM_FPREGS
// OBSOLETE #define NUM_FPREGS (8)
// OBSOLETE 
// OBSOLETE #undef NUM_REGS
// OBSOLETE #define NUM_REGS (NUM_GREGS + NUM_FPREGS)
// OBSOLETE 
// OBSOLETE #undef REGISTER_BYTES
// OBSOLETE #define REGISTER_BYTES (SIZEOF_GREGS + SIZEOF_FPU_REGS)
// OBSOLETE 
// OBSOLETE #endif /* TM_I386AIX_H */
