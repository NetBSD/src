/* TUI layout window management.

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
#include "command.h"
#include "symtab.h"
#include "frame.h"
#include <ctype.h>

#include "tui.h"
#include "tuiData.h"
#include "tuiDataWin.h"
#include "tuiGeneralWin.h"
#include "tuiStack.h"
#include "tuiRegs.h"
#include "tuiWin.h"
#include "tuiSourceWin.h"
#include "tuiDisassem.h"

/*******************************
** Static Local Decls
********************************/
static void showLayout (TuiLayoutType);
static void _initGenWinInfo (TuiGenWinInfoPtr, TuiWinType, int, int, int, int);
static void _initAndMakeWin (Opaque *, TuiWinType, int, int, int, int, int);
static void _showSourceOrDisassemAndCommand (TuiLayoutType);
static void _makeSourceOrDisassemWindow (TuiWinInfoPtr *, TuiWinType, int, int);
static void _makeCommandWindow (TuiWinInfoPtr *, int, int);
static void _makeSourceWindow (TuiWinInfoPtr *, int, int);
static void _makeDisassemWindow (TuiWinInfoPtr *, int, int);
static void _makeDataWindow (TuiWinInfoPtr *, int, int);
static void _showSourceCommand (void);
static void _showDisassemCommand (void);
static void _showSourceDisassemCommand (void);
static void _showData (TuiLayoutType);
static TuiLayoutType _nextLayout (void);
static TuiLayoutType _prevLayout (void);
static void _tuiLayout_command (char *, int);
static void _tuiToggleLayout_command (char *, int);
static void _tuiToggleSplitLayout_command (char *, int);
static CORE_ADDR _extractDisplayStartAddr (void);
static void _tuiHandleXDBLayout (TuiLayoutDefPtr);


/***************************************
** DEFINITIONS
***************************************/

#define LAYOUT_USAGE     "Usage: layout prev | next | <layout_name> \n"

/* Show the screen layout defined.  */
static void
showLayout (TuiLayoutType layout)
{
  TuiLayoutType curLayout = currentLayout ();

  if (layout != curLayout)
    {
      /*
         ** Since the new layout may cause changes in window size, we
         ** should free the content and reallocate on next display of
         ** source/asm
       */
      freeAllSourceWinsContent ();
      clearSourceWindows ();
      if (layout == SRC_DATA_COMMAND || layout == DISASSEM_DATA_COMMAND)
	{
	  _showData (layout);
	  refreshAll (winList);
	}
      else
	{
	  /* First make the current layout be invisible */
	  m_allBeInvisible ();
	  m_beInvisible (locatorWinInfoPtr ());

	  switch (layout)
	    {
	      /* Now show the new layout */
	    case SRC_COMMAND:
	      _showSourceCommand ();
	      addToSourceWindows (srcWin);
	      break;
	    case DISASSEM_COMMAND:
	      _showDisassemCommand ();
	      addToSourceWindows (disassemWin);
	      break;
	    case SRC_DISASSEM_COMMAND:
	      _showSourceDisassemCommand ();
	      addToSourceWindows (srcWin);
	      addToSourceWindows (disassemWin);
	      break;
	    default:
	      break;
	    }
	}
    }
}


/*
   ** tuiSetLayout()
   **    Function to set the layout to SRC_COMMAND, DISASSEM_COMMAND,
   **    SRC_DISASSEM_COMMAND, SRC_DATA_COMMAND, or DISASSEM_DATA_COMMAND.
   **    If the layout is SRC_DATA_COMMAND, DISASSEM_DATA_COMMAND, or
   **    UNDEFINED_LAYOUT, then the data window is populated according
   **    to regsDisplayType.
 */
