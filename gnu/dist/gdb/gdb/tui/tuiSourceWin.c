/* TUI display source/assembly window.

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
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "value.h"

#include "tui.h"
#include "tuiData.h"
#include "tuiStack.h"
#include "tuiWin.h"
#include "tuiGeneralWin.h"
#include "tuiSourceWin.h"
#include "tuiSource.h"
#include "tuiDisassem.h"


/* Function to display the "main" routine.  */
void
tui_display_main (void)
{
  if ((sourceWindows ())->count > 0)
    {
      CORE_ADDR addr;

      addr = tuiGetBeginAsmAddress ();
      if (addr != (CORE_ADDR) 0)
	{
	  struct symtab_and_line sal;

	  tuiUpdateSourceWindowsWithAddr (addr);
	  sal = find_pc_line (addr, 0);
          if (sal.symtab)
             tuiUpdateLocatorFilename (sal.symtab->filename);
          else
             tuiUpdateLocatorFilename ("??");
	}
    }
}



/*
   ** tuiUpdateSourceWindow().
   **    Function to display source in the source window.  This function
   **    initializes the horizontal scroll to 0.
 */
void
tuiUpdateSourceWindow (TuiWinInfoPtr winInfo, struct symtab *s,
                       TuiLineOrAddress lineOrAddr, int noerror)
{
  winInfo->detail.sourceInfo.horizontalOffset = 0;
  tuiUpdateSourceWindowAsIs (winInfo, s, lineOrAddr, noerror);

  return;
}				/* tuiUpdateSourceWindow */


/*
   ** tuiUpdateSourceWindowAsIs().
   **        Function to display source in the source/asm window.  This
   **        function shows the source as specified by the horizontal offset.
 */
void
tuiUpdateSourceWindowAsIs (TuiWinInfoPtr winInfo, struct symtab *s,
                           TuiLineOrAddress lineOrAddr, int noerror)
{
  TuiStatus ret;

  if (winInfo->generic.type == SRC_WIN)
    ret = tuiSetSourceContent (s, lineOrAddr.lineNo, noerror);
  else
    ret = tuiSetDisassemContent (lineOrAddr.addr);

  if (ret == TUI_FAILURE)
    {
      tuiClearSourceContent (winInfo, EMPTY_SOURCE_PROMPT);
      tuiClearExecInfoContent (winInfo);
    }
  else
    {
      tui_update_breakpoint_info (winInfo, 0);
      tuiShowSourceContent (winInfo);
      tuiUpdateExecInfo (winInfo);
      if (winInfo->generic.type == SRC_WIN)
	{
	  current_source_line = lineOrAddr.lineNo +
	    (winInfo->generic.contentSize - 2);
	  current_source_symtab = s;
	  /*
	     ** If the focus was in the asm win, put it in the src
	     ** win if we don't have a split layout
	   */
	  if (tuiWinWithFocus () == disassemWin &&
	      currentLayout () != SRC_DISASSEM_COMMAND)
	    tuiSetWinFocusTo (srcWin);
	}
    }


  return;
}				/* tuiUpdateSourceWindowAsIs */


/*
   ** tuiUpdateSourceWindowsWithAddr().
   **        Function to ensure that the source and/or disassemly windows
   **        reflect the input address.
 */
void
tuiUpdateSourceWindowsWithAddr (CORE_ADDR addr)
{
  if (addr != 0)
    {
      struct symtab_and_line sal;
      TuiLineOrAddress l;
      
      switch (currentLayout ())
	{
	case DISASSEM_COMMAND:
	case DISASSEM_DATA_COMMAND:
	  tuiShowDisassem (addr);
	  break;
	case SRC_DISASSEM_COMMAND:
	  tuiShowDisassemAndUpdateSource (addr);
	  break;
	default:
	  sal = find_pc_line (addr, 0);
	  l.lineNo = sal.line;
	  tuiShowSource (sal.symtab, l, FALSE);
	  break;
	}
    }
  else
    {
      int i;

      for (i = 0; i < (sourceWindows ())->count; i++)
	{
	  TuiWinInfoPtr winInfo = (TuiWinInfoPtr) (sourceWindows ())->list[i];

	  tuiClearSourceContent (winInfo, EMPTY_SOURCE_PROMPT);
	  tuiClearExecInfoContent (winInfo);
	}
    }

  return;
}				/* tuiUpdateSourceWindowsWithAddr */

