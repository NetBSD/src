/* TUI window generic functions.
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

#ifndef _TUI_WIN_H
#define _TUI_WIN_H

/*****************************************
** TYPE DEFINITIONS                        **
******************************************/



/*****************************************
** PUBLIC FUNCTION EXTERNAL DECLS        **
******************************************/
extern void tuiScrollForward (TuiWinInfoPtr, int);
extern void tuiScrollBackward (TuiWinInfoPtr, int);
extern void tuiScrollLeft (TuiWinInfoPtr, int);
extern void tuiScrollRight (TuiWinInfoPtr, int);
extern void tui_scroll (TuiScrollDirection, TuiWinInfoPtr, int);
extern void tuiSetWinFocusTo (TuiWinInfoPtr);
extern void tuiResizeAll (void);
extern void tuiRefreshAll (void);
extern void tuiSigwinchHandler (int);

extern chtype tui_border_ulcorner;
extern chtype tui_border_urcorner;
extern chtype tui_border_lrcorner;
extern chtype tui_border_llcorner;
extern chtype tui_border_vline;
extern chtype tui_border_hline;
extern int tui_border_attrs;
extern int tui_active_border_attrs;

extern int tui_update_variables ();

/* Update gdb's knowledge of the terminal size.  */
extern void tui_update_gdb_sizes (void);

#endif
/*_TUI_WIN_H*/