TuiStatus
tuiSetLayout (TuiLayoutType layoutType,
              TuiRegisterDisplayType regsDisplayType)
{
  TuiStatus status = TUI_SUCCESS;

  if (layoutType != UNDEFINED_LAYOUT || regsDisplayType != TUI_UNDEFINED_REGS)
    {
      TuiLayoutType curLayout = currentLayout (), newLayout = UNDEFINED_LAYOUT;
      int regsPopulate = FALSE;
      CORE_ADDR addr = _extractDisplayStartAddr ();
      TuiWinInfoPtr newWinWithFocus = (TuiWinInfoPtr) NULL, winWithFocus = tuiWinWithFocus ();
      TuiLayoutDefPtr layoutDef = tuiLayoutDef ();


      if (layoutType == UNDEFINED_LAYOUT &&
	  regsDisplayType != TUI_UNDEFINED_REGS)
	{
	  if (curLayout == SRC_DISASSEM_COMMAND)
	    newLayout = DISASSEM_DATA_COMMAND;
	  else if (curLayout == SRC_COMMAND || curLayout == SRC_DATA_COMMAND)
	    newLayout = SRC_DATA_COMMAND;
	  else if (curLayout == DISASSEM_COMMAND ||
		   curLayout == DISASSEM_DATA_COMMAND)
	    newLayout = DISASSEM_DATA_COMMAND;
	}
      else
	newLayout = layoutType;

      regsPopulate = (newLayout == SRC_DATA_COMMAND ||
		      newLayout == DISASSEM_DATA_COMMAND ||
		      regsDisplayType != TUI_UNDEFINED_REGS);
      if (newLayout != curLayout || regsDisplayType != TUI_UNDEFINED_REGS)
	{
	  if (newLayout != curLayout)
	    {
	      showLayout (newLayout);
	      /*
	         ** Now determine where focus should be
	       */
	      if (winWithFocus != cmdWin)
		{
		  switch (newLayout)
		    {
		    case SRC_COMMAND:
		      tuiSetWinFocusTo (srcWin);
		      layoutDef->displayMode = SRC_WIN;
		      layoutDef->split = FALSE;
		      break;
		    case DISASSEM_COMMAND:
		      /* the previous layout was not showing
		         ** code. this can happen if there is no
		         ** source available:
		         ** 1. if the source file is in another dir OR
		         ** 2. if target was compiled without -g
		         ** We still want to show the assembly though!
		       */
		      addr = tuiGetBeginAsmAddress ();
		      tuiSetWinFocusTo (disassemWin);
		      layoutDef->displayMode = DISASSEM_WIN;
		      layoutDef->split = FALSE;
		      break;
		    case SRC_DISASSEM_COMMAND:
		      /* the previous layout was not showing
		         ** code. this can happen if there is no
		         ** source available:
		         ** 1. if the source file is in another dir OR
		         ** 2. if target was compiled without -g
		         ** We still want to show the assembly though!
		       */
		      addr = tuiGetBeginAsmAddress ();
		      if (winWithFocus == srcWin)
			tuiSetWinFocusTo (srcWin);
		      else
			tuiSetWinFocusTo (disassemWin);
		      layoutDef->split = TRUE;
		      break;
		    case SRC_DATA_COMMAND:
		      if (winWithFocus != dataWin)
			tuiSetWinFocusTo (srcWin);
		      else
			tuiSetWinFocusTo (dataWin);
		      layoutDef->displayMode = SRC_WIN;
		      layoutDef->split = FALSE;
		      break;
		    case DISASSEM_DATA_COMMAND:
		      /* the previous layout was not showing
		         ** code. this can happen if there is no
		         ** source available:
		         ** 1. if the source file is in another dir OR
		         ** 2. if target was compiled without -g
		         ** We still want to show the assembly though!
		       */
		      addr = tuiGetBeginAsmAddress ();
		      if (winWithFocus != dataWin)
			tuiSetWinFocusTo (disassemWin);
		      else
			tuiSetWinFocusTo (dataWin);
		      layoutDef->displayMode = DISASSEM_WIN;
		      layoutDef->split = FALSE;
		      break;
		    default:
		      break;
		    }
		}
	      if (newWinWithFocus != (TuiWinInfoPtr) NULL)
		tuiSetWinFocusTo (newWinWithFocus);
	      /*
	         ** Now update the window content
	       */
	      if (!regsPopulate &&
		  (newLayout == SRC_DATA_COMMAND ||
		   newLayout == DISASSEM_DATA_COMMAND))
		tuiDisplayAllData ();

	      tuiUpdateSourceWindowsWithAddr (addr);
	    }
	  if (regsPopulate)
	    {
	      layoutDef->regsDisplayType =
		(regsDisplayType == TUI_UNDEFINED_REGS ?
		 TUI_GENERAL_REGS : regsDisplayType);
	      tuiShowRegisters (layoutDef->regsDisplayType);
	    }
	}
    }
  else
    status = TUI_FAILURE;

  return status;
}

/*
   ** tuiAddWinToLayout().
   **        Add the specified window to the layout in a logical way.
   **        This means setting up the most logical layout given the
   **        window to be added.
 */
