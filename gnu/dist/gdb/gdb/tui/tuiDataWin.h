/* Data/register window display.
   Copyright 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
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

#ifndef _TUI_DATAWIN_H
#define _TUI_DATAWIN_H


/*****************************************
** TYPE DEFINITIONS                        **
******************************************/



/*****************************************
** PUBLIC FUNCTION EXTERNAL DECLS        **
******************************************/
extern void tuiEraseDataContent (char *);
extern void tuiDisplayAllData (void);
extern void tuiCheckDataValues (struct frame_info *);
extern void tuiDisplayDataFromLine (int);
extern int tuiFirstDataItemDisplayed (void);
extern int tuiFirstDataElementNoInLine (int);
extern void tuiDeleteDataContentWindows (void);
extern void tuiRefreshDataWin (void);
extern void tuiDisplayDataFrom (int, int);
extern void tuiVerticalDataScroll (TuiScrollDirection, int);

#endif
/*_TUI_DATAWIN_H*/
