/* TUI window generic functions.

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

/* This module contains procedures for handling tui window functions
   like resize, scrolling, scrolling, changing focus, etc.

   Author: Susan B. Macchia  */

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

#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "command.h"
#include "symtab.h"
#include "breakpoint.h"
#include "frame.h"
#include "cli/cli-cmds.h"
#include "top.h"

#include "tui.h"
#include "tuiData.h"
#include "tuiGeneralWin.h"
#include "tuiStack.h"
#include "tuiRegs.h"
#include "tuiDisassem.h"
#include "tuiSource.h"
#include "tuiSourceWin.h"
#include "tuiDataWin.h"

/*******************************
** Static Local Decls
********************************/
static void _makeVisibleWithNewHeight (TuiWinInfoPtr);
static void _makeInvisibleAndSetNewHeight (TuiWinInfoPtr, int);
static TuiStatus _tuiAdjustWinHeights (TuiWinInfoPtr, int);
static int _newHeightOk (TuiWinInfoPtr, int);
static void _tuiSetTabWidth_command (char *, int);
static void _tuiRefreshAll_command (char *, int);
static void _tuiSetWinHeight_command (char *, int);
static void _tuiXDBsetWinHeight_command (char *, int);
static void _tuiAllWindowsInfo (char *, int);
static void _tuiSetFocus_command (char *, int);
static void _tuiScrollForward_command (char *, int);
static void _tuiScrollBackward_command (char *, int);
static void _tuiScrollLeft_command (char *, int);
static void _tuiScrollRight_command (char *, int);
static void _parseScrollingArgs (char *, TuiWinInfoPtr *, int *);


/***************************************
** DEFINITIONS
***************************************/
#define WIN_HEIGHT_USAGE      "Usage: winheight <win_name> [+ | -] <#lines>\n"
#define XDBWIN_HEIGHT_USAGE   "Usage: w <#lines>\n"
#define FOCUS_USAGE           "Usage: focus {<win> | next | prev}\n"

/***************************************
** PUBLIC FUNCTIONS
***************************************/

#ifndef ACS_LRCORNER
#  define ACS_LRCORNER '+'
#endif
#ifndef ACS_LLCORNER
#  define ACS_LLCORNER '+'
#endif
#ifndef ACS_ULCORNER
#  define ACS_ULCORNER '+'
#endif
#ifndef ACS_URCORNER
#  define ACS_URCORNER '+'
#endif
#ifndef ACS_HLINE
#  define ACS_HLINE '-'
#endif
#ifndef ACS_VLINE
#  define ACS_VLINE '|'
#endif

/* Possible values for tui-border-kind variable.  */
static const char *tui_border_kind_enums[] = {
  "space",
  "ascii",
  "acs",
  NULL
};

/* Possible values for tui-border-mode and tui-active-border-mode.  */
static const char *tui_border_mode_enums[] = {
  "normal",
  "standout",
  "reverse",
  "half",
  "half-standout",
  "bold",
  "bold-standout",
  NULL
};

struct tui_translate
{
  const char *name;
  int value;
};

/* Translation table for border-mode variables.
   The list of values must be terminated by a NULL.
   After the NULL value, an entry defines the default.  */
struct tui_translate tui_border_mode_translate[] = {
  { "normal",		A_NORMAL },
  { "standout",		A_STANDOUT },
  { "reverse",		A_REVERSE },
  { "half",		A_DIM },
  { "half-standout",	A_DIM | A_STANDOUT },
  { "bold",		A_BOLD },
  { "bold-standout",	A_BOLD | A_STANDOUT },
  { 0, 0 },
  { "normal",		A_NORMAL }
};

/* Translation tables for border-kind, one for each border
   character (see wborder, border curses operations).
   -1 is used to indicate the ACS because ACS characters
   are determined at run time by curses (depends on terminal).  */
struct tui_translate tui_border_kind_translate_vline[] = {
  { "space",    ' ' },
  { "ascii",    '|' },
  { "acs",      -1 },
  { 0, 0 },
  { "ascii",    '|' }
};

struct tui_translate tui_border_kind_translate_hline[] = {
  { "space",    ' ' },
  { "ascii",    '-' },
  { "acs",      -1 },
  { 0, 0 },
  { "ascii",    '-' }
};

struct tui_translate tui_border_kind_translate_ulcorner[] = {
  { "space",    ' ' },
  { "ascii",    '+' },
  { "acs",      -1 },
  { 0, 0 },
  { "ascii",    '+' }
};

struct tui_translate tui_border_kind_translate_urcorner[] = {
  { "space",    ' ' },
  { "ascii",    '+' },
  { "acs",      -1 },
  { 0, 0 },
  { "ascii",    '+' }
};

struct tui_translate tui_border_kind_translate_llcorner[] = {
  { "space",    ' ' },
  { "ascii",    '+' },
  { "acs",      -1 },
  { 0, 0 },
  { "ascii",    '+' }
};

struct tui_translate tui_border_kind_translate_lrcorner[] = {
  { "space",    ' ' },
  { "ascii",    '+' },
  { "acs",      -1 },
  { 0, 0 },
  { "ascii",    '+' }
};


/* Tui configuration variables controlled with set/show command.  */
const char *tui_active_border_mode = "bold-standout";
const char *tui_border_mode = "normal";
const char *tui_border_kind = "acs";

/* Tui internal configuration variables.  These variables are
   updated by tui_update_variables to reflect the tui configuration
   variables.  */
chtype tui_border_vline;
chtype tui_border_hline;
chtype tui_border_ulcorner;
chtype tui_border_urcorner;
chtype tui_border_llcorner;
chtype tui_border_lrcorner;

int tui_border_attrs;
int tui_active_border_attrs;

/* Identify the item in the translation table.
   When the item is not recognized, use the default entry.  */
static struct tui_translate *
translate (const char *name, struct tui_translate *table)
{
  while (table->name)
    {
      if (name && strcmp (table->name, name) == 0)
        return table;
      table++;
    }

  /* Not found, return default entry.  */
  table++;
  return table;
}

/* Update the tui internal configuration according to gdb settings.
   Returns 1 if the configuration has changed and the screen should
   be redrawn.  */