void
tuiAddWinToLayout (TuiWinType type)
{
  TuiLayoutType curLayout = currentLayout ();

  switch (type)
    {
    case SRC_WIN:
      if (curLayout != SRC_COMMAND &&
	  curLayout != SRC_DISASSEM_COMMAND &&
	  curLayout != SRC_DATA_COMMAND)
	{
	  clearSourceWindowsDetail ();
	  if (curLayout == DISASSEM_DATA_COMMAND)
	    showLayout (SRC_DATA_COMMAND);
	  else
	    showLayout (SRC_COMMAND);
	}
      break;
    case DISASSEM_WIN:
      if (curLayout != DISASSEM_COMMAND &&
	  curLayout != SRC_DISASSEM_COMMAND &&
	  curLayout != DISASSEM_DATA_COMMAND)
	{
	  clearSourceWindowsDetail ();
	  if (curLayout == SRC_DATA_COMMAND)
	    showLayout (DISASSEM_DATA_COMMAND);
	  else
	    showLayout (DISASSEM_COMMAND);
	}
      break;
    case DATA_WIN:
      if (curLayout != SRC_DATA_COMMAND &&
	  curLayout != DISASSEM_DATA_COMMAND)
	{
	  if (curLayout == DISASSEM_COMMAND)
	    showLayout (DISASSEM_DATA_COMMAND);
	  else
	    showLayout (SRC_DATA_COMMAND);
	}
      break;
    default:
      break;
    }

  return;
}				/* tuiAddWinToLayout */


/*
   ** tuiDefaultWinHeight().
   **        Answer the height of a window.  If it hasn't been created yet,
   **        answer what the height of a window would be based upon its
   **        type and the layout.
 */
int
tuiDefaultWinHeight (TuiWinType type, TuiLayoutType layout)
{
  int h;

  if (winList[type] != (TuiWinInfoPtr) NULL)
    h = winList[type]->generic.height;
  else
    {
      switch (layout)
	{
	case SRC_COMMAND:
	case DISASSEM_COMMAND:
	  if (m_winPtrIsNull (cmdWin))
	    h = termHeight () / 2;
	  else
	    h = termHeight () - cmdWin->generic.height;
	  break;
	case SRC_DISASSEM_COMMAND:
	case SRC_DATA_COMMAND:
	case DISASSEM_DATA_COMMAND:
	  if (m_winPtrIsNull (cmdWin))
	    h = termHeight () / 3;
	  else
	    h = (termHeight () - cmdWin->generic.height) / 2;
	  break;
	default:
	  h = 0;
	  break;
	}
    }

  return h;
}				/* tuiDefaultWinHeight */


/*
   ** tuiDefaultWinViewportHeight().
   **        Answer the height of a window.  If it hasn't been created yet,
   **        answer what the height of a window would be based upon its
   **        type and the layout.
 */
int
tuiDefaultWinViewportHeight (TuiWinType type, TuiLayoutType layout)
{
  int h;

  h = tuiDefaultWinHeight (type, layout);

  if (winList[type] == cmdWin)
    h -= 1;
  else
    h -= 2;

  return h;
}				/* tuiDefaultWinViewportHeight */


/*
   ** _initialize_tuiLayout().
   **        Function to initialize gdb commands, for tui window layout
   **        manipulation.
 */
void
_initialize_tuiLayout (void)
{
  add_com ("layout", class_tui, _tuiLayout_command,
           "Change the layout of windows.\n\
Usage: layout prev | next | <layout_name> \n\
Layout names are:\n\
   src   : Displays source and command windows.\n\
   asm   : Displays disassembly and command windows.\n\
   split : Displays source, disassembly and command windows.\n\
   regs  : Displays register window. If existing layout\n\
           is source/command or assembly/command, the \n\
           register window is displayed. If the\n\
           source/assembly/command (split) is displayed, \n\
           the register window is displayed with \n\
           the window that has current logical focus.\n");
  if (xdb_commands)
    {
      add_com ("td", class_tui, _tuiToggleLayout_command,
               "Toggle between Source/Command and Disassembly/Command layouts.\n");
      add_com ("ts", class_tui, _tuiToggleSplitLayout_command,
               "Toggle between Source/Command or Disassembly/Command and \n\
Source/Disassembly/Command layouts.\n");
    }
}


/*************************
** STATIC LOCAL FUNCTIONS
**************************/


/*
   ** _tuiSetLayoutTo()
   **    Function to set the layout to SRC, ASM, SPLIT, NEXT, PREV, DATA, REGS,
   **        $REGS, $GREGS, $FREGS, $SREGS.
 */
