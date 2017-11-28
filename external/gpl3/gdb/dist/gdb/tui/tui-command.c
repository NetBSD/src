/* Specific command window processing.

   Copyright (C) 1998-2017 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-win.h"
#include "tui/tui-io.h"
#include "tui/tui-command.h"

#include "gdb_curses.h"
/*****************************************
** STATIC LOCAL FUNCTIONS FORWARD DECLS    **
******************************************/



/*****************************************
** PUBLIC FUNCTIONS                        **
******************************************/

/* Dispatch the correct tui function based upon the control
   character.  */
unsigned int
tui_dispatch_ctrl_char (unsigned int ch)
{
  struct tui_win_info *win_info = tui_win_with_focus ();

  /* Handle the CTRL-L refresh for each window.  */
  if (ch == '\f')
    tui_refresh_all_win ();

  /* If the command window has the logical focus, or no-one does
     assume it is the command window; in this case, pass the character
     on through and do nothing here.  */
  if (win_info == NULL || win_info == TUI_CMD_WIN)
    return ch;

  switch (ch)
    {
    case KEY_NPAGE:
      tui_scroll_forward (win_info, 0);
      break;
    case KEY_PPAGE:
      tui_scroll_backward (win_info, 0);
      break;
    case KEY_DOWN:
    case KEY_SF:
      tui_scroll_forward (win_info, 1);
      break;
    case KEY_UP:
    case KEY_SR:
      tui_scroll_backward (win_info, 1);
      break;
    case KEY_RIGHT:
      tui_scroll_left (win_info, 1);
      break;
    case KEY_LEFT:
      tui_scroll_right (win_info, 1);
      break;
    case '\f':
      break;
    default:
      /* We didn't recognize the character as a control character, so pass it
         through.  */
      return ch;
    }

  /* We intercepted the control character, so return 0 (which readline
     will interpret as a no-op).  */
  return 0;
}

/* See tui-command.h.  */

void
tui_refresh_cmd_win (void)
{
  WINDOW *w = TUI_CMD_WIN->generic.handle;

  wrefresh (w);

  /* FIXME: It's not clear why this is here.
     It was present in the original tui_puts code and is kept in order to
     not introduce some subtle breakage.  */
  fflush (stdout);
}
