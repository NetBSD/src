/* TUI display registers in window.

   Copyright (C) 1998-2024 Free Software Foundation, Inc.

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

#ifndef TUI_TUI_REGS_H
#define TUI_TUI_REGS_H

#include "tui/tui-data.h"
#include "reggroups.h"

/* Information about the display of a single register.  */

struct tui_register_info
{
  tui_register_info (int regno, const frame_info_ptr &frame)
    : m_regno (regno)
  {
    update (frame);
    highlight = false;
  }

  DISABLE_COPY_AND_ASSIGN (tui_register_info);

  tui_register_info (tui_register_info &&) = default;

  void update (const frame_info_ptr &frame);

  void rerender (WINDOW *handle, int field_width);

  bool visible () const
  { return y > 0; }

  /* Location.  */
  int x = 0;
  int y = 0;
  bool highlight = false;
  std::string content;

private:

  /* The register number.  */
  const int m_regno;
};

/* The TUI registers window.  */
struct tui_data_window : public tui_win_info
{
  tui_data_window ()
  {
    update_register_data (nullptr);
  }

  DISABLE_COPY_AND_ASSIGN (tui_data_window);

  const char *name () const override
  {
    return DATA_NAME;
  }

  void check_register_values (const frame_info_ptr &frame);

  void set_register_group (const reggroup *group);

  const reggroup *get_current_group () const
  {
    return m_current_group;
  }

protected:

  void do_scroll_vertical (int num_to_scroll) override;
  void do_scroll_horizontal (int num_to_scroll) override
  {
  }

  void rerender () override;

private:

  /* Display the registers in the content from 'start_element_no'
     until the end of the register content or the end of the display
     height.  No checking for displaying past the end of the registers
     is done here.  */
  void display_registers_from (int start_element_no);

  /* Display the registers starting at line line_no in the data
     window.  Answers the line number that the display actually
     started from.  If nothing is displayed (-1) is returned.  */
  int display_registers_from_line (int line_no);

  /* Return the index of the first element displayed.  If none are
     displayed, then return -1.  */
  int first_data_item_displayed ();

  /* Display the registers in the content from 'start_element_no' on
     'start_line_no' until the end of the register content or the end
     of the display height.  This function checks that we won't
     display off the end of the register display.  */
  void display_reg_element_at_line (int start_element_no, int start_line_no);

  void update_register_data (const reggroup *group);

  /* Answer the number of the last line in the regs display.  If there
     are no registers (-1) is returned.  */
  int last_regs_line_no () const;

  /* Answer the line number that the register element at element_no is
     on.  If element_no is greater than the number of register
     elements there are, -1 is returned.  */
  int line_from_reg_element_no (int element_no) const;

  /* Answer the index of the first element in line_no.  If line_no is
     past the register area (-1) is returned.  */
  int first_reg_element_no_inline (int line_no) const;

  void erase_data_content ();

  /* Information about each register in the current register group.  */
  std::vector<tui_register_info> m_regs_content;
  int m_regs_column_count = 0;
  const reggroup *m_current_group = nullptr;

  /* Width of each register's display area.  */
  int m_item_width = 0;

  /* Architecture of frame whose registers are being displayed, or
     nullptr if the display is empty (i.e., there is no frame).  */
  gdbarch *m_gdbarch = nullptr;
};

#endif /* TUI_TUI_REGS_H */