int
tui_update_variables ()
{
  int need_redraw = 0;
  struct tui_translate *entry;

  entry = translate (tui_border_mode, tui_border_mode_translate);
  if (tui_border_attrs != entry->value)
    {
      tui_border_attrs = entry->value;
      need_redraw = 1;
    }
  entry = translate (tui_active_border_mode, tui_border_mode_translate);
  if (tui_active_border_attrs != entry->value)
    {
      tui_active_border_attrs = entry->value;
      need_redraw = 1;
    }

  /* If one corner changes, all characters are changed.
     Only check the first one.  The ACS characters are determined at
     run time by curses terminal management.  */
  entry = translate (tui_border_kind, tui_border_kind_translate_lrcorner);
  if (tui_border_lrcorner != (chtype) entry->value)
    {
      tui_border_lrcorner = (entry->value < 0) ? ACS_LRCORNER : entry->value;
      need_redraw = 1;
    }
  entry = translate (tui_border_kind, tui_border_kind_translate_llcorner);
  tui_border_llcorner = (entry->value < 0) ? ACS_LLCORNER : entry->value;

  entry = translate (tui_border_kind, tui_border_kind_translate_ulcorner);
  tui_border_ulcorner = (entry->value < 0) ? ACS_ULCORNER : entry->value;

  entry = translate (tui_border_kind, tui_border_kind_translate_urcorner);
  tui_border_urcorner = (entry->value < 0) ? ACS_URCORNER : entry->value;

  entry = translate (tui_border_kind, tui_border_kind_translate_hline);
  tui_border_hline = (entry->value < 0) ? ACS_HLINE : entry->value;

  entry = translate (tui_border_kind, tui_border_kind_translate_vline);
  tui_border_vline = (entry->value < 0) ? ACS_VLINE : entry->value;

  return need_redraw;
}

static void
set_tui_cmd (char *args, int from_tty)
{
}

static void
show_tui_cmd (char *args, int from_tty)
{
}

/*
   ** _initialize_tuiWin().
   **        Function to initialize gdb commands, for tui window manipulation.
 */
void
_initialize_tuiWin (void)
{
  struct cmd_list_element *c;
  static struct cmd_list_element *tui_setlist;
  static struct cmd_list_element *tui_showlist;

  /* Define the classes of commands.
     They will appear in the help list in the reverse of this order.  */
  add_cmd ("tui", class_tui, NULL,
	   "Text User Interface commands.",
	   &cmdlist);

  add_prefix_cmd ("tui", class_tui, set_tui_cmd,
                  "TUI configuration variables",
		  &tui_setlist, "set tui ",
		  0/*allow-unknown*/, &setlist);
  add_prefix_cmd ("tui", class_tui, show_tui_cmd,
                  "TUI configuration variables",
		  &tui_showlist, "show tui ",
		  0/*allow-unknown*/, &showlist);

  add_com ("refresh", class_tui, _tuiRefreshAll_command,
           "Refresh the terminal display.\n");
  if (xdb_commands)
    add_com_alias ("U", "refresh", class_tui, 0);
  add_com ("tabset", class_tui, _tuiSetTabWidth_command,
           "Set the width (in characters) of tab stops.\n\
Usage: tabset <n>\n");
  add_com ("winheight", class_tui, _tuiSetWinHeight_command,
           "Set the height of a specified window.\n\
Usage: winheight <win_name> [+ | -] <#lines>\n\
Window names are:\n\
src  : the source window\n\
cmd  : the command window\n\
asm  : the disassembly window\n\
regs : the register display\n");
  add_com_alias ("wh", "winheight", class_tui, 0);
  add_info ("win", _tuiAllWindowsInfo,
            "List of all displayed windows.\n");
  add_com ("focus", class_tui, _tuiSetFocus_command,
           "Set focus to named window or next/prev window.\n\
Usage: focus {<win> | next | prev}\n\
Valid Window names are:\n\
src  : the source window\n\
asm  : the disassembly window\n\
regs : the register display\n\
cmd  : the command window\n");
  add_com_alias ("fs", "focus", class_tui, 0);
  add_com ("+", class_tui, _tuiScrollForward_command,
           "Scroll window forward.\nUsage: + [win] [n]\n");
  add_com ("-", class_tui, _tuiScrollBackward_command,
           "Scroll window backward.\nUsage: - [win] [n]\n");
  add_com ("<", class_tui, _tuiScrollLeft_command,
           "Scroll window forward.\nUsage: < [win] [n]\n");
  add_com (">", class_tui, _tuiScrollRight_command,
           "Scroll window backward.\nUsage: > [win] [n]\n");
  if (xdb_commands)
    add_com ("w", class_xdb, _tuiXDBsetWinHeight_command,
             "XDB compatibility command for setting the height of a command window.\n\
Usage: w <#lines>\n");

  /* Define the tui control variables.  */
  c = add_set_enum_cmd
    ("border-kind", no_class,
     tui_border_kind_enums, &tui_border_kind,
     "Set the kind of border for TUI windows.\n"
     "This variable controls the border of TUI windows:\n"
     "space           use a white space\n"
     "ascii           use ascii characters + - | for the border\n"
     "acs             use the Alternate Character Set\n",
     &tui_setlist);
  add_show_from_set (c, &tui_showlist);

  c = add_set_enum_cmd
    ("border-mode", no_class,
     tui_border_mode_enums, &tui_border_mode,
     "Set the attribute mode to use for the TUI window borders.\n"
     "This variable controls the attributes to use for the window borders:\n"
     "normal          normal display\n"
     "standout        use highlight mode of terminal\n"
     "reverse         use reverse video mode\n"
     "half            use half bright\n"
     "half-standout   use half bright and standout mode\n"
     "bold            use extra bright or bold\n"
     "bold-standout   use extra bright or bold with standout mode\n",
     &tui_setlist);
  add_show_from_set (c, &tui_showlist);

  c = add_set_enum_cmd
    ("active-border-mode", no_class,
     tui_border_mode_enums, &tui_active_border_mode,
     "Set the attribute mode to use for the active TUI window border.\n"
     "This variable controls the attributes to use for the active window border:\n"
     "normal          normal display\n"
     "standout        use highlight mode of terminal\n"
     "reverse         use reverse video mode\n"
     "half            use half bright\n"
     "half-standout   use half bright and standout mode\n"
     "bold            use extra bright or bold\n"
     "bold-standout   use extra bright or bold with standout mode\n",
     &tui_setlist);
  add_show_from_set (c, &tui_showlist);
}

