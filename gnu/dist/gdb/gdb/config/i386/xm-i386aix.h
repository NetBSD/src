// OBSOLETE /* Macro defintions for AIX PS/2 (i386)
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
// OBSOLETE  * Changed for IBM AIX ps/2 by Minh Tran Le (tranle@intellicorp.com)
// OBSOLETE  * Revision:    23-Oct-92 17:42:49
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #include "i386/xm-i386v.h"
// OBSOLETE 
// OBSOLETE #undef HAVE_TERMIO
// OBSOLETE #define HAVE_SGTTY
