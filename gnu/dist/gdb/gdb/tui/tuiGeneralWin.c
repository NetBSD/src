/* General window behavior.

   Copyright 1998, 1999, 2000, 2001, 2002 Free Software Foundation,
   Inc.

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

/* FIXME: cagney/2002-02-28: The GDB coding standard indicates that
   "defs.h" should be included first.  Unfortunatly some systems
   (currently Debian GNU/Linux) include the <stdbool.h> via <curses.h>
   and they clash with "bfd.h"'s definiton of true/false.  The correct
   fix is to remove true/false from "bfd.h", however, until that
   happens, hack around it by including "config.h" and <curses.h>
   first.  */

#include "config.h"
#ifdef HAVE_NCURSES_H       
#include <ncurses.h>
#else
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#endif

#include "defs.h"
#include "tui.h"
#include "tuiData.h"
#include "tuiGeneralWin.h"
#include "tuiWin.h"

/***********************
** PUBLIC FUNCTIONS
***********************/
/*
   ** tuiRefreshWin()
   **        Refresh the window
 */
void
tuiRefreshWin (TuiGenWinInfoPtr winInfo)
{
  if (winInfo->type == DATA_WIN && winInfo->contentSize > 0)
    {
      int i;

      for (i = 0; (i < winInfo->contentSize); i++)
	{
	  TuiGenWinInfoPtr dataItemWinPtr;

	  dataItemWinPtr = &((TuiWinContent)
			     winInfo->content)[i]->whichElement.dataWindow;
	  if (m_genWinPtrNotNull (dataItemWinPtr) &&
	      dataItemWinPtr->handle != (WINDOW *) NULL)
	    wrefresh (dataItemWinPtr->handle);
	}
    }
  else if (winInfo->type == CMD_WIN)
    {
      /* Do nothing */
    }
  else
    {
      if (winInfo->handle != (WINDOW *) NULL)
	wrefresh (winInfo->handle);
    }

  return;
}				/* tuiRefreshWin */


/*
   ** tuiDelwin()
   **        Function to delete the curses window, checking for null
 */
void
tuiDelwin (WINDOW * window)
{
  if (window != (WINDOW *) NULL)
    delwin (window);

  return;
}				/* tuiDelwin */


/* Draw a border arround the window.  */
void
boxWin (TuiGenWinInfoPtr winInfo, int highlightFlag)
{
  if (winInfo && winInfo->handle)
    {
      WINDOW *win;
      int attrs;

      win = winInfo->handle;
      if (highlightFlag == HILITE)
        attrs = tui_active_border_attrs;
      else
        attrs = tui_border_attrs;

      wattron (win, attrs);
      wborder (win, tui_border_vline, tui_border_vline,
               tui_border_hline, tui_border_hline,
               tui_border_ulcorner, tui_border_urcorner,
               tui_border_llcorner, tui_border_lrcorner);
      if (winInfo->title)
        mvwaddstr (win, 0, 3, winInfo->title);
      wattroff (win, attrs);
    }
}


/*
   ** unhighlightWin().
 */
void
unhighlightWin (TuiWinInfoPtr winInfo)
{
  if (m_winPtrNotNull (winInfo) && winInfo->generic.handle != (WINDOW *) NULL)
    {
      boxWin ((TuiGenWinInfoPtr) winInfo, NO_HILITE);
      wrefresh (winInfo->generic.handle);
      m_setWinHighlightOff (winInfo);
    }
}				/* unhighlightWin */


/*
   ** highlightWin().
 */
void
highlightWin (TuiWinInfoPtr winInfo)
{
  if (m_winPtrNotNull (winInfo) &&
      winInfo->canHighlight && winInfo->generic.handle != (WINDOW *) NULL)
    {
      boxWin ((TuiGenWinInfoPtr) winInfo, HILITE);
      wrefresh (winInfo->generic.handle);
      m_setWinHighlightOn (winInfo);
    }
}				/* highlightWin */


/*
   ** checkAndDisplayHighlightIfNecessay
 */
void
checkAndDisplayHighlightIfNeeded (TuiWinInfoPtr winInfo)
{
  if (m_winPtrNotNull (winInfo) && winInfo->generic.type != CMD_WIN)
    {
      if (winInfo->isHighlighted)
	highlightWin (winInfo);
      else
	unhighlightWin (winInfo);

    }
  return;
}				/* checkAndDisplayHighlightIfNeeded */


/*
   ** makeWindow().
 */
void
makeWindow (TuiGenWinInfoPtr winInfo, int boxIt)
{
  WINDOW *handle;

  handle = newwin (winInfo->height,
		   winInfo->width,
		   winInfo->origin.y,
		   winInfo->origin.x);
  winInfo->handle = handle;
  if (handle != (WINDOW *) NULL)
    {
      if (boxIt == BOX_WINDOW)
	boxWin (winInfo, NO_HILITE);
      winInfo->isVisible = TRUE;
      scrollok (handle, TRUE);
    }
}


/*
   ** makeVisible().
   **        We can't really make windows visible, or invisible.  So we
   **        have to delete the entire window when making it visible,
   **        and create it again when making it visible.
 */
void
makeVisible (TuiGenWinInfoPtr winInfo, int visible)
{
  /* Don't tear down/recreate command window */
  if (winInfo->type == CMD_WIN)
    return;

  if (visible)
    {
      if (!winInfo->isVisible)
	{
	  makeWindow (
		       winInfo,
	   (winInfo->type != CMD_WIN && !m_winIsAuxillary (winInfo->type)));
	  winInfo->isVisible = TRUE;
	}
    }
  else if (!visible &&
	   winInfo->isVisible && winInfo->handle != (WINDOW *) NULL)
    {
      winInfo->isVisible = FALSE;
      tuiDelwin (winInfo->handle);
      winInfo->handle = (WINDOW *) NULL;
    }

  return;
}				/* makeVisible */


/*
   ** makeAllVisible().
   **        Makes all windows invisible (except the command and locator windows)
 */
void
makeAllVisible (int visible)
{
  int i;

  for (i = 0; i < MAX_MAJOR_WINDOWS; i++)
    {
      if (m_winPtrNotNull (winList[i]) &&
	  ((winList[i])->generic.type) != CMD_WIN)
	{
	  if (m_winIsSourceType ((winList[i])->generic.type))
	    makeVisible ((winList[i])->detail.sourceInfo.executionInfo,
			 visible);
	  makeVisible ((TuiGenWinInfoPtr) winList[i], visible);
	}
    }

  return;
}				/* makeAllVisible */

/*
   ** refreshAll().
   **        Function to refresh all the windows currently displayed
 */
void
refreshAll (TuiWinInfoPtr * list)
{
  TuiWinType type;
  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();

  for (type = SRC_WIN; (type < MAX_MAJOR_WINDOWS); type++)
    {
      if (list[type] && list[type]->generic.isVisible)
	{
	  if (type == SRC_WIN || type == DISASSEM_WIN)
	    {
	      touchwin (list[type]->detail.sourceInfo.executionInfo->handle);
	      tuiRefreshWin (list[type]->detail.sourceInfo.executionInfo);
	    }
	  touchwin (list[type]->generic.handle);
	  tuiRefreshWin (&list[type]->generic);
	}
    }
  if (locator->isVisible)
    {
      touchwin (locator->handle);
      tuiRefreshWin (locator);
    }

  return;
}				/* refreshAll */


/*********************************
** Local Static Functions
*********************************/
