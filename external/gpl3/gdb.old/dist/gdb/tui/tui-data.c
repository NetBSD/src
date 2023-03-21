/* TUI data manipulation routines.

   Copyright (C) 1998-2020 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-winsource.h"
#include "gdb_curses.h"
#include <algorithm>

struct tui_win_info *tui_win_list[MAX_MAJOR_WINDOWS];

static int term_height, term_width;
static struct tui_win_info *win_with_focus = NULL;

static bool win_resized = false;

/* Answer a whether the terminal window has been resized or not.  */
bool
tui_win_resized ()
{
  return win_resized;
}


/* Set a whether the terminal window has been resized or not.  */
void
tui_set_win_resized_to (bool resized)
{
  win_resized = resized;
}


/* Answer the window with the logical focus.  */
struct tui_win_info *
tui_win_with_focus (void)
{
  return win_with_focus;
}


/* Set the logical focus to win_info.  */
void
tui_set_win_focus_to (struct tui_win_info *win_info)
{
  if (win_info != NULL)
    {
      tui_unhighlight_win (win_with_focus);
      win_with_focus = win_info;
      tui_highlight_win (win_info);
    }
}


/* Accessor for the term_height.  */
int
tui_term_height (void)
{
  return term_height;
}


/* Mutator for the term height.  */
void
tui_set_term_height_to (int h)
{
  term_height = h;
}


/* Accessor for the term_width.  */
int
tui_term_width (void)
{
  return term_width;
}


/* Mutator for the term_width.  */
void
tui_set_term_width_to (int w)
{
  term_width = w;
}


/* Answer the next window in the list, cycling back to the top if
   necessary.  */
struct tui_win_info *
tui_next_win (struct tui_win_info *cur_win)
{
  auto iter = std::find (tui_windows.begin (), tui_windows.end (), cur_win);
  gdb_assert (iter != tui_windows.end ());

  ++iter;
  if (iter == tui_windows.end ())
    return tui_windows[0];
  return *iter;
}


/* Answer the prev window in the list, cycling back to the bottom if
   necessary.  */
struct tui_win_info *
tui_prev_win (struct tui_win_info *cur_win)
{
  auto iter = std::find (tui_windows.begin (), tui_windows.end (), cur_win);
  gdb_assert (iter != tui_windows.end ());

  if (iter == tui_windows.begin ())
    return tui_windows.back ();
  --iter;
  return *iter;
}


void
tui_win_info::rerender ()
{
  check_and_display_highlight_if_needed ();
}
