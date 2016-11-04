/* TUI Interpreter definitions for GDB, the GNU debugger.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

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
#include "event-loop.h"
#include "ui-out.h"
#include "cli-out.h"
#include "tui/tui-data.h"
#include "readline/readline.h"
#include "tui/tui-win.h"
#include "tui/tui.h"
#include "tui/tui-io.h"
#include "infrun.h"
#include "observer.h"
#include "gdbthread.h"

static struct ui_out *tui_ui_out (struct interp *self);

/* Set to 1 when the TUI mode must be activated when we first start
   gdb.  */
static int tui_start_enabled = 0;

/* Returns the INTERP if the INTERP is a TUI, and returns NULL
   otherwise.  */

static struct interp *
as_tui_interp (struct interp *interp)
{
  if (strcmp (interp_name (interp), INTERP_TUI) == 0)
    return interp;
  return NULL;
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
  struct switch_thru_all_uis state;

  if (!print_frame)
    return;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *interp = top_level_interpreter ();
      struct interp *tui = as_tui_interp (interp);
      struct thread_info *thread;

      if (tui == NULL)
	continue;

      thread = inferior_thread ();
      if (should_print_stop_to_console (interp, thread))
	print_stop_event (tui_ui_out (tui));
    }
}

/* Observer for the signal_received notification.  */

static void
tui_on_signal_received (enum gdb_signal siggnal)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_signal_received_reason (tui_ui_out (tui), siggnal);
    }
}

/* Observer for the end_stepping_range notification.  */

static void
tui_on_end_stepping_range (void)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_end_stepping_range_reason (tui_ui_out (tui));
    }
}

/* Observer for the signal_exited notification.  */

static void
tui_on_signal_exited (enum gdb_signal siggnal)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_signal_exited_reason (tui_ui_out (tui), siggnal);
    }
}

/* Observer for the exited notification.  */

static void
tui_on_exited (int exitstatus)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_exited_reason (tui_ui_out (tui), exitstatus);
    }
}

/* Observer for the no_history notification.  */

static void
tui_on_no_history (void)
{
  struct switch_thru_all_uis state;

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      print_no_history_reason (tui_ui_out (tui));
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
  struct switch_thru_all_uis state;
  struct thread_info *tp;

  /* This event is suppressed.  */
  if (cli_suppress_notification.user_selected_context)
    return;

  tp = find_thread_ptid (inferior_ptid);

  SWITCH_THRU_ALL_UIS (state)
    {
      struct interp *tui = as_tui_interp (top_level_interpreter ());

      if (tui == NULL)
	continue;

      if (selection & USER_SELECTED_INFERIOR)
	print_selected_inferior (tui_ui_out (tui));

      if (tp != NULL
	  && ((selection & (USER_SELECTED_THREAD | USER_SELECTED_FRAME))))
	print_selected_thread_frame (tui_ui_out (tui), selection);

    }
}

/* These implement the TUI interpreter.  */

static void *
tui_init (struct interp *self, int top_level)
{
  /* Install exit handler to leave the screen in a good shape.  */
  atexit (tui_exit);

  tui_initialize_static_data ();

  tui_initialize_io ();
  tui_initialize_win ();
  if (ui_file_isatty (gdb_stdout))
    tui_initialize_readline ();

  return NULL;
}

static int
tui_resume (void *data)
{
  struct ui *ui = current_ui;
  struct ui_file *stream;

  /* gdb_setup_readline will change gdb_stdout.  If the TUI was
     previously writing to gdb_stdout, then set it to the new
     gdb_stdout afterwards.  */

  stream = cli_out_set_stream (tui_old_uiout, gdb_stdout);
  if (stream != gdb_stdout)
    {
      cli_out_set_stream (tui_old_uiout, stream);
      stream = NULL;
    }

  gdb_setup_readline (1);

  ui->input_handler = command_line_handler;

  if (stream != NULL)
    cli_out_set_stream (tui_old_uiout, gdb_stdout);

  if (tui_start_enabled)
    tui_enable ();
  return 1;
}

static int
tui_suspend (void *data)
{
  tui_start_enabled = tui_active;
  tui_disable ();
  return 1;
}

static struct ui_out *
tui_ui_out (struct interp *self)
{
  if (tui_active)
    return tui_out;
  else
    return tui_old_uiout;
}

static struct gdb_exception
tui_exec (void *data, const char *command_str)
{
  internal_error (__FILE__, __LINE__, _("tui_exec called"));
}

/* The TUI interpreter's vtable.  */

static const struct interp_procs tui_interp_procs = {
  tui_init,
  tui_resume,
  tui_suspend,
  tui_exec,
  tui_ui_out,
  NULL,
  cli_interpreter_pre_command_loop,
  cli_interpreter_supports_command_editing,
};

/* Factory for TUI interpreters.  */

static struct interp *
tui_interp_factory (const char *name)
{
  return interp_new (name, &tui_interp_procs, NULL);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_tui_interp;

void
_initialize_tui_interp (void)
{
  interp_factory_register (INTERP_TUI, tui_interp_factory);

  if (interpreter_p && strcmp (interpreter_p, INTERP_TUI) == 0)
    tui_start_enabled = 1;

  if (interpreter_p && strcmp (interpreter_p, INTERP_CONSOLE) == 0)
    {
      xfree (interpreter_p);
      interpreter_p = xstrdup (INTERP_TUI);
    }

  /* If changing this, remember to update cli-interp.c as well.  */
  observer_attach_normal_stop (tui_on_normal_stop);
  observer_attach_signal_received (tui_on_signal_received);
  observer_attach_end_stepping_range (tui_on_end_stepping_range);
  observer_attach_signal_exited (tui_on_signal_exited);
  observer_attach_exited (tui_on_exited);
  observer_attach_no_history (tui_on_no_history);
  observer_attach_sync_execution_done (tui_on_sync_execution_done);
  observer_attach_command_error (tui_on_command_error);
  observer_attach_user_selected_context_changed
    (tui_on_user_selected_context_changed);
}