/* Update gdb's knowledge of the terminal size.  */
void
tui_update_gdb_sizes ()
{
  char cmd[50];
  extern int screenheight, screenwidth;		/* in readline */

  /* Set to TUI command window dimension or use readline values.  */
  sprintf (cmd, "set width %d",
           tui_active ? cmdWin->generic.width : screenwidth);
  execute_command (cmd, 0);
  sprintf (cmd, "set height %d",
           tui_active ? cmdWin->generic.height : screenheight);
  execute_command (cmd, 0);
}


/*
   ** tuiSetWinFocusTo
   **        Set the logical focus to winInfo
 */
void
tuiSetWinFocusTo (TuiWinInfoPtr winInfo)
{
  if (m_winPtrNotNull (winInfo))
    {
      TuiWinInfoPtr winWithFocus = tuiWinWithFocus ();

      if (m_winPtrNotNull (winWithFocus) &&
	  winWithFocus->generic.type != CMD_WIN)
	unhighlightWin (winWithFocus);
      tuiSetWinWithFocus (winInfo);
      if (winInfo->generic.type != CMD_WIN)
	highlightWin (winInfo);
    }

  return;
}				/* tuiSetWinFocusTo */


/*
   ** tuiScrollForward().
 */
void
tuiScrollForward (TuiWinInfoPtr winToScroll, int numToScroll)
{
  if (winToScroll != cmdWin)
    {
      int _numToScroll = numToScroll;

      if (numToScroll == 0)
	_numToScroll = winToScroll->generic.height - 3;
      /*
         ** If we are scrolling the source or disassembly window, do a
         ** "psuedo" scroll since not all of the source is in memory,
         ** only what is in the viewport.  If winToScroll is the
         ** command window do nothing since the term should handle it.
       */
      if (winToScroll == srcWin)
	tuiVerticalSourceScroll (FORWARD_SCROLL, _numToScroll);
      else if (winToScroll == disassemWin)
	tuiVerticalDisassemScroll (FORWARD_SCROLL, _numToScroll);
      else if (winToScroll == dataWin)
	tuiVerticalDataScroll (FORWARD_SCROLL, _numToScroll);
    }

  return;
}				/* tuiScrollForward */


/*
   ** tuiScrollBackward().
 */
void
tuiScrollBackward (TuiWinInfoPtr winToScroll, int numToScroll)
{
  if (winToScroll != cmdWin)
    {
      int _numToScroll = numToScroll;

      if (numToScroll == 0)
	_numToScroll = winToScroll->generic.height - 3;
      /*
         ** If we are scrolling the source or disassembly window, do a
         ** "psuedo" scroll since not all of the source is in memory,
         ** only what is in the viewport.  If winToScroll is the
         ** command window do nothing since the term should handle it.
       */
      if (winToScroll == srcWin)
	tuiVerticalSourceScroll (BACKWARD_SCROLL, _numToScroll);
      else if (winToScroll == disassemWin)
	tuiVerticalDisassemScroll (BACKWARD_SCROLL, _numToScroll);
      else if (winToScroll == dataWin)
	tuiVerticalDataScroll (BACKWARD_SCROLL, _numToScroll);
    }
  return;
}				/* tuiScrollBackward */


/*
   ** tuiScrollLeft().
 */
void
tuiScrollLeft (TuiWinInfoPtr winToScroll, int numToScroll)
{
  if (winToScroll != cmdWin)
    {
      int _numToScroll = numToScroll;

      if (_numToScroll == 0)
	_numToScroll = 1;
      /*
         ** If we are scrolling the source or disassembly window, do a
         ** "psuedo" scroll since not all of the source is in memory,
         ** only what is in the viewport. If winToScroll is the
         ** command window do nothing since the term should handle it.
       */
      if (winToScroll == srcWin || winToScroll == disassemWin)
	tuiHorizontalSourceScroll (winToScroll, LEFT_SCROLL, _numToScroll);
    }
  return;
}				/* tuiScrollLeft */


/*
   ** tuiScrollRight().
 */
void
tuiScrollRight (TuiWinInfoPtr winToScroll, int numToScroll)
{
  if (winToScroll != cmdWin)
    {
      int _numToScroll = numToScroll;

      if (_numToScroll == 0)
	_numToScroll = 1;
      /*
         ** If we are scrolling the source or disassembly window, do a
         ** "psuedo" scroll since not all of the source is in memory,
         ** only what is in the viewport. If winToScroll is the
         ** command window do nothing since the term should handle it.
       */
      if (winToScroll == srcWin || winToScroll == disassemWin)
	tuiHorizontalSourceScroll (winToScroll, RIGHT_SCROLL, _numToScroll);
    }
  return;
}				/* tuiScrollRight */


/*
   ** tui_scroll().
   **    Scroll a window.  Arguments are passed through a va_list.
 */
void
tui_scroll (TuiScrollDirection direction,
	    TuiWinInfoPtr winToScroll,
	    int numToScroll)
{
  switch (direction)
    {
    case FORWARD_SCROLL:
      tuiScrollForward (winToScroll, numToScroll);
      break;
    case BACKWARD_SCROLL:
      tuiScrollBackward (winToScroll, numToScroll);
      break;
    case LEFT_SCROLL:
      tuiScrollLeft (winToScroll, numToScroll);
      break;
    case RIGHT_SCROLL:
      tuiScrollRight (winToScroll, numToScroll);
      break;
    default:
      break;
    }
}


/*
   ** tuiRefreshAll().
 */
void
tuiRefreshAll (void)
{
  TuiWinType type;

  clearok (curscr, TRUE);
  refreshAll (winList);
  for (type = SRC_WIN; type < MAX_MAJOR_WINDOWS; type++)
    {
      if (winList[type] && winList[type]->generic.isVisible)
	{
	  switch (type)
	    {
	    case SRC_WIN:
	    case DISASSEM_WIN:
	      tuiShowSourceContent (winList[type]);
	      checkAndDisplayHighlightIfNeeded (winList[type]);
	      tuiEraseExecInfoContent (winList[type]);
	      tuiUpdateExecInfo (winList[type]);
	      break;
	    case DATA_WIN:
	      tuiRefreshDataWin ();
	      break;
	    default:
	      break;
	    }
	}
    }
  tuiShowLocatorContent ();
}


/*
   ** tuiResizeAll().
   **      Resize all the windows based on the the terminal size.  This
   **      function gets called from within the readline sinwinch handler.
 */
