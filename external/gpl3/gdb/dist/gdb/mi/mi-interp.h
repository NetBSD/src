/* MI Interpreter Definitions and Commands for GDB, the GNU debugger.

   Copyright (C) 2017-2023 Free Software Foundation, Inc.

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

#ifndef MI_MI_INTERP_H
#define MI_MI_INTERP_H

#include "interps.h"

struct mi_console_file;

/* An MI interpreter.  */

class mi_interp final : public interp
{
public:
  mi_interp (const char *name)
    : interp (name)
  {}

  void init (bool top_level) override;
  void resume () override;
  void suspend () override;
  gdb_exception exec (const char *command_str) override;
  ui_out *interp_ui_out () override;
  void set_logging (ui_file_up logfile, bool logging_redirect,
		    bool debug_redirect) override;
  void pre_command_loop () override;

  /* MI's output channels */
  mi_console_file *out;
  mi_console_file *err;
  mi_console_file *log;
  mi_console_file *targ;
  mi_console_file *event_channel;

  /* Raw console output.  */
  struct ui_file *raw_stdout;

  /* Save the original value of raw_stdout here when logging, and the
     file which we need to delete, so we can restore correctly when
     done.  */
  struct ui_file *saved_raw_stdout;
  ui_file_up logfile_holder;
  ui_file_up stdout_holder;

  /* MI's builder.  */
  struct ui_out *mi_uiout;

  /* MI's CLI builder (wraps OUT).  */
  struct ui_out *cli_uiout;
};

/* Output the shared object attributes to UIOUT.  */

void mi_output_solib_attribs (ui_out *uiout, struct so_list *solib);

#endif /* MI_MI_INTERP_H */
