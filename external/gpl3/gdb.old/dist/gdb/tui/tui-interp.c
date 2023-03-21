/* TUI Interpreter definitions for GDB, the GNU debugger.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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
#include "cli/cli-interp.h"
#include "interps.h"
#include "top.h"
#include "event-top.h"
#include "gdbsupport/event-loop.h"
#include "ui-out.h"
#include "cli-out.h"
#include "tui/tui-data.h"
#include "tui/tui-win.h"
#include "tui/tui.h"
#include "tui/tui-io.h"
#include "infrun.h"
#include "observable.h"
#include "gdbthread.h"
#include "inferior.h"
#include "main.h"

/* Set to true when the TUI mode must be activated when we first start
   gdb.  */
static bool tui_start_enabled = false;

class tui_interp final : public cli_interp_base
{
public:
  explicit tui_interp (const char *name)
    : cli_interp_base (name)
  {}

  void init (bool top_level) override;
  void resume () override;
  void suspend () override;
  gdb_exception exec (const char *command_str) override;
  ui_out *interp_ui_out () override;
};

/* Returns the INTERP if the INTERP is a TUI, and returns NULL
   otherwise.  */

static tui_interp *
as_tui_interp (struct interp *interp)
{
  return dynamic_cast<tui_interp *> (interp);
}

/* Cleanup the tui before exiting.  */

static void
tui_exit (void)
{
  /* Disable the tui.  Curses mode is left leaving the screen in a
     clean state (see endwin()).  */
  tui_disable ();
}

/* Observers for several run control events.  If the interpreter is
   quiet (i.e., another interpreter is being run with
   interpreter-exec), print nothing.  */

/* Observer for the normal_stop notification.  */

static void
tui_on_normal_stop (struct bpstats *bs, int print_frame)
{
  if (!print_frame)
    return;

  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *interp = top_level_interpreter ();
      struct interp *tui = as_tui_interp (interp);
      struct thread_info *thread;

      if (tui == NULL)
	continue;

      thread = inferior_thread ();
      if (should_print_stop_to_console (interp, thread))
	print_stop_event (tui->interp_ui_out ());
    }
}

/* Observer for the signal_received notification.  */

static void
tui_on_signal_received (enum gdb_signal siggnal)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_signal_received_reason (tui->interp_ui_out (), siggnal);
    }
}

/* Observer for the end_stepping_range notification.  */

static void
tui_on_end_stepping_range (void)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_end_stepping_range_reason (tui->interp_ui_out ());
    }
}

/* Observer for the signal_exited notification.  */

static void
tui_on_signal_exited (enum gdb_signal siggnal)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_signal_exited_reason (tui->interp_ui_out (), siggnal);
    }
}

/* Observer for the exited notification.  */

static void
tui_on_exited (int exitstatus)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_exited_reason (tui->interp_ui_out (), exitstatus);
    }
}

/* Observer for the no_history notification.  */

static void
tui_on_no_history (void)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_no_history_reason (tui->interp_ui_out ());
    }
}

/* Observer for the sync_execution_done notification.  */

static void
tui_on_sync_execution_done (void)
{
  struct interp *tui = as_tui_interp (top_level_interpreter ());

  if (tui == NULL)
    return;

  display_gdb_prompt (NULL);
}

/* Observer for the command_error notification.  */

static void
tui_on_command_error (void)
{
  struct interp *tui = as_tui_interp (top_level_interpreter ());

  if (tui == NULL)
    return;

  display_gdb_prompt (NULL);
}

/* Observer for the user_selected_context_changed notification.  */

static void
tui_on_user_selected_context_changed (user_selected_what selection)
{
  /* This event is suppressed.  */
  if (cli_suppress_notification.user_selected_context)
    return;

  thread_info *tp = inferior_ptid != null_ptid ? inferior_thread () : NULL;

  SWITCH_THRU_ALL_UIS ()
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      if (selection & USER_SELECTED_INFERIOR)
	print_selected_inferior (tui->interp_ui_out ());

      if (tp != NULL
	  && ((selection & (USER_SELECTED_THREAD | USER_SELECTED_FRAME))))
	print_selected_thread_frame (tui->interp_ui_out (), selection);

    }
}

/* These implement the TUI interpreter.  */

void
tui_interp::init (bool top_level)
{
  /* Install exit handler to leave the screen in a good shape.  */
  atexit (tui_exit);

  tui_initialize_io ();
  tui_initialize_win ();
  if (gdb_stdout->isatty ())
    tui_ensure_readline_initialized ();
}

void
tui_interp::resume ()
{
  struct ui *ui = current_ui;
  struct ui_file *stream;

  /* gdb_setup_readline will change gdb_stdout.  If the TUI was
     previously writing to gdb_stdout, then set it to the new
     gdb_stdout afterwards.  */

  stream = tui_old_uiout->set_stream (gdb_stdout);
  if (stream != gdb_stdout)
    {
      tui_old_uiout->set_stream (stream);
      stream = NULL;
    }

  gdb_setup_readline (1);

  ui->input_handler = command_line_handler;

  if (stream != NULL)
    tui_old_uiout->set_stream (gdb_stdout);

  if (tui_start_enabled)
    tui_enable ();
}

void
tui_interp::suspend ()
{
  tui_start_enabled = tui_active;
  tui_disable ();
}

ui_out *
tui_interp::interp_ui_out ()
{
  if (tui_active)
    return tui_out;
  else
    return tui_old_uiout;
}

gdb_exception
tui_interp::exec (const char *command_str)
{
  internal_error (__FILE__, __LINE__, _("tui_exec called"));
}


/* Factory for TUI interpreters.  */

static struct interp *
tui_interp_factory (const char *name)
{
  return new tui_interp (name);
}

void _initialize_tui_interp ();
void
_initialize_tui_interp ()
{
  interp_factory_register (INTERP_TUI, tui_interp_factory);

  if (interpreter_p && strcmp (interpreter_p, INTERP_TUI) == 0)
    tui_start_enabled = true;

  if (interpreter_p && strcmp (interpreter_p, INTERP_CONSOLE) == 0)
    {
      xfree (interpreter_p);
      interpreter_p = xstrdup (INTERP_TUI);
    }

  /* If changing this, remember to update cli-interp.c as well.  */
  gdb::observers::normal_stop.attach (tui_on_normal_stop);
  gdb::observers::signal_received.attach (tui_on_signal_received);
  gdb::observers::end_stepping_range.attach (tui_on_end_stepping_range);
  gdb::observers::signal_exited.attach (tui_on_signal_exited);
  gdb::observers::exited.attach (tui_on_exited);
  gdb::observers::no_history.attach (tui_on_no_history);
  gdb::observers::sync_execution_done.attach (tui_on_sync_execution_done);
  gdb::observers::command_error.attach (tui_on_command_error);
  gdb::observers::user_selected_context_changed.attach
    (tui_on_user_selected_context_changed);
}