void
tuiResizeAll (void)
{
  int heightDiff, widthDiff;
  extern int screenheight, screenwidth;		/* in readline */

  widthDiff = screenwidth - termWidth ();
  heightDiff = screenheight - termHeight ();
  if (heightDiff || widthDiff)
    {
      TuiLayoutType curLayout = currentLayout ();
      TuiWinInfoPtr winWithFocus = tuiWinWithFocus ();
      TuiWinInfoPtr firstWin, secondWin;
      TuiGenWinInfoPtr locator = locatorWinInfoPtr ();
      TuiWinType winType;
      int newHeight, splitDiff, cmdSplitDiff, numWinsDisplayed = 2;

      /* turn keypad off while we resize */
      if (winWithFocus != cmdWin)
	keypad (cmdWin->generic.handle, FALSE);
      tui_update_gdb_sizes ();
      setTermHeightTo (screenheight);
      setTermWidthTo (screenwidth);
      if (curLayout == SRC_DISASSEM_COMMAND ||
	curLayout == SRC_DATA_COMMAND || curLayout == DISASSEM_DATA_COMMAND)
	numWinsDisplayed++;
      splitDiff = heightDiff / numWinsDisplayed;
      cmdSplitDiff = splitDiff;
      if (heightDiff % numWinsDisplayed)
	{
	  if (heightDiff < 0)
	    cmdSplitDiff--;
	  else
	    cmdSplitDiff++;
	}
      /* now adjust each window */
      clear ();
      refresh ();
      switch (curLayout)
	{
	case SRC_COMMAND:
	case DISASSEM_COMMAND:
	  firstWin = (TuiWinInfoPtr) (sourceWindows ())->list[0];
	  firstWin->generic.width += widthDiff;
	  locator->width += widthDiff;
	  /* check for invalid heights */
	  if (heightDiff == 0)
	    newHeight = firstWin->generic.height;
	  else if ((firstWin->generic.height + splitDiff) >=
		   (screenheight - MIN_CMD_WIN_HEIGHT - 1))
	    newHeight = screenheight - MIN_CMD_WIN_HEIGHT - 1;
	  else if ((firstWin->generic.height + splitDiff) <= 0)
	    newHeight = MIN_WIN_HEIGHT;
	  else
	    newHeight = firstWin->generic.height + splitDiff;

	  _makeInvisibleAndSetNewHeight (firstWin, newHeight);
	  cmdWin->generic.origin.y = locator->origin.y + 1;
	  cmdWin->generic.width += widthDiff;
	  newHeight = screenheight - cmdWin->generic.origin.y;
	  _makeInvisibleAndSetNewHeight (cmdWin, newHeight);
	  _makeVisibleWithNewHeight (firstWin);
	  _makeVisibleWithNewHeight (cmdWin);
	  if (firstWin->generic.contentSize <= 0)
	    tuiEraseSourceContent (firstWin, EMPTY_SOURCE_PROMPT);
	  break;
	default:
	  if (curLayout == SRC_DISASSEM_COMMAND)
	    {
	      firstWin = srcWin;
	      firstWin->generic.width += widthDiff;
	      secondWin = disassemWin;
	      secondWin->generic.width += widthDiff;
	    }
	  else
	    {
	      firstWin = dataWin;
	      firstWin->generic.width += widthDiff;
	      secondWin = (TuiWinInfoPtr) (sourceWindows ())->list[0];
	      secondWin->generic.width += widthDiff;
	    }
	  /* Change the first window's height/width */
	  /* check for invalid heights */
	  if (heightDiff == 0)
	    newHeight = firstWin->generic.height;
	  else if ((firstWin->generic.height +
		    secondWin->generic.height + (splitDiff * 2)) >=
		   (screenheight - MIN_CMD_WIN_HEIGHT - 1))
	    newHeight = (screenheight - MIN_CMD_WIN_HEIGHT - 1) / 2;
	  else if ((firstWin->generic.height + splitDiff) <= 0)
	    newHeight = MIN_WIN_HEIGHT;
	  else
	    newHeight = firstWin->generic.height + splitDiff;
	  _makeInvisibleAndSetNewHeight (firstWin, newHeight);

	  if (firstWin == dataWin && widthDiff != 0)
	    firstWin->detail.dataDisplayInfo.regsColumnCount =
	      tuiCalculateRegsColumnCount (
			  firstWin->detail.dataDisplayInfo.regsDisplayType);
	  locator->width += widthDiff;

	  /* Change the second window's height/width */
	  /* check for invalid heights */
	  if (heightDiff == 0)
	    newHeight = secondWin->generic.height;
	  else if ((firstWin->generic.height +
		    secondWin->generic.height + (splitDiff * 2)) >=
		   (screenheight - MIN_CMD_WIN_HEIGHT - 1))
	    {
	      newHeight = screenheight - MIN_CMD_WIN_HEIGHT - 1;
	      if (newHeight % 2)
		newHeight = (newHeight / 2) + 1;
	      else
		newHeight /= 2;
	    }
	  else if ((secondWin->generic.height + splitDiff) <= 0)
	    newHeight = MIN_WIN_HEIGHT;
	  else
	    newHeight = secondWin->generic.height + splitDiff;
	  secondWin->generic.origin.y = firstWin->generic.height - 1;
	  _makeInvisibleAndSetNewHeight (secondWin, newHeight);

	  /* Change the command window's height/width */
	  cmdWin->generic.origin.y = locator->origin.y + 1;
	  _makeInvisibleAndSetNewHeight (
			     cmdWin, cmdWin->generic.height + cmdSplitDiff);
	  _makeVisibleWithNewHeight (firstWin);
	  _makeVisibleWithNewHeight (secondWin);
	  _makeVisibleWithNewHeight (cmdWin);
	  if (firstWin->generic.contentSize <= 0)
	    tuiEraseSourceContent (firstWin, EMPTY_SOURCE_PROMPT);
	  if (secondWin->generic.contentSize <= 0)
	    tuiEraseSourceContent (secondWin, EMPTY_SOURCE_PROMPT);
	  break;
	}
      /*
         ** Now remove all invisible windows, and their content so that they get
         ** created again when called for with the new size
       */
      for (winType = SRC_WIN; (winType < MAX_MAJOR_WINDOWS); winType++)
	{
	  if (winType != CMD_WIN && m_winPtrNotNull (winList[winType]) &&
	      !winList[winType]->generic.isVisible)
	    {
	      freeWindow (winList[winType]);
	      winList[winType] = (TuiWinInfoPtr) NULL;
	    }
	}
      tuiSetWinResizedTo (TRUE);
      /* turn keypad back on, unless focus is in the command window */
      if (winWithFocus != cmdWin)
	keypad (cmdWin->generic.handle, TRUE);
    }
  return;
}				/* tuiResizeAll */


