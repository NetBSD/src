/* TUI display source window.
   Copyright 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Hewlett-Packard Company.

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

#ifndef _TUI_SOURCE_H
#define _TUI_SOURCE_H

#include "defs.h"

extern TuiStatus tuiSetSourceContent (struct symtab *, int, int);
extern void tuiShowSource (struct symtab *, TuiLineOrAddress, int);
extern int tuiSourceIsDisplayed (char *);
extern void tuiVerticalSourceScroll (TuiScrollDirection, int);

#endif
/*_TUI_SOURCE_H*/
