/* General window behavior.
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

#ifndef TUI_GENERAL_WIN_H
#define TUI_GENERAL_WIN_H

/*
   ** Functions
 */
extern void unhighlightWin (TuiWinInfoPtr);
extern void makeVisible (TuiGenWinInfoPtr, int);
extern void makeAllVisible (int);
extern void makeWindow (TuiGenWinInfoPtr, int);
extern TuiWinInfoPtr copyWin (TuiWinInfoPtr);
extern void boxWin (TuiGenWinInfoPtr, int);
extern void highlightWin (TuiWinInfoPtr);
extern void checkAndDisplayHighlightIfNeeded (TuiWinInfoPtr);
extern void refreshAll (TuiWinInfoPtr *);
extern void tuiDelwin (WINDOW * window);
extern void tuiRefreshWin (TuiGenWinInfoPtr);

/*
   ** Macros
 */
#define    m_beVisible(winInfo)   makeVisible((TuiGenWinInfoPtr)(winInfo), TRUE)
#define    m_beInvisible(winInfo) \
                            makeVisible((TuiGenWinInfoPtr)(winInfo), FALSE)
#define    m_allBeVisible()       makeAllVisible(TRUE)
#define m_allBeInvisible()        makeAllVisible(FALSE)

#endif /*TUI_GENERAL_WIN_H */
