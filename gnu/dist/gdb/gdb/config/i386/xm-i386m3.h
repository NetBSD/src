// OBSOLETE /* Definitions to make GDB run on Mach 3 on an Intel 386
// OBSOLETE    Copyright 1986, 1987, 1989, 1991, 1993, 1994, 1996
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
// OBSOLETE /* Do implement the attach and detach commands.  */
// OBSOLETE #define ATTACH_DETACH	1
// OBSOLETE 
// OBSOLETE /* Not needeed */
// OBSOLETE #define KERNEL_U_ADDR 0
// OBSOLETE 
// OBSOLETE #ifndef EMULATOR_BASE
// OBSOLETE /* For EMULATOR_BASE and EMULATOR_END.
// OBSOLETE  * OSF 1/MK has different values in some other place.
// OBSOLETE  */
// OBSOLETE #include <machine/vmparam.h>
// OBSOLETE #endif /* EMULATOR_BASE */