/*
   ** tuiSigwinchHandler()
   **    SIGWINCH signal handler for the tui.  This signal handler is
   **    always called, even when the readline package clears signals
   **    because it is set as the old_sigwinch() (TUI only)
 */
void
tuiSigwinchHandler (int signal)
{
  /*
     ** Say that a resize was done so that the readline can do it
     ** later when appropriate.
   */
  tuiSetWinResizedTo (TRUE);

  return;
}				/* tuiSigwinchHandler */



/*************************
** STATIC LOCAL FUNCTIONS
**************************/


/*
   ** _tuiScrollForward_command().
 */
static void
_tuiScrollForward_command (char *arg, int fromTTY)
{
  int numToScroll = 1;
  TuiWinInfoPtr winToScroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg == (char *) NULL)
    _parseScrollingArgs (arg, &winToScroll, (int *) NULL);
  else
    _parseScrollingArgs (arg, &winToScroll, &numToScroll);
  tui_scroll (FORWARD_SCROLL, winToScroll, numToScroll);
}


/*
   ** _tuiScrollBackward_command().
 */
static void
_tuiScrollBackward_command (char *arg, int fromTTY)
{
  int numToScroll = 1;
  TuiWinInfoPtr winToScroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg == (char *) NULL)
    _parseScrollingArgs (arg, &winToScroll, (int *) NULL);
  else
    _parseScrollingArgs (arg, &winToScroll, &numToScroll);
  tui_scroll (BACKWARD_SCROLL, winToScroll, numToScroll);
}


/*
   ** _tuiScrollLeft_command().
 */
static void
_tuiScrollLeft_command (char *arg, int fromTTY)
{
  int numToScroll;
  TuiWinInfoPtr winToScroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  _parseScrollingArgs (arg, &winToScroll, &numToScroll);
  tui_scroll (LEFT_SCROLL, winToScroll, numToScroll);
}


/*
   ** _tuiScrollRight_command().
 */
static void
_tuiScrollRight_command (char *arg, int fromTTY)
{
  int numToScroll;
  TuiWinInfoPtr winToScroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  _parseScrollingArgs (arg, &winToScroll, &numToScroll);
  tui_scroll (RIGHT_SCROLL, winToScroll, numToScroll);
}


/*
   ** _tuiSetFocus().
   **     Set focus to the window named by 'arg'
 */
static void
_tuiSetFocus (char *arg, int fromTTY)
{
  if (arg != (char *) NULL)
    {
      char *bufPtr = (char *) xstrdup (arg);
      int i;
      TuiWinInfoPtr winInfo = (TuiWinInfoPtr) NULL;

      for (i = 0; (i < strlen (bufPtr)); i++)
	bufPtr[i] = toupper (arg[i]);

      if (subset_compare (bufPtr, "NEXT"))
	winInfo = tuiNextWin (tuiWinWithFocus ());
      else if (subset_compare (bufPtr, "PREV"))
	winInfo = tuiPrevWin (tuiWinWithFocus ());
      else
	winInfo = partialWinByName (bufPtr);

      if (winInfo == (TuiWinInfoPtr) NULL || !winInfo->generic.isVisible)
	warning ("Invalid window specified. \n\
The window name specified must be valid and visible.\n");
      else
	{
	  tuiSetWinFocusTo (winInfo);
	  keypad (cmdWin->generic.handle, (winInfo != cmdWin));
	}

      if (dataWin && dataWin->generic.isVisible)
	tuiRefreshDataWin ();
      tuiFree (bufPtr);
      printf_filtered ("Focus set to %s window.\n",
		       winName ((TuiGenWinInfoPtr) tuiWinWithFocus ()));
    }
  else
    warning ("Incorrect Number of Arguments.\n%s", FOCUS_USAGE);

  return;
}				/* _tuiSetFocus */

/*
   ** _tuiSetFocus_command()
 */
static void
_tuiSetFocus_command (char *arg, int fromTTY)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  _tuiSetFocus (arg, fromTTY);
}


/*
   ** _tuiAllWindowsInfo().
 */
static void
_tuiAllWindowsInfo (char *arg, int fromTTY)
{
  TuiWinType type;
  TuiWinInfoPtr winWithFocus = tuiWinWithFocus ();

  for (type = SRC_WIN; (type < MAX_MAJOR_WINDOWS); type++)
    if (winList[type] && winList[type]->generic.isVisible)
      {
	if (winWithFocus == winList[type])
	  printf_filtered ("        %s\t(%d lines)  <has focus>\n",
			   winName (&winList[type]->generic),
			   winList[type]->generic.height);
	else
	  printf_filtered ("        %s\t(%d lines)\n",
			   winName (&winList[type]->generic),
			   winList[type]->generic.height);
      }

  return;
}				/* _tuiAllWindowsInfo */


/*
   ** _tuiRefreshAll_command().
 */
static void
_tuiRefreshAll_command (char *arg, int fromTTY)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();

  tuiRefreshAll ();
}


/*
   ** _tuiSetWinTabWidth_command().
   **        Set the height of the specified window.
 */
static void
_tuiSetTabWidth_command (char *arg, int fromTTY)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg != (char *) NULL)
    {
      int ts;

      ts = atoi (arg);
      if (ts > 0)
	tuiSetDefaultTabLen (ts);
      else
	warning ("Tab widths greater than 0 must be specified.\n");
    }

  return;
}				/* _tuiSetTabWidth_command */


/*
   ** _tuiSetWinHeight().
   **        Set the height of the specified window.
 */
