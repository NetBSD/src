// OBSOLETE /* Native support for i386 aix ps/2.
// OBSOLETE    Copyright 1986, 1987, 1989, 1992, 1993 Free Software Foundation, Inc.
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
// OBSOLETE /*
// OBSOLETE  * Changes for IBM AIX PS/2 by Minh Tran-Le (tranle@intellicorp.com)
// OBSOLETE  * Revision:     5-May-93 00:11:35
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #ifndef NM_I386AIX_H
// OBSOLETE #define NM_I386AIX_H 1
// OBSOLETE 
// OBSOLETE /* code to execute to print interesting information about the
// OBSOLETE  * floating point processor (if any)
// OBSOLETE  * No need to define if there is nothing to do.
// OBSOLETE  */
// OBSOLETE #define FLOAT_INFO { i386_float_info (); }
// OBSOLETE 
// OBSOLETE /* This is the amount to subtract from u.u_ar0
// OBSOLETE    to get the offset in the core file of the register values.  */
// OBSOLETE #undef  KERNEL_U_ADDR
// OBSOLETE #define KERNEL_U_ADDR 0xf03fd000
// OBSOLETE 
// OBSOLETE /* Override copies of {fetch,store}_inferior_registers in infptrace.c.  */
// OBSOLETE #define FETCH_INFERIOR_REGISTERS
// OBSOLETE 
// OBSOLETE #endif /* NM_I386AIX_H */