TuiStatus
tui_set_layout (const char *layoutName)
{
  TuiStatus status = TUI_SUCCESS;

  if (layoutName != (char *) NULL)
    {
      register int i;
      register char *bufPtr;
      TuiLayoutType newLayout = UNDEFINED_LAYOUT;
      TuiRegisterDisplayType dpyType = TUI_UNDEFINED_REGS;
      TuiLayoutType curLayout = currentLayout ();

      bufPtr = (char *) xstrdup (layoutName);
      for (i = 0; (i < strlen (layoutName)); i++)
	bufPtr[i] = toupper (bufPtr[i]);

      /* First check for ambiguous input */
      if (strlen (bufPtr) <= 1 && (*bufPtr == 'S' || *bufPtr == '$'))
	{
	  warning ("Ambiguous command input.\n");
	  status = TUI_FAILURE;
	}
      else
	{
	  if (subset_compare (bufPtr, "SRC"))
	    newLayout = SRC_COMMAND;
	  else if (subset_compare (bufPtr, "ASM"))
	    newLayout = DISASSEM_COMMAND;
	  else if (subset_compare (bufPtr, "SPLIT"))
	    newLayout = SRC_DISASSEM_COMMAND;
	  else if (subset_compare (bufPtr, "REGS") ||
		   subset_compare (bufPtr, TUI_GENERAL_SPECIAL_REGS_NAME) ||
		   subset_compare (bufPtr, TUI_GENERAL_REGS_NAME) ||
		   subset_compare (bufPtr, TUI_FLOAT_REGS_NAME) ||
		   subset_compare (bufPtr, TUI_SPECIAL_REGS_NAME))
	    {
	      if (curLayout == SRC_COMMAND || curLayout == SRC_DATA_COMMAND)
		newLayout = SRC_DATA_COMMAND;
	      else
		newLayout = DISASSEM_DATA_COMMAND;

/* could ifdef out the following code. when compile with -z, there are null 
   pointer references that cause a core dump if 'layout regs' is the first 
   layout command issued by the user. HP has asked us to hook up this code 
   - edie epstein
 */
	      if (subset_compare (bufPtr, TUI_FLOAT_REGS_NAME))
		{
		  if (dataWin->detail.dataDisplayInfo.regsDisplayType !=
		      TUI_SFLOAT_REGS &&
		      dataWin->detail.dataDisplayInfo.regsDisplayType !=
		      TUI_DFLOAT_REGS)
		    dpyType = TUI_SFLOAT_REGS;
		  else
		    dpyType =
		      dataWin->detail.dataDisplayInfo.regsDisplayType;
		}
	      else if (subset_compare (bufPtr,
				      TUI_GENERAL_SPECIAL_REGS_NAME))
		dpyType = TUI_GENERAL_AND_SPECIAL_REGS;
	      else if (subset_compare (bufPtr, TUI_GENERAL_REGS_NAME))
		dpyType = TUI_GENERAL_REGS;
	      else if (subset_compare (bufPtr, TUI_SPECIAL_REGS_NAME))
		dpyType = TUI_SPECIAL_REGS;
	      else if (dataWin)
		{
		  if (dataWin->detail.dataDisplayInfo.regsDisplayType !=
		      TUI_UNDEFINED_REGS)
		    dpyType =
		      dataWin->detail.dataDisplayInfo.regsDisplayType;
		  else
		    dpyType = TUI_GENERAL_REGS;
		}

/* end of potential ifdef 
 */

/* if ifdefed out code above, then assume that the user wishes to display the 
   general purpose registers 
 */

/*              dpyType = TUI_GENERAL_REGS; 
 */
	    }
	  else if (subset_compare (bufPtr, "NEXT"))
	    newLayout = _nextLayout ();
	  else if (subset_compare (bufPtr, "PREV"))
	    newLayout = _prevLayout ();
	  else
	    status = TUI_FAILURE;
	  xfree (bufPtr);

	  tuiSetLayout (newLayout, dpyType);
	}
    }
  else
    status = TUI_FAILURE;

  return status;
}