static void
_tuiSetWinHeight (char *arg, int fromTTY)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg != (char *) NULL)
    {
      char *buf = xstrdup (arg);
      char *bufPtr = buf;
      char *wname = (char *) NULL;
      int newHeight, i;
      TuiWinInfoPtr winInfo;

      wname = bufPtr;
      bufPtr = strchr (bufPtr, ' ');
      if (bufPtr != (char *) NULL)
	{
	  *bufPtr = (char) 0;

	  /*
	     ** Validate the window name
	   */
	  for (i = 0; i < strlen (wname); i++)
	    wname[i] = toupper (wname[i]);
	  winInfo = partialWinByName (wname);

	  if (winInfo == (TuiWinInfoPtr) NULL || !winInfo->generic.isVisible)
	    warning ("Invalid window specified. \n\
The window name specified must be valid and visible.\n");
	  else
	    {
	      /* Process the size */
	      while (*(++bufPtr) == ' ')
		;

	      if (*bufPtr != (char) 0)
		{
		  int negate = FALSE;
		  int fixedSize = TRUE;
		  int inputNo;;

		  if (*bufPtr == '+' || *bufPtr == '-')
		    {
		      if (*bufPtr == '-')
			negate = TRUE;
		      fixedSize = FALSE;
		      bufPtr++;
		    }
		  inputNo = atoi (bufPtr);
		  if (inputNo > 0)
		    {
		      if (negate)
			inputNo *= (-1);
		      if (fixedSize)
			newHeight = inputNo;
		      else
			newHeight = winInfo->generic.height + inputNo;
		      /*
		         ** Now change the window's height, and adjust all
		         ** other windows around it
		       */
		      if (_tuiAdjustWinHeights (winInfo,
						newHeight) == TUI_FAILURE)
			warning ("Invalid window height specified.\n%s",
				 WIN_HEIGHT_USAGE);
		      else
                        tui_update_gdb_sizes ();
		    }
		  else
		    warning ("Invalid window height specified.\n%s",
			     WIN_HEIGHT_USAGE);
		}
	    }
	}
      else
	printf_filtered (WIN_HEIGHT_USAGE);

      if (buf != (char *) NULL)
	tuiFree (buf);
    }
  else
    printf_filtered (WIN_HEIGHT_USAGE);

  return;
}				/* _tuiSetWinHeight */

/*
   ** _tuiSetWinHeight_command().
   **        Set the height of the specified window, with va_list.
 */
static void
_tuiSetWinHeight_command (char *arg, int fromTTY)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  _tuiSetWinHeight (arg, fromTTY);
}


/*
   ** _tuiXDBsetWinHeight().
   **        XDB Compatibility command for setting the window height.  This will
   **        increase or decrease the command window by the specified amount.
 */
static void
_tuiXDBsetWinHeight (char *arg, int fromTTY)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg != (char *) NULL)
    {
      int inputNo = atoi (arg);

      if (inputNo > 0)
	{			/* Add 1 for the locator */
	  int newHeight = termHeight () - (inputNo + 1);

	  if (!_newHeightOk (winList[CMD_WIN], newHeight) ||
	      _tuiAdjustWinHeights (winList[CMD_WIN],
				    newHeight) == TUI_FAILURE)
	    warning ("Invalid window height specified.\n%s",
		     XDBWIN_HEIGHT_USAGE);
	}
      else
	warning ("Invalid window height specified.\n%s",
		 XDBWIN_HEIGHT_USAGE);
    }
  else
    warning ("Invalid window height specified.\n%s", XDBWIN_HEIGHT_USAGE);

  return;
}				/* _tuiXDBsetWinHeight */

/*
   ** _tuiSetWinHeight_command().
   **        Set the height of the specified window, with va_list.
 */
static void
_tuiXDBsetWinHeight_command (char *arg, int fromTTY)
{
  _tuiXDBsetWinHeight (arg, fromTTY);
}


/*
   ** _tuiAdjustWinHeights().
   **        Function to adjust all window heights around the primary
 */