/*
   ** tuiUpdateSourceWindowsWithLine().
   **        Function to ensure that the source and/or disassemly windows
   **        reflect the input address.
 */
void
tuiUpdateSourceWindowsWithLine (struct symtab *s, int line)
{
  CORE_ADDR pc;
  TuiLineOrAddress l;
  
  switch (currentLayout ())
    {
    case DISASSEM_COMMAND:
    case DISASSEM_DATA_COMMAND:
      find_line_pc (s, line, &pc);
      tuiUpdateSourceWindowsWithAddr (pc);
      break;
    default:
      l.lineNo = line;
      tuiShowSource (s, l, FALSE);
      if (currentLayout () == SRC_DISASSEM_COMMAND)
	{
	  find_line_pc (s, line, &pc);
	  tuiShowDisassem (pc);
	}
      break;
    }

  return;
}				/* tuiUpdateSourceWindowsWithLine */

/*
   ** tuiClearSourceContent().
 */
void
tuiClearSourceContent (TuiWinInfoPtr winInfo, int displayPrompt)
{
  if (m_winPtrNotNull (winInfo))
    {
      register int i;

      winInfo->generic.contentInUse = FALSE;
      tuiEraseSourceContent (winInfo, displayPrompt);
      for (i = 0; i < winInfo->generic.contentSize; i++)
	{
	  TuiWinElementPtr element =
	  (TuiWinElementPtr) winInfo->generic.content[i];
	  element->whichElement.source.hasBreak = FALSE;
	  element->whichElement.source.isExecPoint = FALSE;
	}
    }

  return;
}				/* tuiClearSourceContent */


/*
   ** tuiEraseSourceContent().
 */
void
tuiEraseSourceContent (TuiWinInfoPtr winInfo, int displayPrompt)
{
  int xPos;
  int halfWidth = (winInfo->generic.width - 2) / 2;

  if (winInfo->generic.handle != (WINDOW *) NULL)
    {
      werase (winInfo->generic.handle);
      checkAndDisplayHighlightIfNeeded (winInfo);
      if (displayPrompt == EMPTY_SOURCE_PROMPT)
	{
	  char *noSrcStr;

	  if (winInfo->generic.type == SRC_WIN)
	    noSrcStr = NO_SRC_STRING;
	  else
	    noSrcStr = NO_DISASSEM_STRING;
	  if (strlen (noSrcStr) >= halfWidth)
	    xPos = 1;
	  else
	    xPos = halfWidth - strlen (noSrcStr);
	  mvwaddstr (winInfo->generic.handle,
		     (winInfo->generic.height / 2),
		     xPos,
		     noSrcStr);

	  /* elz: added this function call to set the real contents of
	     the window to what is on the  screen, so that later calls
	     to refresh, do display
	     the correct stuff, and not the old image */

	  tuiSetSourceContentNil (winInfo, noSrcStr);
	}
      tuiRefreshWin (&winInfo->generic);
    }
  return;
}				/* tuiEraseSourceContent */


/* Redraw the complete line of a source or disassembly window.  */
static void
tui_show_source_line (TuiWinInfoPtr winInfo, int lineno)
{
  TuiWinElementPtr line;
  int x, y;

  line = (TuiWinElementPtr) winInfo->generic.content[lineno - 1];
  if (line->whichElement.source.isExecPoint)
    wattron (winInfo->generic.handle, A_STANDOUT);

  mvwaddstr (winInfo->generic.handle, lineno, 1,
             line->whichElement.source.line);
  if (line->whichElement.source.isExecPoint)
    wattroff (winInfo->generic.handle, A_STANDOUT);

  /* Clear to end of line but stop before the border.  */
  getyx (winInfo->generic.handle, y, x);
  while (x + 1 < winInfo->generic.width)
    {
      waddch (winInfo->generic.handle, ' ');
      getyx (winInfo->generic.handle, y, x);
    }
}

/*
   ** tuiShowSourceContent().
 */
