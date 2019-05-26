/* Interface for common GDB/MI data
   Copyright (C) 2005-2017 Free Software Foundation, Inc.

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

#ifndef MI_COMMON_H
#define MI_COMMON_H

#include "interps.h"

struct mi_console_file;

/* Represents the reason why GDB is sending an asynchronous command to
   the front end.  NOTE: When modifing this, don't forget to update
   gdb.texinfo!  */
enum async_reply_reason
{
  EXEC_ASYNC_BREAKPOINT_HIT = 0,
  EXEC_ASYNC_WATCHPOINT_TRIGGER,
  EXEC_ASYNC_READ_WATCHPOINT_TRIGGER,
  EXEC_ASYNC_ACCESS_WATCHPOINT_TRIGGER,
  EXEC_ASYNC_FUNCTION_FINISHED,
  EXEC_ASYNC_LOCATION_REACHED,
  EXEC_ASYNC_WATCHPOINT_SCOPE,
  EXEC_ASYNC_END_STEPPING_RANGE,
  EXEC_ASYNC_EXITED_SIGNALLED,
  EXEC_ASYNC_EXITED,
  EXEC_ASYNC_EXITED_NORMALLY,
  EXEC_ASYNC_SIGNAL_RECEIVED,
  EXEC_ASYNC_SOLIB_EVENT,
  EXEC_ASYNC_FORK,
  EXEC_ASYNC_VFORK,
  EXEC_ASYNC_SYSCALL_ENTRY,
  EXEC_ASYNC_SYSCALL_RETURN,
  EXEC_ASYNC_EXEC,
  /* This is here only to represent the number of enums.  */
  EXEC_ASYNC_LAST
};

const char *async_reason_lookup (enum async_reply_reason reason);

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
  void set_logging (ui_file_up logfile, bool logging_redirect) override;
  void pre_command_loop () override;

  /* MI's output channels */
  mi_console_file *out;
  mi_console_file *err;
  mi_console_file *log;
  mi_console_file *targ;
  mi_console_file *event_channel;

  /* Raw console output.  */
  struct ui_file *raw_stdout;

  /* Save the original value of raw_stdout here when logging, so we
     can restore correctly when done.  */
  struct ui_file *saved_raw_stdout;

  /* MI's builder.  */
  struct ui_out *mi_uiout;

  /* MI's CLI builder (wraps OUT).  */
  struct ui_out *cli_uiout;
};

#endif
