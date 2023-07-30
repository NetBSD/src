/* Specific command window processing.

   Copyright (C) 1998-2023 Free Software Foundation, Inc.

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

#ifndef TUI_TUI_COMMAND_H
#define TUI_TUI_COMMAND_H

#include "tui/tui-data.h"

/* The TUI command window.  */
struct tui_cmd_window : public tui_win_info
{
  tui_cmd_window () = default;

  DISABLE_COPY_AND_ASSIGN (tui_cmd_window);

  void refresh_window () override
  {
  }

  const char *name () const override
  {
    return CMD_NAME;
  }

  bool can_scroll () const override
  {
    return false;
  }

  bool can_box () const override
  {
    return false;
  }

  void resize (int height, int width, int origin_x, int origin_y) override;

  void make_visible (bool visible) override
  {
    /* The command window can't be made invisible.  */
  }

  int start_line = 0;

protected:

  void do_scroll_vertical (int num_to_scroll) override
  {
  }

  void do_scroll_horizontal (int num_to_scroll) override
  {
  }
};

/* Refresh the command window.  */
extern void tui_refresh_cmd_win (void);

#endif /* TUI_TUI_COMMAND_H */
