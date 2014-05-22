/* CLI Definitions for GDB, the GNU debugger.

   Copyright (C) 2002-2013 Free Software Foundation, Inc.

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
#include "event-top.h"
#include "ui-out.h"
#include "cli-out.h"
#include "top.h"		/* for "execute_command" */
#include "gdb_string.h"
#include "exceptions.h"

struct ui_out *cli_uiout;

/* These are the ui_out and the interpreter for the console
   interpreter.  */

/* Longjmp-safe wrapper for "execute_command".  */
static struct gdb_exception safe_execute_command (struct ui_out *uiout,
						  char *command, 
						  int from_tty);
/* These implement the cli out interpreter: */

static void *
cli_interpreter_init (struct interp *self, int top_level)
{
  return NULL;
}

static int
cli_interpreter_resume (void *data)
{
  struct ui_file *stream;

  /*sync_execution = 1; */

  /* gdb_setup_readline will change gdb_stdout.  If the CLI was
     previously writing to gdb_stdout, then set it to the new
     gdb_stdout afterwards.  */

  stream = cli_out_set_stream (cli_uiout, gdb_stdout);
  if (stream != gdb_stdout)
    {
      cli_out_set_stream (cli_uiout, stream);
      stream = NULL;
    }

  gdb_setup_readline ();

  if (stream != NULL)
    cli_out_set_stream (cli_uiout, gdb_stdout);

  return 1;
}

static int
cli_interpreter_suspend (void *data)
{
  gdb_disable_readline ();
  return 1;
}

/* Don't display the prompt if we are set quiet.  */
static int
cli_interpreter_display_prompt_p (void *data)
{
  if (interp_quiet_p (NULL))
    return 0;
  else
    return 1;
}

static struct gdb_exception
cli_interpreter_exec (void *data, const char *command_str)
{
  struct ui_file *old_stream;
  struct gdb_exception result;

  /* FIXME: cagney/2003-02-01: Need to const char *propogate
     safe_execute_command.  */
  char *str = strcpy (alloca (strlen (command_str) + 1), command_str);

  /* gdb_stdout could change between the time cli_uiout was
     initialized and now.  Since we're probably using a different
     interpreter which has a new ui_file for gdb_stdout, use that one
     instead of the default.

     It is important that it gets reset everytime, since the user
     could set gdb to use a different interpreter.  */
  old_stream = cli_out_set_stream (cli_uiout, gdb_stdout);
  result = safe_execute_command (cli_uiout, str, 1);
  cli_out_set_stream (cli_uiout, old_stream);
  return result;
}

static struct gdb_exception
safe_execute_command (struct ui_out *command_uiout, char *command, int from_tty)
{
  volatile struct gdb_exception e;
  struct ui_out *saved_uiout;

  /* Save and override the global ``struct ui_out'' builder.  */
  saved_uiout = current_uiout;
  current_uiout = command_uiout;

  TRY_CATCH (e, RETURN_MASK_ALL)
    {
      execute_command (command, from_tty);
    }

  /* Restore the global builder.  */
  current_uiout = saved_uiout;

  /* FIXME: cagney/2005-01-13: This shouldn't be needed.  Instead the
     caller should print the exception.  */
  exception_print (gdb_stderr, e);
  return e;
}

static struct ui_out *
cli_ui_out (struct interp *self)
{
  return cli_uiout;
}

/* Standard gdb initialization hook.  */
extern initialize_file_ftype _initialize_cli_interp; /* -Wmissing-prototypes */

void
_initialize_cli_interp (void)
{
  static const struct interp_procs procs = {
    cli_interpreter_init,	/* init_proc */
    cli_interpreter_resume,	/* resume_proc */
    cli_interpreter_suspend,	/* suspend_proc */
    cli_interpreter_exec,	/* exec_proc */
    cli_interpreter_display_prompt_p,	/* prompt_proc_p */
    cli_ui_out			/* ui_out_proc */
  };
  struct interp *cli_interp;

  /* Create a default uiout builder for the CLI.  */
  cli_uiout = cli_out_new (gdb_stdout);
  cli_interp = interp_new (INTERP_CONSOLE, &procs);

  interp_add (cli_interp);
}
