/* Output generating routines for GDB CLI.

   Copyright (C) 1999-2017 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions.
   Written by Fernando Nasser for Cygnus.

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
#include "ui-out.h"
#include "tui-out.h"
#include "tui.h"

/* Output an int field.  */

void
tui_ui_out::do_field_int (int fldno, int width, ui_align alignment,
			  const char *fldname, int value)
{
  if (suppress_output ())
    return;

  /* Don't print line number, keep it for later.  */
  if (m_start_of_line == 0 && strcmp (fldname, "line") == 0)
    {
      m_start_of_line++;
      m_line = value;
      return;
    }
  m_start_of_line++;

  cli_ui_out::do_field_int (fldno, width, alignment, fldname, value);
}

/* Other cli_field_* end up here so alignment and field separators are
   both handled by tui_field_string.  */

void
tui_ui_out::do_field_string (int fldno, int width, ui_align align,
			     const char *fldname, const char *string)
{
  if (suppress_output ())
    return;

  if (fldname && m_line > 0 && strcmp (fldname, "fullname") == 0)
    {
      m_start_of_line++;
      if (m_line > 0)
        {
          tui_show_source (string, m_line);
        }
      return;
    }
  
  m_start_of_line++;

  cli_ui_out::do_field_string (fldno, width, align, fldname, string);
}

/* This is the only field function that does not align.  */

void
tui_ui_out::do_field_fmt (int fldno, int width, ui_align align,
			  const char *fldname, const char *format,
			  va_list args)
{
  if (suppress_output ())
    return;

  m_start_of_line++;

  cli_ui_out::do_field_fmt (fldno, width, align, fldname, format, args);
}

void
tui_ui_out::do_text (const char *string)
{
  if (suppress_output ())
    return;

  m_start_of_line++;
  if (m_line > 0)
    {
      if (strchr (string, '\n') != 0)
        {
          m_line = -1;
          m_start_of_line = 0;
        }
      return;
    }
  if (strchr (string, '\n'))
    m_start_of_line = 0;

  cli_ui_out::do_text (string);
}

tui_ui_out::tui_ui_out (ui_file *stream)
: cli_ui_out (stream, 0),
  m_line (0),
  m_start_of_line (-1)
{
}

tui_ui_out *
tui_out_new (struct ui_file *stream)
{
  return new tui_ui_out (stream);
}