static CORE_ADDR
_extractDisplayStartAddr (void)
{
  TuiLayoutType curLayout = currentLayout ();
  CORE_ADDR addr;
  CORE_ADDR pc;

  switch (curLayout)
    {
    case SRC_COMMAND:
    case SRC_DATA_COMMAND:
      find_line_pc (current_source_symtab,
		    srcWin->detail.sourceInfo.startLineOrAddr.lineNo,
		    &pc);
      addr = pc;
      break;
    case DISASSEM_COMMAND:
    case SRC_DISASSEM_COMMAND:
    case DISASSEM_DATA_COMMAND:
      addr = disassemWin->detail.sourceInfo.startLineOrAddr.addr;
      break;
    default:
      addr = 0;
      break;
    }

  return addr;
}				/* _extractDisplayStartAddr */


static void
_tuiHandleXDBLayout (TuiLayoutDefPtr layoutDef)
{
  if (layoutDef->split)
    {
      tuiSetLayout (SRC_DISASSEM_COMMAND, TUI_UNDEFINED_REGS);
      tuiSetWinFocusTo (winList[layoutDef->displayMode]);
    }
  else
    {
      if (layoutDef->displayMode == SRC_WIN)
	tuiSetLayout (SRC_COMMAND, TUI_UNDEFINED_REGS);
      else
	tuiSetLayout (DISASSEM_DATA_COMMAND, layoutDef->regsDisplayType);
    }


  return;
}				/* _tuiHandleXDBLayout */


static void
_tuiToggleLayout_command (char *arg, int fromTTY)
{
  TuiLayoutDefPtr layoutDef = tuiLayoutDef ();

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (layoutDef->displayMode == SRC_WIN)
    layoutDef->displayMode = DISASSEM_WIN;
  else
    layoutDef->displayMode = SRC_WIN;

  if (!layoutDef->split)
    _tuiHandleXDBLayout (layoutDef);

}


static void
_tuiToggleSplitLayout_command (char *arg, int fromTTY)
{
  TuiLayoutDefPtr layoutDef = tuiLayoutDef ();

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  layoutDef->split = (!layoutDef->split);
  _tuiHandleXDBLayout (layoutDef);

}


static void
_tuiLayout_command (char *arg, int fromTTY)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();

  /* Switch to the selected layout.  */
  if (tui_set_layout (arg) != TUI_SUCCESS)
    warning ("Invalid layout specified.\n%s", LAYOUT_USAGE);

}

/*
   ** _nextLayout().
   **        Answer the previous layout to cycle to.
 */
static TuiLayoutType
_nextLayout (void)
{
  TuiLayoutType newLayout;

  newLayout = currentLayout ();
  if (newLayout == UNDEFINED_LAYOUT)
    newLayout = SRC_COMMAND;
  else
    {
      newLayout++;
      if (newLayout == UNDEFINED_LAYOUT)
	newLayout = SRC_COMMAND;
    }

  return newLayout;
}				/* _nextLayout */


/*
   ** _prevLayout().
   **        Answer the next layout to cycle to.
 */
static TuiLayoutType
_prevLayout (void)
{
  TuiLayoutType newLayout;

  newLayout = currentLayout ();
  if (newLayout == SRC_COMMAND)
    newLayout = DISASSEM_DATA_COMMAND;
  else
    {
      newLayout--;
      if (newLayout == UNDEFINED_LAYOUT)
	newLayout = DISASSEM_DATA_COMMAND;
    }

  return newLayout;
}				/* _prevLayout */



/*
   ** _makeCommandWindow().
 */
static void
_makeCommandWindow (TuiWinInfoPtr * winInfoPtr, int height, int originY)
{
  _initAndMakeWin ((Opaque *) winInfoPtr,
		   CMD_WIN,
		   height,
		   termWidth (),
		   0,
		   originY,
		   DONT_BOX_WINDOW);

  (*winInfoPtr)->canHighlight = FALSE;

  return;
}				/* _makeCommandWindow */


/*
   ** _makeSourceWindow().
 */
static void
_makeSourceWindow (TuiWinInfoPtr * winInfoPtr, int height, int originY)
{
  _makeSourceOrDisassemWindow (winInfoPtr, SRC_WIN, height, originY);

  return;
}				/* _makeSourceWindow */


/*
   ** _makeDisassemWindow().
 */
static void
_makeDisassemWindow (TuiWinInfoPtr * winInfoPtr, int height, int originY)
{
  _makeSourceOrDisassemWindow (winInfoPtr, DISASSEM_WIN, height, originY);

  return;
}				/* _makeDisassemWindow */


/*
   ** _makeDataWindow().
 */
static void
_makeDataWindow (TuiWinInfoPtr * winInfoPtr, int height, int originY)
{
  _initAndMakeWin ((Opaque *) winInfoPtr,
		   DATA_WIN,
		   height,
		   termWidth (),
		   0,
		   originY,
		   BOX_WINDOW);

  return;
}				/* _makeDataWindow */



