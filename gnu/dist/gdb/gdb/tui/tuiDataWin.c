/* Data/register window display.

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
#include "tuiRegs.h"


/*****************************************
** STATIC LOCAL FUNCTIONS FORWARD DECLS    **
******************************************/



/*****************************************
** PUBLIC FUNCTIONS                        **
******************************************/


/*
   ** tuiFirstDataItemDisplayed()
   **    Answer the index first element displayed.
   **    If none are displayed, then return (-1).
 */
int
tuiFirstDataItemDisplayed (void)
{
  int elementNo = (-1);
  int i;

  for (i = 0; (i < dataWin->generic.contentSize && elementNo < 0); i++)
    {
      TuiGenWinInfoPtr dataItemWin;

      dataItemWin = &((TuiWinContent)
		      dataWin->generic.content)[i]->whichElement.dataWindow;
      if (dataItemWin->handle != (WINDOW *) NULL && dataItemWin->isVisible)
	elementNo = i;
    }

  return elementNo;
}				/* tuiFirstDataItemDisplayed */


/*
   ** tuiFirstDataElementNoInLine()
   **        Answer the index of the first element in lineNo.  If lineNo is
   **        past the data area (-1) is returned.
 */
int
tuiFirstDataElementNoInLine (int lineNo)
{
  int firstElementNo = (-1);

  /*
     ** First see if there is a register on lineNo, and if so, set the
     ** first element number
   */
  if ((firstElementNo = tuiFirstRegElementNoInLine (lineNo)) == -1)
    {				/*
				   ** Looking at the general data, the 1st element on lineNo
				 */
    }

  return firstElementNo;
}				/* tuiFirstDataElementNoInLine */


/*
   ** tuiDeleteDataContentWindows()
   **        Function to delete all the item windows in the data window.
   **        This is usually done when the data window is scrolled.
 */
void
tuiDeleteDataContentWindows (void)
{
  int i;
  TuiGenWinInfoPtr dataItemWinPtr;

  for (i = 0; (i < dataWin->generic.contentSize); i++)
    {
      dataItemWinPtr = &((TuiWinContent)
		      dataWin->generic.content)[i]->whichElement.dataWindow;
      tuiDelwin (dataItemWinPtr->handle);
      dataItemWinPtr->handle = (WINDOW *) NULL;
      dataItemWinPtr->isVisible = FALSE;
    }

  return;
}				/* tuiDeleteDataContentWindows */


void
tuiEraseDataContent (char *prompt)
{
  werase (dataWin->generic.handle);
  checkAndDisplayHighlightIfNeeded (dataWin);
  if (prompt != (char *) NULL)
    {
      int halfWidth = (dataWin->generic.width - 2) / 2;
      int xPos;

      if (strlen (prompt) >= halfWidth)
	xPos = 1;
      else
	xPos = halfWidth - strlen (prompt);
      mvwaddstr (dataWin->generic.handle,
		 (dataWin->generic.height / 2),
		 xPos,
		 prompt);
    }
  wrefresh (dataWin->generic.handle);

  return;
}				/* tuiEraseDataContent */


/*
   ** tuiDisplayAllData().
   **        This function displays the data that is in the data window's
   **        content.  It does not set the content.
 */
void
tuiDisplayAllData (void)
{
  if (dataWin->generic.contentSize <= 0)
    tuiEraseDataContent (NO_DATA_STRING);
  else
    {
      tuiEraseDataContent ((char *) NULL);
      tuiDeleteDataContentWindows ();
      checkAndDisplayHighlightIfNeeded (dataWin);
      tuiDisplayRegistersFrom (0);
      /*
         ** Then display the other data
       */
      if (dataWin->detail.dataDisplayInfo.dataContent !=
	  (TuiWinContent) NULL &&
	  dataWin->detail.dataDisplayInfo.dataContentCount > 0)
	{
	}
    }
  return;
}				/* tuiDisplayAllData */


/*
   ** tuiDisplayDataFromLine()
   **        Function to display the data starting at line, lineNo, in the
   **        data window.
 */
