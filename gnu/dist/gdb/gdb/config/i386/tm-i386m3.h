// OBSOLETE /* Macro definitions for i386, Mach 3.0
// OBSOLETE    Copyright 1992, 1993, 1995, 1999 Free Software Foundation, Inc.
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
// OBSOLETE /* Include common definitions for Mach3 systems */
// OBSOLETE #include "config/nm-m3.h"
// OBSOLETE 
// OBSOLETE /* Define offsets to access CPROC stack when it does not have
// OBSOLETE  * a kernel thread.
// OBSOLETE  */
// OBSOLETE #define MACHINE_CPROC_SP_OFFSET 20
// OBSOLETE #define MACHINE_CPROC_PC_OFFSET 16
// OBSOLETE #define MACHINE_CPROC_FP_OFFSET 12
// OBSOLETE 
// OBSOLETE /* Thread flavors used in re-setting the T bit.
// OBSOLETE  * @@ this is also bad for cross debugging.
// OBSOLETE  */
// OBSOLETE #define TRACE_FLAVOR		i386_THREAD_STATE
// OBSOLETE #define TRACE_FLAVOR_SIZE	i386_THREAD_STATE_COUNT
// OBSOLETE #define TRACE_SET(x,state) \
// OBSOLETE   	((struct i386_thread_state *)state)->efl |= 0x100
// OBSOLETE #define TRACE_CLEAR(x,state) \
// OBSOLETE   	((((struct i386_thread_state *)state)->efl &= ~0x100), 1)
// OBSOLETE 
// OBSOLETE /* we can do it */
// OBSOLETE #define ATTACH_DETACH 1
// OBSOLETE 
// OBSOLETE /* Sigh. There should be a file for i386 but no sysv stuff in it */
// OBSOLETE #include "i386/tm-i386.h"
// OBSOLETE 
// OBSOLETE /* I want to test this float info code. See comment in tm-i386v.h */
// OBSOLETE #undef FLOAT_INFO
// OBSOLETE #define FLOAT_INFO { i386_mach3_float_info (); }
// OBSOLETE 
// OBSOLETE /* Address of end of stack space.
// OBSOLETE  * for MACH, see <machine/vmparam.h>
// OBSOLETE  * @@@ I don't know what is in the 5 ints...
// OBSOLETE  */
// OBSOLETE #undef  STACK_END_ADDR
// OBSOLETE #define STACK_END_ADDR (0xc0000000-sizeof(int [5]))
