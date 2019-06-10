/* Copyright (C) 2016-2017 Free Software Foundation, Inc.

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

#ifndef TUI_OUT_H
#define TUI_OUT_H

#include "cli-out.h"

class tui_ui_out : public cli_ui_out
{
public:

  explicit tui_ui_out (ui_file *stream);

protected:

  void do_field_int (int fldno, int width, ui_align align, const char *fldname,
		  int value) override;
  void do_field_string (int fldno, int width, ui_align align, const char *fldname,
		     const char *string) override;
  void do_field_fmt (int fldno, int width, ui_align align, const char *fldname,
		  const char *format, va_list args) override
    ATTRIBUTE_PRINTF (6,0);
  void do_text (const char *string) override;

private:

  int m_line;
  int m_start_of_line;
};

extern tui_ui_out *tui_out_new (struct ui_file *stream);

#endif
