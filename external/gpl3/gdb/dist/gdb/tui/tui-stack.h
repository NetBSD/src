/* TUI display locator.

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

#ifndef TUI_TUI_STACK_H
#define TUI_TUI_STACK_H

#include "tui/tui-data.h"

struct frame_info;

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

  void rerender () override;

  /* Update the locator, with the provided arguments.

     Returns true if any of the locator's fields were actually
     changed, and false otherwise.  */
  bool set_locator_info (struct gdbarch *gdbarch,
			 const struct symtab_and_line &sal,
			 const char *procname);

  /* Set the full_name portion of the locator.  */
  void set_locator_fullname (const char *fullname);

  std::string full_name;
  std::string proc_name;
  int line_no = 0;
  CORE_ADDR addr = 0;
  /* Architecture associated with code at this location.  */
  struct gdbarch *gdbarch = nullptr;

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

extern void tui_update_locator_fullname (struct symtab *symtab);
extern void tui_show_locator_content (void);
extern bool tui_show_frame_info (struct frame_info *);

#endif /* TUI_TUI_STACK_H */