/*
   **    _showSourceCommand().
   **        Show the Source/Command layout
 */
static void
_showSourceCommand (void)
{
  _showSourceOrDisassemAndCommand (SRC_COMMAND);

  return;
}				/* _showSourceCommand */


/*
   **    _showDisassemCommand().
   **        Show the Dissassem/Command layout
 */
static void
_showDisassemCommand (void)
{
  _showSourceOrDisassemAndCommand (DISASSEM_COMMAND);

  return;
}				/* _showDisassemCommand */


/*
   **    _showSourceDisassemCommand().
   **        Show the Source/Disassem/Command layout
 */
static void
_showSourceDisassemCommand (void)
{
  if (currentLayout () != SRC_DISASSEM_COMMAND)
    {
      int cmdHeight, srcHeight, asmHeight;

      if (m_winPtrNotNull (cmdWin))
	cmdHeight = cmdWin->generic.height;
      else
	cmdHeight = termHeight () / 3;

      srcHeight = (termHeight () - cmdHeight) / 2;
      asmHeight = termHeight () - (srcHeight + cmdHeight);

      if (m_winPtrIsNull (srcWin))
	_makeSourceWindow (&srcWin, srcHeight, 0);
      else
	{
	  _initGenWinInfo (&srcWin->generic,
			   srcWin->generic.type,
			   srcHeight,
			   srcWin->generic.width,
			   srcWin->detail.sourceInfo.executionInfo->width,
			   0);
	  srcWin->canHighlight = TRUE;
	  _initGenWinInfo (srcWin->detail.sourceInfo.executionInfo,
			   EXEC_INFO_WIN,
			   srcHeight,
			   3,
			   0,
			   0);
	  m_beVisible (srcWin);
	  m_beVisible (srcWin->detail.sourceInfo.executionInfo);
	  srcWin->detail.sourceInfo.hasLocator = FALSE;;
	}
      if (m_winPtrNotNull (srcWin))
	{
	  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();

	  tuiShowSourceContent (srcWin);
	  if (m_winPtrIsNull (disassemWin))
	    {
	      _makeDisassemWindow (&disassemWin, asmHeight, srcHeight - 1);
	      _initAndMakeWin ((Opaque *) & locator,
			       LOCATOR_WIN,
			       2 /* 1 */ ,
			       termWidth (),
			       0,
			       (srcHeight + asmHeight) - 1,
			       DONT_BOX_WINDOW);
	    }
	  else
	    {
	      _initGenWinInfo (locator,
			       LOCATOR_WIN,
			       2 /* 1 */ ,
			       termWidth (),
			       0,
			       (srcHeight + asmHeight) - 1);
	      disassemWin->detail.sourceInfo.hasLocator = TRUE;
	      _initGenWinInfo (
				&disassemWin->generic,
				disassemWin->generic.type,
				asmHeight,
				disassemWin->generic.width,
			disassemWin->detail.sourceInfo.executionInfo->width,
				srcHeight - 1);
	      _initGenWinInfo (disassemWin->detail.sourceInfo.executionInfo,
			       EXEC_INFO_WIN,
			       asmHeight,
			       3,
			       0,
			       srcHeight - 1);
	      disassemWin->canHighlight = TRUE;
	      m_beVisible (disassemWin);
	      m_beVisible (disassemWin->detail.sourceInfo.executionInfo);
	    }
	  if (m_winPtrNotNull (disassemWin))
	    {
	      srcWin->detail.sourceInfo.hasLocator = FALSE;
	      disassemWin->detail.sourceInfo.hasLocator = TRUE;
	      m_beVisible (locator);
	      tuiShowLocatorContent ();
	      tuiShowSourceContent (disassemWin);

	      if (m_winPtrIsNull (cmdWin))
		_makeCommandWindow (&cmdWin,
				    cmdHeight,
				    termHeight () - cmdHeight);
	      else
		{
		  _initGenWinInfo (&cmdWin->generic,
				   cmdWin->generic.type,
				   cmdWin->generic.height,
				   cmdWin->generic.width,
				   0,
				   cmdWin->generic.origin.y);
		  cmdWin->canHighlight = FALSE;
		  m_beVisible (cmdWin);
		}
	      if (m_winPtrNotNull (cmdWin))
		tuiRefreshWin (&cmdWin->generic);
	    }
	}
      setCurrentLayoutTo (SRC_DISASSEM_COMMAND);
    }

  return;
}				/* _showSourceDisassemCommand */