static TuiStatus
_tuiAdjustWinHeights (TuiWinInfoPtr primaryWinInfo, int newHeight)
{
  TuiStatus status = TUI_FAILURE;

  if (_newHeightOk (primaryWinInfo, newHeight))
    {
      status = TUI_SUCCESS;
      if (newHeight != primaryWinInfo->generic.height)
	{
	  int diff;
	  TuiWinInfoPtr winInfo;
	  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();
	  TuiLayoutType curLayout = currentLayout ();

	  diff = (newHeight - primaryWinInfo->generic.height) * (-1);
	  if (curLayout == SRC_COMMAND || curLayout == DISASSEM_COMMAND)
	    {
	      TuiWinInfoPtr srcWinInfo;

	      _makeInvisibleAndSetNewHeight (primaryWinInfo, newHeight);
	      if (primaryWinInfo->generic.type == CMD_WIN)
		{
		  winInfo = (TuiWinInfoPtr) (sourceWindows ())->list[0];
		  srcWinInfo = winInfo;
		}
	      else
		{
		  winInfo = winList[CMD_WIN];
		  srcWinInfo = primaryWinInfo;
		}
	      _makeInvisibleAndSetNewHeight (winInfo,
					     winInfo->generic.height + diff);
	      cmdWin->generic.origin.y = locator->origin.y + 1;
	      _makeVisibleWithNewHeight (winInfo);
	      _makeVisibleWithNewHeight (primaryWinInfo);
	      if (srcWinInfo->generic.contentSize <= 0)
		tuiEraseSourceContent (srcWinInfo, EMPTY_SOURCE_PROMPT);
	    }
	  else
	    {
	      TuiWinInfoPtr firstWin, secondWin;

	      if (curLayout == SRC_DISASSEM_COMMAND)
		{
		  firstWin = srcWin;
		  secondWin = disassemWin;
		}
	      else
		{
		  firstWin = dataWin;
		  secondWin = (TuiWinInfoPtr) (sourceWindows ())->list[0];
		}
	      if (primaryWinInfo == cmdWin)
		{		/*
				   ** Split the change in height accross the 1st & 2nd windows
				   ** adjusting them as well.
				 */
		  int firstSplitDiff = diff / 2;	/* subtract the locator */
		  int secondSplitDiff = firstSplitDiff;

		  if (diff % 2)
		    {
		      if (firstWin->generic.height >
			  secondWin->generic.height)
			if (diff < 0)
			  firstSplitDiff--;
			else
			  firstSplitDiff++;
		      else
			{
			  if (diff < 0)
			    secondSplitDiff--;
			  else
			    secondSplitDiff++;
			}
		    }
		  /* make sure that the minimum hieghts are honored */
		  while ((firstWin->generic.height + firstSplitDiff) < 3)
		    {
		      firstSplitDiff++;
		      secondSplitDiff--;
		    }
		  while ((secondWin->generic.height + secondSplitDiff) < 3)
		    {
		      secondSplitDiff++;
		      firstSplitDiff--;
		    }
		  _makeInvisibleAndSetNewHeight (
						  firstWin,
				 firstWin->generic.height + firstSplitDiff);
		  secondWin->generic.origin.y = firstWin->generic.height - 1;
		  _makeInvisibleAndSetNewHeight (
		    secondWin, secondWin->generic.height + secondSplitDiff);
		  cmdWin->generic.origin.y = locator->origin.y + 1;
		  _makeInvisibleAndSetNewHeight (cmdWin, newHeight);
		}
	      else
		{
		  if ((cmdWin->generic.height + diff) < 1)
		    {		/*
				   ** If there is no way to increase the command window
				   ** take real estate from the 1st or 2nd window.
				 */
		      if ((cmdWin->generic.height + diff) < 1)
			{
			  int i;
			  for (i = cmdWin->generic.height + diff;
			       (i < 1); i++)
			    if (primaryWinInfo == firstWin)
			      secondWin->generic.height--;
			    else
			      firstWin->generic.height--;
			}
		    }
		  if (primaryWinInfo == firstWin)
		    _makeInvisibleAndSetNewHeight (firstWin, newHeight);
		  else
		    _makeInvisibleAndSetNewHeight (
						    firstWin,
						  firstWin->generic.height);
		  secondWin->generic.origin.y = firstWin->generic.height - 1;
		  if (primaryWinInfo == secondWin)
		    _makeInvisibleAndSetNewHeight (secondWin, newHeight);
		  else
		    _makeInvisibleAndSetNewHeight (
				      secondWin, secondWin->generic.height);
		  cmdWin->generic.origin.y = locator->origin.y + 1;
		  if ((cmdWin->generic.height + diff) < 1)
		    _makeInvisibleAndSetNewHeight (cmdWin, 1);
		  else
		    _makeInvisibleAndSetNewHeight (
				     cmdWin, cmdWin->generic.height + diff);
		}
	      _makeVisibleWithNewHeight (cmdWin);
	      _makeVisibleWithNewHeight (secondWin);
	      _makeVisibleWithNewHeight (firstWin);
	      if (firstWin->generic.contentSize <= 0)
		tuiEraseSourceContent (firstWin, EMPTY_SOURCE_PROMPT);
	      if (secondWin->generic.contentSize <= 0)
		tuiEraseSourceContent (secondWin, EMPTY_SOURCE_PROMPT);
	    }
	}
    }

  return status;
}				/* _tuiAdjustWinHeights */


/*
   ** _makeInvisibleAndSetNewHeight().
   **        Function make the target window (and auxillary windows associated
   **        with the targer) invisible, and set the new height and location.
 */
static void
_makeInvisibleAndSetNewHeight (TuiWinInfoPtr winInfo, int height)
{
  int i;
  TuiGenWinInfoPtr genWinInfo;


  m_beInvisible (&winInfo->generic);
  winInfo->generic.height = height;
  if (height > 1)
    winInfo->generic.viewportHeight = height - 1;
  else
    winInfo->generic.viewportHeight = height;
  if (winInfo != cmdWin)
    winInfo->generic.viewportHeight--;

  /* Now deal with the auxillary windows associated with winInfo */
  switch (winInfo->generic.type)
    {
    case SRC_WIN:
    case DISASSEM_WIN:
      genWinInfo = winInfo->detail.sourceInfo.executionInfo;
      m_beInvisible (genWinInfo);
      genWinInfo->height = height;
      genWinInfo->origin.y = winInfo->generic.origin.y;
      if (height > 1)
	genWinInfo->viewportHeight = height - 1;
      else
	genWinInfo->viewportHeight = height;
      if (winInfo != cmdWin)
	genWinInfo->viewportHeight--;

      if (m_hasLocator (winInfo))
	{
	  genWinInfo = locatorWinInfoPtr ();
	  m_beInvisible (genWinInfo);
	  genWinInfo->origin.y = winInfo->generic.origin.y + height;
	}
      break;
    case DATA_WIN:
      /* delete all data item windows */
      for (i = 0; i < winInfo->generic.contentSize; i++)
	{
	  genWinInfo = (TuiGenWinInfoPtr) & ((TuiWinElementPtr)
		      winInfo->generic.content[i])->whichElement.dataWindow;
	  tuiDelwin (genWinInfo->handle);
	  genWinInfo->handle = (WINDOW *) NULL;
	}
      break;
    default:
      break;
    }
}


/*
   ** _makeVisibleWithNewHeight().
   **        Function to make the windows with new heights visible.
   **        This means re-creating the windows' content since the window
   **        had to be destroyed to be made invisible.
 */
static void
_makeVisibleWithNewHeight (TuiWinInfoPtr winInfo)
{
  struct symtab *s;

  m_beVisible (&winInfo->generic);
  checkAndDisplayHighlightIfNeeded (winInfo);
  switch (winInfo->generic.type)
    {
    case SRC_WIN:
    case DISASSEM_WIN:
      freeWinContent (winInfo->detail.sourceInfo.executionInfo);
      m_beVisible (winInfo->detail.sourceInfo.executionInfo);
      if (winInfo->generic.content != (OpaquePtr) NULL)
	{
	  TuiLineOrAddress lineOrAddr;

	  if (winInfo->generic.type == SRC_WIN)
	    lineOrAddr.lineNo =
	      winInfo->detail.sourceInfo.startLineOrAddr.lineNo;
	  else
	    lineOrAddr.addr =
	      winInfo->detail.sourceInfo.startLineOrAddr.addr;
	  freeWinContent (&winInfo->generic);
	  tuiUpdateSourceWindow (winInfo,
				 current_source_symtab, lineOrAddr, TRUE);
	}
      else if (selected_frame != (struct frame_info *) NULL)
	{
	  TuiLineOrAddress line;
	  extern int current_source_line;

	  s = find_pc_symtab (selected_frame->pc);
	  if (winInfo->generic.type == SRC_WIN)
	    line.lineNo = current_source_line;
	  else
	    {
	      find_line_pc (s, current_source_line, &line.addr);
	    }
	  tuiUpdateSourceWindow (winInfo, s, line, TRUE);
	}
      if (m_hasLocator (winInfo))
	{
	  m_beVisible (locatorWinInfoPtr ());
	  tuiShowLocatorContent ();
	}
      break;
    case DATA_WIN:
      tuiDisplayAllData ();
      break;
    case CMD_WIN:
      winInfo->detail.commandInfo.curLine = 0;
      winInfo->detail.commandInfo.curch = 0;
      wmove (winInfo->generic.handle,
	     winInfo->detail.commandInfo.curLine,
	     winInfo->detail.commandInfo.curch);
      break;
    default:
      break;
    }

  return;
}				/* _makeVisibleWithNewHeight */