void
tuiShowSourceContent (TuiWinInfoPtr winInfo)
{
  if (winInfo->generic.contentSize > 0)
    {
      int lineno;

      for (lineno = 1; lineno <= winInfo->generic.contentSize; lineno++)
        tui_show_source_line (winInfo, lineno);
    }
  else
    tuiEraseSourceContent (winInfo, TRUE);

  checkAndDisplayHighlightIfNeeded (winInfo);
  tuiRefreshWin (&winInfo->generic);
  winInfo->generic.contentInUse = TRUE;
}


/*
   ** tuiHorizontalSourceScroll().
   **      Scroll the source forward or backward horizontally
 */
void
tuiHorizontalSourceScroll (TuiWinInfoPtr winInfo,
                           TuiScrollDirection direction,
                           int numToScroll)
{
  if (winInfo->generic.content != (OpaquePtr) NULL)
    {
      int offset;
      struct symtab *s;

      if (current_source_symtab == (struct symtab *) NULL)
	s = find_pc_symtab (selected_frame->pc);
      else
	s = current_source_symtab;

      if (direction == LEFT_SCROLL)
	offset = winInfo->detail.sourceInfo.horizontalOffset + numToScroll;
      else
	{
	  if ((offset =
	     winInfo->detail.sourceInfo.horizontalOffset - numToScroll) < 0)
	    offset = 0;
	}
      winInfo->detail.sourceInfo.horizontalOffset = offset;
      tuiUpdateSourceWindowAsIs (
				  winInfo,
				  s,
				  ((TuiWinElementPtr)
				   winInfo->generic.content[0])->whichElement.source.lineOrAddr,
				  FALSE);
    }

  return;
}				/* tuiHorizontalSourceScroll */


/* Set or clear the hasBreak flag in the line whose line is lineNo.  */
void
tuiSetIsExecPointAt (TuiLineOrAddress l, TuiWinInfoPtr winInfo)
{
  int changed = 0;
  int i;
  TuiWinContent content = (TuiWinContent) winInfo->generic.content;

  i = 0;
  while (i < winInfo->generic.contentSize)
    {
      int newState;

      if (content[i]->whichElement.source.lineOrAddr.addr == l.addr)
        newState = TRUE;
      else
	newState = FALSE;
      if (newState != content[i]->whichElement.source.isExecPoint)
        {
          changed++;
          content[i]->whichElement.source.isExecPoint = newState;
          tui_show_source_line (winInfo, i + 1);
        }
      i++;
    }
  if (changed)
    tuiRefreshWin (&winInfo->generic);
}

/* Update the execution windows to show the active breakpoints.
   This is called whenever a breakpoint is inserted, removed or
   has its state changed.  */
void
tui_update_all_breakpoint_info ()
{
  TuiList* list = sourceWindows ();
  int i;

  for (i = 0; i < list->count; i++)
    {
      TuiWinInfoPtr win = (TuiWinInfoPtr) list->list[i];

      if (tui_update_breakpoint_info (win, FALSE))
        {
          tuiUpdateExecInfo (win);
        }
    }
}


/* Scan the source window and the breakpoints to update the
   hasBreak information for each line.
   Returns 1 if something changed and the execution window
   must be refreshed.  */
int
tui_update_breakpoint_info (TuiWinInfoPtr win, int current_only)
{
  int i;
  int need_refresh = 0;
  TuiSourceInfoPtr src = &win->detail.sourceInfo;

  for (i = 0; i < win->generic.contentSize; i++)
    {
      struct breakpoint *bp;
      extern struct breakpoint *breakpoint_chain;
      int mode;
      TuiSourceElement* line;

      line = &((TuiWinElementPtr) win->generic.content[i])->whichElement.source;
      if (current_only && !line->isExecPoint)
         continue;

      /* Scan each breakpoint to see if the current line has something to
         do with it.  Identify enable/disabled breakpoints as well as
         those that we already hit.  */
      mode = 0;
      for (bp = breakpoint_chain;
           bp != (struct breakpoint *) NULL;
           bp = bp->next)
        {
          if ((win == srcWin
               && bp->source_file
               && (strcmp (src->filename, bp->source_file) == 0)
               && bp->line_number == line->lineOrAddr.lineNo)
              || (win == disassemWin
                  && bp->address == line->lineOrAddr.addr))
            {
              if (bp->enable_state == bp_disabled)
                mode |= TUI_BP_DISABLED;
              else
                mode |= TUI_BP_ENABLED;
              if (bp->hit_count)
                mode |= TUI_BP_HIT;
              if (bp->cond)
                mode |= TUI_BP_CONDITIONAL;
              if (bp->type == bp_hardware_breakpoint)
                mode |= TUI_BP_HARDWARE;
            }
        }
      if (line->hasBreak != mode)
        {
          line->hasBreak = mode;
          need_refresh = 1;
        }
    }
  return need_refresh;
}