void
tuiDisplayDataFromLine (int lineNo)
{
  int _lineNo = lineNo;

  if (lineNo < 0)
    _lineNo = 0;

  checkAndDisplayHighlightIfNeeded (dataWin);

  /* there is no general data, force regs to display (if there are any) */
  if (dataWin->detail.dataDisplayInfo.dataContentCount <= 0)
    tuiDisplayRegistersFromLine (_lineNo, TRUE);
  else
    {
      int elementNo, startLineNo;
      int regsLastLine = tuiLastRegsLineNo ();


      /* display regs if we can */
      if (tuiDisplayRegistersFromLine (_lineNo, FALSE) < 0)
	{			/*
				   ** _lineNo is past the regs display, so calc where the
				   ** start data element is
				 */
	  if (regsLastLine < _lineNo)
	    {			/* figure out how many lines each element is to obtain
				   the start elementNo */
	    }
	}
      else
	{			/*
				   ** calculate the starting element of the data display, given
				   ** regsLastLine and how many lines each element is, up to
				   ** _lineNo
				 */
	}
      /* Now display the data , starting at elementNo */
    }

  return;
}				/* tuiDisplayDataFromLine */


/*
   ** tuiDisplayDataFrom()
   **        Display data starting at element elementNo
 */
void
tuiDisplayDataFrom (int elementNo, int reuseWindows)
{
  int firstLine = (-1);

  if (elementNo < dataWin->detail.dataDisplayInfo.regsContentCount)
    firstLine = tuiLineFromRegElementNo (elementNo);
  else
    {				/* calculate the firstLine from the element number */
    }

  if (firstLine >= 0)
    {
      tuiEraseDataContent ((char *) NULL);
      if (!reuseWindows)
	tuiDeleteDataContentWindows ();
      tuiDisplayDataFromLine (firstLine);
    }

  return;
}				/* tuiDisplayDataFrom */


/*
   ** tuiRefreshDataWin()
   **        Function to redisplay the contents of the data window.
 */
void
tuiRefreshDataWin (void)
{
  tuiEraseDataContent ((char *) NULL);
  if (dataWin->generic.contentSize > 0)
    {
      int firstElement = tuiFirstDataItemDisplayed ();

      if (firstElement >= 0)	/* re-use existing windows */
	tuiDisplayDataFrom (firstElement, TRUE);
    }

  return;
}				/* tuiRefreshDataWin */


/*
   ** tuiCheckDataValues().
   **        Function to check the data values and hilite any that have changed
 */
void
tuiCheckDataValues (struct frame_info *frame)
{
  tuiCheckRegisterValues (frame);

  /* Now check any other data values that there are */
  if (m_winPtrNotNull (dataWin) && dataWin->generic.isVisible)
    {
      int i;

      for (i = 0; dataWin->detail.dataDisplayInfo.dataContentCount; i++)
	{
#ifdef LATER
	  TuiDataElementPtr dataElementPtr;
	  TuiGenWinInfoPtr dataItemWinPtr;
	  Opaque newValue;

	  dataItemPtr = &dataWin->detail.dataDisplayInfo.
	    dataContent[i]->whichElement.dataWindow;
	  dataElementPtr = &((TuiWinContent)
			     dataItemWinPtr->content)[0]->whichElement.data;
	  if value
	    has changed (dataElementPtr, frame, &newValue)
	    {
	      dataElementPtr->value = newValue;
	      update the display with the new value, hiliting it.
	    }
#endif
	}
    }
}				/* tuiCheckDataValues */


/*
   ** tuiVerticalDataScroll()
   **        Scroll the data window vertically forward or backward.
 */
void
tuiVerticalDataScroll (TuiScrollDirection scrollDirection, int numToScroll)
{
  int firstElementNo;
  int firstLine = (-1);

  firstElementNo = tuiFirstDataItemDisplayed ();
  if (firstElementNo < dataWin->detail.dataDisplayInfo.regsContentCount)
    firstLine = tuiLineFromRegElementNo (firstElementNo);
  else
    {				/* calculate the first line from the element number which is in
				   ** the general data content
				 */
    }

  if (firstLine >= 0)
    {
      int lastElementNo, lastLine;

      if (scrollDirection == FORWARD_SCROLL)
	firstLine += numToScroll;
      else
	firstLine -= numToScroll;
      tuiEraseDataContent ((char *) NULL);
      tuiDeleteDataContentWindows ();
      tuiDisplayDataFromLine (firstLine);
    }

  return;
}				/* tuiVerticalDataScroll */


/*****************************************
** STATIC LOCAL FUNCTIONS               **
******************************************/