static int
_newHeightOk (TuiWinInfoPtr primaryWinInfo, int newHeight)
{
  int ok = (newHeight < termHeight ());

  if (ok)
    {
      int diff;
      TuiLayoutType curLayout = currentLayout ();

      diff = (newHeight - primaryWinInfo->generic.height) * (-1);
      if (curLayout == SRC_COMMAND || curLayout == DISASSEM_COMMAND)
	{
	  ok = ((primaryWinInfo->generic.type == CMD_WIN &&
		 newHeight <= (termHeight () - 4) &&
		 newHeight >= MIN_CMD_WIN_HEIGHT) ||
		(primaryWinInfo->generic.type != CMD_WIN &&
		 newHeight <= (termHeight () - 2) &&
		 newHeight >= MIN_WIN_HEIGHT));
	  if (ok)
	    {			/* check the total height */
	      TuiWinInfoPtr winInfo;

	      if (primaryWinInfo == cmdWin)
		winInfo = (TuiWinInfoPtr) (sourceWindows ())->list[0];
	      else
		winInfo = cmdWin;
	      ok = ((newHeight +
		     (winInfo->generic.height + diff)) <= termHeight ());
	    }
	}
      else
	{
	  int curTotalHeight, totalHeight, minHeight = 0;
	  TuiWinInfoPtr firstWin, secondWin;

	  if (curLayout == SRC_DISASSEM_COMMAND)
	    {
	      firstWin = srcWin;
	      secondWin = disassemWin;
	    }
	  else
	    {
	      firstWin = dataWin;
	      secondWin = (TuiWinInfoPtr) (sourceWindows ())->list[0];
	    }
	  /*
	     ** We could simply add all the heights to obtain the same result
	     ** but below is more explicit since we subtract 1 for the
	     ** line that the first and second windows share, and add one
	     ** for the locator.
	   */
	  totalHeight = curTotalHeight =
	    (firstWin->generic.height + secondWin->generic.height - 1)
	    + cmdWin->generic.height + 1 /*locator */ ;
	  if (primaryWinInfo == cmdWin)
	    {
	      /* locator included since first & second win share a line */
	      ok = ((firstWin->generic.height +
		     secondWin->generic.height + diff) >=
		    (MIN_WIN_HEIGHT * 2) &&
		    newHeight >= MIN_CMD_WIN_HEIGHT);
	      if (ok)
		{
		  totalHeight = newHeight + (firstWin->generic.height +
					  secondWin->generic.height + diff);
		  minHeight = MIN_CMD_WIN_HEIGHT;
		}
	    }
	  else
	    {
	      minHeight = MIN_WIN_HEIGHT;
	      /*
	         ** First see if we can increase/decrease the command
	         ** window.  And make sure that the command window is
	         ** at least 1 line
	       */
	      ok = ((cmdWin->generic.height + diff) > 0);
	      if (!ok)
		{		/*
				   ** Looks like we have to increase/decrease one of
				   ** the other windows
				 */
		  if (primaryWinInfo == firstWin)
		    ok = (secondWin->generic.height + diff) >= minHeight;
		  else
		    ok = (firstWin->generic.height + diff) >= minHeight;
		}
	      if (ok)
		{
		  if (primaryWinInfo == firstWin)
		    totalHeight = newHeight +
		      secondWin->generic.height +
		      cmdWin->generic.height + diff;
		  else
		    totalHeight = newHeight +
		      firstWin->generic.height +
		      cmdWin->generic.height + diff;
		}
	    }
	  /*
	     ** Now make sure that the proposed total height doesn't exceed
	     ** the old total height.
	   */
	  if (ok)
	    ok = (newHeight >= minHeight && totalHeight <= curTotalHeight);
	}
    }

  return ok;
}				/* _newHeightOk */


/*
   ** _parseScrollingArgs().
 */
static void
_parseScrollingArgs (char *arg, TuiWinInfoPtr * winToScroll, int *numToScroll)
{
  if (numToScroll)
    *numToScroll = 0;
  *winToScroll = tuiWinWithFocus ();

  /*
     ** First set up the default window to scroll, in case there is no
     ** window name arg
   */
  if (arg != (char *) NULL)
    {
      char *buf, *bufPtr;

      /* process the number of lines to scroll */
      buf = bufPtr = xstrdup (arg);
      if (isdigit (*bufPtr))
	{
	  char *numStr;

	  numStr = bufPtr;
	  bufPtr = strchr (bufPtr, ' ');
	  if (bufPtr != (char *) NULL)
	    {
	      *bufPtr = (char) 0;
	      if (numToScroll)
		*numToScroll = atoi (numStr);
	      bufPtr++;
	    }
	  else if (numToScroll)
	    *numToScroll = atoi (numStr);
	}

      /* process the window name if one is specified */
      if (bufPtr != (char *) NULL)
	{
	  char *wname;
	  int i;

	  if (*bufPtr == ' ')
	    while (*(++bufPtr) == ' ')
	      ;

	  if (*bufPtr != (char) 0)
	    wname = bufPtr;
	  else
	    wname = "?";
	  
	  /* Validate the window name */
	  for (i = 0; i < strlen (wname); i++)
	    wname[i] = toupper (wname[i]);
	  *winToScroll = partialWinByName (wname);

	  if (*winToScroll == (TuiWinInfoPtr) NULL ||
	      !(*winToScroll)->generic.isVisible)
	    warning ("Invalid window specified. \n\
The window name specified must be valid and visible.\n");
	  else if (*winToScroll == cmdWin)
	    *winToScroll = (TuiWinInfoPtr) (sourceWindows ())->list[0];
	}
      tuiFree (buf);
    }

  return;
}				/* _parseScrollingArgs */
