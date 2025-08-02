/* TUI display locator.

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

#ifndef TUI_TUI_STACK_H
#define TUI_TUI_STACK_H

#include "tui/tui-data.h"

class frame_info_ptr;

/* Locator window class.  */

struct tui_locator_window : public tui_win_info
{
  tui_locator_window () = default;

  const char *name () const override
  {
    return STATUS_NAME;
  }

  int max_height () const override
  {
    return 1;
  }

  int min_height () const override
  {
    return 1;
  }

  bool can_box () const override
  {
    return false;
  }

  bool can_focus () const override
  {
    return false;
  }

  void rerender () override;

protected:

  void do_scroll_vertical (int n) override
  {
  }

  void do_scroll_horizontal (int n) override
  {
  }

private:

  /* Create the status line to display as much information as we can
     on this single line: target name, process number, current
     function, current line, current PC, SingleKey mode.  */

  std::string make_status_line () const;
};

extern void tui_show_locator_content (void);
extern bool tui_show_frame_info (frame_info_ptr);

#endif /* TUI_TUI_STACK_H */
