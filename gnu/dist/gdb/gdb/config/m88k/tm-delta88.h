// OBSOLETE /* Target machine description for Motorola Delta 88 box, for GDB.
// OBSOLETE    Copyright 1986, 1987, 1988, 1989, 1990, 1991, 1993, 1994
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
// OBSOLETE #include "m88k/tm-m88k.h"
// OBSOLETE 
// OBSOLETE #define DELTA88
// OBSOLETE 
// OBSOLETE #define IN_SIGTRAMP(pc, name) ((name) && STREQ ("_sigcode", (name)))
// OBSOLETE #define SIGTRAMP_FRAME_FIXUP(frame) (frame) += 0x20
// OBSOLETE #define SIGTRAMP_SP_FIXUP(sp) (sp) = read_memory_integer((sp), 4)
