/* Output generating routines for GDB CLI.

   Copyright (C) 1999-2016 Free Software Foundation, Inc.

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
#include "cli-out.h"
#include "tui.h"
struct tui_ui_out_data
  {
    struct cli_ui_out_data base;

    int line;
    int start_of_line;
  };
typedef struct tui_ui_out_data tui_out_data;

/* This is the TUI ui-out implementation functions vector.  It is
   initialized below in _initialize_tui_out, inheriting the CLI
   version, and overriding a few methods.  */

static struct ui_out_impl tui_ui_out_impl;

/* Output an int field.  */

static void
tui_field_int (struct ui_out *uiout, 
	       int fldno, int width,
	       enum ui_align alignment,
	       const char *fldname, 
	       int value)
{
  tui_out_data *data = (tui_out_data *) ui_out_data (uiout);

  if (data->base.suppress_output)
    return;

  /* Don't print line number, keep it for later.  */
  if (data->start_of_line == 0 && strcmp (fldname, "line") == 0)
    {
      data->start_of_line ++;
      data->line = value;
      return;
    }
  data->start_of_line ++;

  (*cli_ui_out_impl.field_int) (uiout, fldno,
				width, alignment, fldname, value);
}

/* Other cli_field_* end up here so alignment and field separators are
   both handled by tui_field_string.  */

static void
tui_field_string (struct ui_out *uiout,
		  int fldno, int width,
		  enum ui_align align,
		  const char *fldname,
		  const char *string)
{
  tui_out_data *data = (tui_out_data *) ui_out_data (uiout);

  if (data->base.suppress_output)
    return;

  if (fldname && data->line > 0 && strcmp (fldname, "fullname") == 0)
    {
      data->start_of_line ++;
      if (data->line > 0)
        {
          tui_show_source (string, data->line);
        }
      return;
    }
  
  data->start_of_line++;

  (*cli_ui_out_impl.field_string) (uiout, fldno,
				   width, align,
				   fldname, string);
}

/* This is the only field function that does not align.  */

static void
tui_field_fmt (struct ui_out *uiout, int fldno,
	       int width, enum ui_align align,
	       const char *fldname,
	       const char *format,
	       va_list args)
{
  tui_out_data *data = (tui_out_data *) ui_out_data (uiout);

  if (data->base.suppress_output)
    return;

  data->start_of_line++;

  (*cli_ui_out_impl.field_fmt) (uiout, fldno,
				width, align,
				fldname, format, args);
}

static void
tui_text (struct ui_out *uiout, const char *string)
{
  tui_out_data *data = (tui_out_data *) ui_out_data (uiout);

  if (data->base.suppress_output)
    return;
  data->start_of_line ++;
  if (data->line > 0)
    {
      if (strchr (string, '\n') != 0)
        {
          data->line = -1;
          data->start_of_line = 0;
        }
      return;
    }
  if (strchr (string, '\n'))
    data->start_of_line = 0;

  (*cli_ui_out_impl.text) (uiout, string);
}

struct ui_out *
tui_out_new (struct ui_file *stream)
{
  int flags = 0;

  tui_out_data *data = XNEW (tui_out_data);

  /* Initialize base "class".  */
  cli_out_data_ctor (&data->base, stream);

  /* Initialize our fields.  */
  data->line = -1;
  data->start_of_line = 0;

  return ui_out_new (&tui_ui_out_impl, data, flags);
}

/* Standard gdb initialization hook.  */

extern void _initialize_tui_out (void);

void
_initialize_tui_out (void)
{
  /* Inherit the CLI version.  */
  tui_ui_out_impl = cli_ui_out_impl;

  /* Override a few methods.  */
  tui_ui_out_impl.field_int = tui_field_int;
  tui_ui_out_impl.field_string = tui_field_string;
  tui_ui_out_impl.field_fmt = tui_field_fmt;
  tui_ui_out_impl.text = tui_text;
}
