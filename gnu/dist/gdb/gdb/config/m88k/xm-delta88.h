// OBSOLETE /* Host machine description for Motorola Delta 88 system, for GDB.
// OBSOLETE    Copyright 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993
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
// OBSOLETE #if !defined (USG)
// OBSOLETE #define USG 1
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #include <sys/param.h>
// OBSOLETE #include <sys/time.h>
// OBSOLETE 
// OBSOLETE #define HAVE_TERMIO
// OBSOLETE 
// OBSOLETE /*#define USIZE 2048 */
// OBSOLETE /*#define NBPG NBPC */
// OBSOLETE /* Might be defined in <sys/param.h>.  I suspect this define was a relic
// OBSOLETE    from before when BFD did core files.  */
// OBSOLETE /* #define UPAGES USIZE */
// OBSOLETE 
// OBSOLETE /* This is the amount to subtract from u.u_ar0
// OBSOLETE    to get the offset in the core file of the register values.  */
// OBSOLETE 
// OBSOLETE /* Since registers r0 through r31 are stored directly in the struct ptrace_user,
// OBSOLETE    (for m88k BCS)
// OBSOLETE    the ptrace_user offsets are sufficient and KERNEL_U_ADDRESS can be 0 */
// OBSOLETE 
// OBSOLETE #define KERNEL_U_ADDR 0
