/* Macro definitions for GDB on an Intel i386 running SVR4.
   Copyright 1991, 1994, 1995, 1998, 1999, 2000, 2002
   Free Software Foundation, Inc.
   Written by Fred Fish at Cygnus Support (fnf@cygnus.com)

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef TM_I386V4_H
#define TM_I386V4_H 1

/* Pick up most of what we need from the generic i386 target include file.  */
#include "i386/tm-i386.h"

/* It is unknown which, if any, SVR4 assemblers do not accept dollar signs
   in identifiers.  The default in G++ is to use dots instead, for all SVR4
   systems, so we make that our default also.  FIXME: There should be some
   way to get G++ to tell us what CPLUS_MARKER it is using, perhaps by
   stashing it in the debugging information as part of the name of an
   invented symbol ("gcc_cplus_marker$" for example). */

#undef CPLUS_MARKER
#define CPLUS_MARKER '.'

#endif /* ifndef TM_I386V4_H */
