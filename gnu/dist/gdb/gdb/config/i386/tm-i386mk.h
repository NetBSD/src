// OBSOLETE /* Macro definitions for i386, Mach 3.0, OSF 1/MK
// OBSOLETE    Copyright 1992, 1993, 2000 Free Software Foundation, Inc.
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
// OBSOLETE /* Until OSF switches to a newer Mach kernel that has
// OBSOLETE  * a different get_emul_vector() interface.
// OBSOLETE  */
// OBSOLETE #define MK67 1
// OBSOLETE 
// OBSOLETE #include "i386/tm-i386m3.h"
// OBSOLETE 
// OBSOLETE /* FIMXE: kettenis/2000-03-26: On OSF 1, `long double' is equivalent
// OBSOLETE    to `double'.  However, I'm not sure what is the consequence of:
// OBSOLETE 
// OBSOLETE    #define TARGET_LONG_DOUBLE_FORMAT TARGET_DOUBLE_FORMAT
// OBSOLETE    #define TARGET_LONG_DOUBLE_BIT TARGET_DOUBLE_BIT
// OBSOLETE 
// OBSOLETE    So I'll go with the current status quo instead.  It looks like this
// OBSOLETE    target won't compile anyway.  Perhaps it should be obsoleted?  */
// OBSOLETE    
// OBSOLETE #undef TARGET_LONG_DOUBLE_FORMAT
// OBSOLETE #undef TARGET_LONG_DOUBLE_BIT