/*
   ** tuiSetExecInfoContent().
   **      Function to initialize the content of the execution info window,
   **      based upon the input window which is either the source or
   **      disassembly window.
 */
TuiStatus
tuiSetExecInfoContent (TuiWinInfoPtr winInfo)
{
  TuiStatus ret = TUI_SUCCESS;

  if (winInfo->detail.sourceInfo.executionInfo != (TuiGenWinInfoPtr) NULL)
    {
      TuiGenWinInfoPtr execInfoPtr = winInfo->detail.sourceInfo.executionInfo;

      if (execInfoPtr->content == (OpaquePtr) NULL)
	execInfoPtr->content =
	  (OpaquePtr) allocContent (winInfo->generic.height,
				    execInfoPtr->type);
      if (execInfoPtr->content != (OpaquePtr) NULL)
	{
	  int i;

          tui_update_breakpoint_info (winInfo, 1);
	  for (i = 0; i < winInfo->generic.contentSize; i++)
	    {
	      TuiWinElementPtr element;
	      TuiWinElementPtr srcElement;
              int mode;

	      element = (TuiWinElementPtr) execInfoPtr->content[i];
	      srcElement = (TuiWinElementPtr) winInfo->generic.content[i];

              memset(element->whichElement.simpleString, ' ',
                     sizeof(element->whichElement.simpleString));
              element->whichElement.simpleString[TUI_EXECINFO_SIZE - 1] = 0;

	      /* Now update the exec info content based upon the state
                 of each line as indicated by the source content.  */
              mode = srcElement->whichElement.source.hasBreak;
              if (mode & TUI_BP_HIT)
                element->whichElement.simpleString[TUI_BP_HIT_POS] =
                  (mode & TUI_BP_HARDWARE) ? 'H' : 'B';
              else if (mode & (TUI_BP_ENABLED | TUI_BP_DISABLED))
                element->whichElement.simpleString[TUI_BP_HIT_POS] =
                  (mode & TUI_BP_HARDWARE) ? 'h' : 'b';

              if (mode & TUI_BP_ENABLED)
                element->whichElement.simpleString[TUI_BP_BREAK_POS] = '+';
              else if (mode & TUI_BP_DISABLED)
                element->whichElement.simpleString[TUI_BP_BREAK_POS] = '-';

              if (srcElement->whichElement.source.isExecPoint)
                element->whichElement.simpleString[TUI_EXEC_POS] = '>';
	    }
	  execInfoPtr->contentSize = winInfo->generic.contentSize;
	}
      else
	ret = TUI_FAILURE;
    }

  return ret;
}


/*
   ** tuiShowExecInfoContent().
 */
void
tuiShowExecInfoContent (TuiWinInfoPtr winInfo)
{
  TuiGenWinInfoPtr execInfo = winInfo->detail.sourceInfo.executionInfo;
  int curLine;

  werase (execInfo->handle);
  tuiRefreshWin (execInfo);
  for (curLine = 1; (curLine <= execInfo->contentSize); curLine++)
    mvwaddstr (execInfo->handle,
	       curLine,
	       0,
	       ((TuiWinElementPtr)
		execInfo->content[curLine - 1])->whichElement.simpleString);
  tuiRefreshWin (execInfo);
  execInfo->contentInUse = TRUE;

  return;
}				/* tuiShowExecInfoContent */


/*
   ** tuiEraseExecInfoContent().
 */
