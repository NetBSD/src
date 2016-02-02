/* TUI Interpreter definitions for GDB, the GNU debugger.

   Copyright (C) 2003-2015 Free Software Foundation, Inc.

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

static struct ui_out *tui_ui_out (struct interp *self);

/* Set to 1 when the TUI mode must be activated when we first start
   gdb.  */
static int tui_start_enabled = 0;

/* The TUI interpreter.  */
static struct interp *tui_interp;

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

/* Observer for the signal_received notification.  */

static void
tui_on_signal_received (enum gdb_signal siggnal)
{
  if (!interp_quiet_p (tui_interp))
    print_signal_received_reason (tui_ui_out (tui_interp), siggnal);
}

/* Observer for the end_stepping_range notification.  */

static void
tui_on_end_stepping_range (void)
{
  if (!interp_quiet_p (tui_interp))
    print_end_stepping_range_reason (tui_ui_out (tui_interp));
}

/* Observer for the signal_exited notification.  */

static void
tui_on_signal_exited (enum gdb_signal siggnal)
{
  if (!interp_quiet_p (tui_interp))
    print_signal_exited_reason (tui_ui_out (tui_interp), siggnal);
}

/* Observer for the exited notification.  */

static void
tui_on_exited (int exitstatus)
{
  if (!interp_quiet_p (tui_interp))
    print_exited_reason (tui_ui_out (tui_interp), exitstatus);
}

/* Observer for the no_history notification.  */

static void
tui_on_no_history (void)
{
  if (!interp_quiet_p (tui_interp))
    print_no_history_reason (tui_ui_out (tui_interp));
}

/* Observer for the sync_execution_done notification.  */

static void
tui_on_sync_execution_done (void)
{
  if (!interp_quiet_p (tui_interp))
    display_gdb_prompt (NULL);
}

/* Observer for the command_error notification.  */

static void
tui_on_command_error (void)
{
  if (!interp_quiet_p (tui_interp))
    display_gdb_prompt (NULL);
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

  /* If changing this, remember to update cli-interp.c as well.  */
  observer_attach_signal_received (tui_on_signal_received);
  observer_attach_end_stepping_range (tui_on_end_stepping_range);
  observer_attach_signal_exited (tui_on_signal_exited);
  observer_attach_exited (tui_on_exited);
  observer_attach_no_history (tui_on_no_history);
  observer_attach_sync_execution_done (tui_on_sync_execution_done);
  observer_attach_command_error (tui_on_command_error);

  return NULL;
}

static int
tui_resume (void *data)
{
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

  gdb_setup_readline ();

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

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_tui_interp;

void
_initialize_tui_interp (void)
{
  static const struct interp_procs procs = {
    tui_init,
    tui_resume,
    tui_suspend,
    tui_exec,
    tui_ui_out,
    NULL,
    cli_command_loop
  };

  /* Create a default uiout builder for the TUI.  */
  tui_interp = interp_new (INTERP_TUI, &procs);
  interp_add (tui_interp);
  if (interpreter_p && strcmp (interpreter_p, INTERP_TUI) == 0)
    tui_start_enabled = 1;

  if (interpreter_p && strcmp (interpreter_p, INTERP_CONSOLE) == 0)
    {
      xfree (interpreter_p);
      interpreter_p = xstrdup (INTERP_TUI);
    }
}
