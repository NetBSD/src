/* CLI Definitions for GDB, the GNU debugger.

   Copyright (C) 2016-2017 Free Software Foundation, Inc.

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

#ifndef CLI_INTERP_H
#define CLI_INTERP_H 1

#include "interps.h"

/* A console-like interpreter.  Implements functionality common to the
   CLI and the TUI.  */
class cli_interp_base : public interp
{
public:
  explicit cli_interp_base (const char *name);
  virtual ~cli_interp_base () = 0;

  void set_logging (ui_file_up logfile, bool logging_redirect) override;
  void pre_command_loop () override;
  bool supports_command_editing () override;
};

/* Make the output ui_file to use when logging is enabled.
   CURR_OUTPUT is the stream where output is currently being sent to
   (e.g., gdb_stdout for the CLI, raw output stream for the MI).
   LOGFILE is the log file already opened by the caller.
   LOGGING_REDIRECT is the value of the "set logging redirect"
   setting.  If true, the resulting output is the logfile.  If false,
   the output stream is a tee, with the log file as one of the
   outputs.  Ownership of LOGFILE is transferred to the returned
   output file, which is an owning pointer.  */
extern ui_file *make_logging_output (ui_file *curr_output,
				     ui_file_up logfile,
				     bool logging_redirect);

/* The CLI interpreter's set_logging_proc method.  Exported so other
   interpreters can reuse it.  */
extern void cli_set_logging (struct interp *interp,
			     ui_file_up logfile, bool logging_redirect);

extern int cli_interpreter_supports_command_editing (struct interp *interp);

extern void cli_interpreter_pre_command_loop (struct interp *self);

/* Returns true if the current stop should be printed to
   CONSOLE_INTERP.  */
extern int should_print_stop_to_console (struct interp *interp,
					 struct thread_info *tp);

#endif