/*
   **    _showData().
   **        Show the Source/Data/Command or the Dissassembly/Data/Command layout
 */
static void
_showData (TuiLayoutType newLayout)
{
  int totalHeight = (termHeight () - cmdWin->generic.height);
  int srcHeight, dataHeight;
  TuiWinType winType;
  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();


  dataHeight = totalHeight / 2;
  srcHeight = totalHeight - dataHeight;
  m_allBeInvisible ();
  m_beInvisible (locator);
  _makeDataWindow (&dataWin, dataHeight, 0);
  dataWin->canHighlight = TRUE;
  if (newLayout == SRC_DATA_COMMAND)
    winType = SRC_WIN;
  else
    winType = DISASSEM_WIN;
  if (m_winPtrIsNull (winList[winType]))
    {
      if (winType == SRC_WIN)
	_makeSourceWindow (&winList[winType], srcHeight, dataHeight - 1);
      else
	_makeDisassemWindow (&winList[winType], srcHeight, dataHeight - 1);
      _initAndMakeWin ((Opaque *) & locator,
		       LOCATOR_WIN,
		       2 /* 1 */ ,
		       termWidth (),
		       0,
		       totalHeight - 1,
		       DONT_BOX_WINDOW);
    }
  else
    {
      _initGenWinInfo (&winList[winType]->generic,
		       winList[winType]->generic.type,
		       srcHeight,
		       winList[winType]->generic.width,
		   winList[winType]->detail.sourceInfo.executionInfo->width,
		       dataHeight - 1);
      _initGenWinInfo (winList[winType]->detail.sourceInfo.executionInfo,
		       EXEC_INFO_WIN,
		       srcHeight,
		       3,
		       0,
		       dataHeight - 1);
      m_beVisible (winList[winType]);
      m_beVisible (winList[winType]->detail.sourceInfo.executionInfo);
      _initGenWinInfo (locator,
		       LOCATOR_WIN,
		       2 /* 1 */ ,
		       termWidth (),
		       0,
		       totalHeight - 1);
    }
  winList[winType]->detail.sourceInfo.hasLocator = TRUE;
  m_beVisible (locator);
  tuiShowLocatorContent ();
  addToSourceWindows (winList[winType]);
  setCurrentLayoutTo (newLayout);

  return;
}				/* _showData */

/*
   ** _initGenWinInfo().
 */
static void
_initGenWinInfo (TuiGenWinInfoPtr winInfo, TuiWinType type,
                 int height, int width, int originX, int originY)
{
  int h = height;

  winInfo->type = type;
  winInfo->width = width;
  winInfo->height = h;
  if (h > 1)
    {
      winInfo->viewportHeight = h - 1;
      if (winInfo->type != CMD_WIN)
	winInfo->viewportHeight--;
    }
  else
    winInfo->viewportHeight = 1;
  winInfo->origin.x = originX;
  winInfo->origin.y = originY;

  return;
}				/* _initGenWinInfo */

/*
   ** _initAndMakeWin().
 */
static void
_initAndMakeWin (Opaque * winInfoPtr, TuiWinType winType,
                 int height, int width, int originX, int originY, int boxIt)
{
  Opaque opaqueWinInfo = *winInfoPtr;
  TuiGenWinInfoPtr generic;

  if (opaqueWinInfo == (Opaque) NULL)
    {
      if (m_winIsAuxillary (winType))
	opaqueWinInfo = (Opaque) allocGenericWinInfo ();
      else
	opaqueWinInfo = (Opaque) allocWinInfo (winType);
    }
  if (m_winIsAuxillary (winType))
    generic = (TuiGenWinInfoPtr) opaqueWinInfo;
  else
    generic = &((TuiWinInfoPtr) opaqueWinInfo)->generic;

  if (opaqueWinInfo != (Opaque) NULL)
    {
      _initGenWinInfo (generic, winType, height, width, originX, originY);
      if (!m_winIsAuxillary (winType))
	{
	  if (generic->type == CMD_WIN)
	    ((TuiWinInfoPtr) opaqueWinInfo)->canHighlight = FALSE;
	  else
	    ((TuiWinInfoPtr) opaqueWinInfo)->canHighlight = TRUE;
	}
      makeWindow (generic, boxIt);
    }
  *winInfoPtr = opaqueWinInfo;
}