void
tuiEraseExecInfoContent (TuiWinInfoPtr winInfo)
{
  TuiGenWinInfoPtr execInfo = winInfo->detail.sourceInfo.executionInfo;

  werase (execInfo->handle);
  tuiRefreshWin (execInfo);

  return;
}				/* tuiEraseExecInfoContent */

/*
   ** tuiClearExecInfoContent().
 */
void
tuiClearExecInfoContent (TuiWinInfoPtr winInfo)
{
  winInfo->detail.sourceInfo.executionInfo->contentInUse = FALSE;
  tuiEraseExecInfoContent (winInfo);

  return;
}				/* tuiClearExecInfoContent */

/*
   ** tuiUpdateExecInfo().
   **        Function to update the execution info window
 */
void
tuiUpdateExecInfo (TuiWinInfoPtr winInfo)
{
  tuiSetExecInfoContent (winInfo);
  tuiShowExecInfoContent (winInfo);
}				/* tuiUpdateExecInfo */

TuiStatus
tuiAllocSourceBuffer (TuiWinInfoPtr winInfo)
{
  register char *srcLineBuf;
  register int i, lineWidth, maxLines;
  TuiStatus ret = TUI_FAILURE;

  maxLines = winInfo->generic.height;	/* less the highlight box */
  lineWidth = winInfo->generic.width - 1;
  /*
     ** Allocate the buffer for the source lines.  Do this only once since they
     ** will be re-used for all source displays.  The only other time this will
     ** be done is when a window's size changes.
   */
  if (winInfo->generic.content == (OpaquePtr) NULL)
    {
      srcLineBuf = (char *) xmalloc ((maxLines * lineWidth) * sizeof (char));
      if (srcLineBuf == (char *) NULL)
	fputs_unfiltered (
	   "Unable to Allocate Memory for Source or Disassembly Display.\n",
			   gdb_stderr);
      else
	{
	  /* allocate the content list */
	  if ((winInfo->generic.content =
	  (OpaquePtr) allocContent (maxLines, SRC_WIN)) == (OpaquePtr) NULL)
	    {
	      tuiFree (srcLineBuf);
	      srcLineBuf = (char *) NULL;
	      fputs_unfiltered (
				 "Unable to Allocate Memory for Source or Disassembly Display.\n",
				 gdb_stderr);
	    }
	}
      for (i = 0; i < maxLines; i++)
	((TuiWinElementPtr)
	 winInfo->generic.content[i])->whichElement.source.line =
	  srcLineBuf + (lineWidth * i);
      ret = TUI_SUCCESS;
    }
  else
    ret = TUI_SUCCESS;

  return ret;
}				/* tuiAllocSourceBuffer */


/*
   ** tuiLineIsDisplayed().
   **      Answer whether the a particular line number or address is displayed
   **      in the current source window.
 */
int
tuiLineIsDisplayed (int line, TuiWinInfoPtr winInfo,
                    int checkThreshold)
{
  int isDisplayed = FALSE;
  int i, threshold;

  if (checkThreshold)
    threshold = SCROLL_THRESHOLD;
  else
    threshold = 0;
  i = 0;
  while (i < winInfo->generic.contentSize - threshold && !isDisplayed)
    {
      isDisplayed = (((TuiWinElementPtr)
		      winInfo->generic.content[i])->whichElement.source.lineOrAddr.lineNo
		     == (int) line);
      i++;
    }

  return isDisplayed;
}				/* tuiLineIsDisplayed */


/*
   ** tuiLineIsDisplayed().
   **      Answer whether the a particular line number or address is displayed
   **      in the current source window.
 */
int
tuiAddrIsDisplayed (CORE_ADDR addr, TuiWinInfoPtr winInfo,
		    int checkThreshold)
{
  int isDisplayed = FALSE;
  int i, threshold;

  if (checkThreshold)
    threshold = SCROLL_THRESHOLD;
  else
    threshold = 0;
  i = 0;
  while (i < winInfo->generic.contentSize - threshold && !isDisplayed)
    {
      isDisplayed = (((TuiWinElementPtr)
		      winInfo->generic.content[i])->whichElement.source.lineOrAddr.addr
		     == addr);
      i++;
    }

  return isDisplayed;
}


/*****************************************
** STATIC LOCAL FUNCTIONS               **
******************************************/