/*
   ** _makeSourceOrDisassemWindow().
 */
static void
_makeSourceOrDisassemWindow (TuiWinInfoPtr * winInfoPtr, TuiWinType type,
                             int height, int originY)
{
  TuiGenWinInfoPtr executionInfo = (TuiGenWinInfoPtr) NULL;

  /*
     ** Create the exeuction info window.
   */
  if (type == SRC_WIN)
    executionInfo = sourceExecInfoWinPtr ();
  else
    executionInfo = disassemExecInfoWinPtr ();
  _initAndMakeWin ((Opaque *) & executionInfo,
		   EXEC_INFO_WIN,
		   height,
		   3,
		   0,
		   originY,
		   DONT_BOX_WINDOW);
  /*
     ** Now create the source window.
   */
  _initAndMakeWin ((Opaque *) winInfoPtr,
		   type,
		   height,
		   termWidth () - executionInfo->width,
		   executionInfo->width,
		   originY,
		   BOX_WINDOW);

  (*winInfoPtr)->detail.sourceInfo.executionInfo = executionInfo;

  return;
}				/* _makeSourceOrDisassemWindow */


/*
   **    _showSourceOrDisassemAndCommand().
   **        Show the Source/Command or the Disassem layout
 */
static void
_showSourceOrDisassemAndCommand (TuiLayoutType layoutType)
{
  if (currentLayout () != layoutType)
    {
      TuiWinInfoPtr *winInfoPtr;
      int srcHeight, cmdHeight;
      TuiGenWinInfoPtr locator = locatorWinInfoPtr ();

      if (m_winPtrNotNull (cmdWin))
	cmdHeight = cmdWin->generic.height;
      else
	cmdHeight = termHeight () / 3;
      srcHeight = termHeight () - cmdHeight;


      if (layoutType == SRC_COMMAND)
	winInfoPtr = &srcWin;
      else
	winInfoPtr = &disassemWin;

      if (m_winPtrIsNull (*winInfoPtr))
	{
	  if (layoutType == SRC_COMMAND)
	    _makeSourceWindow (winInfoPtr, srcHeight - 1, 0);
	  else
	    _makeDisassemWindow (winInfoPtr, srcHeight - 1, 0);
	  _initAndMakeWin ((Opaque *) & locator,
			   LOCATOR_WIN,
			   2 /* 1 */ ,
			   termWidth (),
			   0,
			   srcHeight - 1,
			   DONT_BOX_WINDOW);
	}
      else
	{
	  _initGenWinInfo (locator,
			   LOCATOR_WIN,
			   2 /* 1 */ ,
			   termWidth (),
			   0,
			   srcHeight - 1);
	  (*winInfoPtr)->detail.sourceInfo.hasLocator = TRUE;
	  _initGenWinInfo (
			    &(*winInfoPtr)->generic,
			    (*winInfoPtr)->generic.type,
			    srcHeight - 1,
			    (*winInfoPtr)->generic.width,
		      (*winInfoPtr)->detail.sourceInfo.executionInfo->width,
			    0);
	  _initGenWinInfo ((*winInfoPtr)->detail.sourceInfo.executionInfo,
			   EXEC_INFO_WIN,
			   srcHeight - 1,
			   3,
			   0,
			   0);
	  (*winInfoPtr)->canHighlight = TRUE;
	  m_beVisible (*winInfoPtr);
	  m_beVisible ((*winInfoPtr)->detail.sourceInfo.executionInfo);
	}
      if (m_winPtrNotNull (*winInfoPtr))
	{
	  (*winInfoPtr)->detail.sourceInfo.hasLocator = TRUE;
	  m_beVisible (locator);
	  tuiShowLocatorContent ();
	  tuiShowSourceContent (*winInfoPtr);

	  if (m_winPtrIsNull (cmdWin))
	    {
	      _makeCommandWindow (&cmdWin, cmdHeight, srcHeight);
	      tuiRefreshWin (&cmdWin->generic);
	    }
	  else
	    {
	      _initGenWinInfo (&cmdWin->generic,
			       cmdWin->generic.type,
			       cmdWin->generic.height,
			       cmdWin->generic.width,
			       cmdWin->generic.origin.x,
			       cmdWin->generic.origin.y);
	      cmdWin->canHighlight = FALSE;
	      m_beVisible (cmdWin);
	    }
	}
      setCurrentLayoutTo (layoutType);
    }

  return;
}				/* _showSourceOrDisassemAndCommand */
