/* Manages interpreters for GDB, the GNU debugger.

   Copyright (C) 2000-2016 Free Software Foundation, Inc.

   Written by Jim Ingham <jingham@apple.com> of Apple Computer, Inc.

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

/* This is just a first cut at separating out the "interpreter"
   functions of gdb into self-contained modules.  There are a couple
   of open areas that need to be sorted out:

   1) The interpreter explicitly contains a UI_OUT, and can insert itself
   into the event loop, but it doesn't explicitly contain hooks for readline.
   I did this because it seems to me many interpreters won't want to use
   the readline command interface, and it is probably simpler to just let
   them take over the input in their resume proc.  */

#include "defs.h"
#include "gdbcmd.h"
#include "ui-out.h"
#include "event-loop.h"
#include "event-top.h"
#include "interps.h"
#include "completer.h"
#include "top.h"		/* For command_loop.  */
#include "continuations.h"

/* Each UI has its own independent set of interpreters.  */

struct ui_interp_info
{
  /* Each top level has its own independent set of interpreters.  */
  struct interp *interp_list;
  struct interp *current_interpreter;
  struct interp *top_level_interpreter;

  /* The interpreter that is active while `interp_exec' is active, NULL
     at all other times.  */
  struct interp *command_interpreter;
};

/* Get UI's ui_interp_info object.  Never returns NULL.  */

static struct ui_interp_info *
get_interp_info (struct ui *ui)
{
  if (ui->interp_info == NULL)
    ui->interp_info = XCNEW (struct ui_interp_info);
  return ui->interp_info;
}

/* Get the current UI's ui_interp_info object.  Never returns
   NULL.  */

static struct ui_interp_info *
get_current_interp_info (void)
{
  return get_interp_info (current_ui);
}

struct interp
{
  /* This is the name in "-i=" and set interpreter.  */
  const char *name;

  /* Interpreters are stored in a linked list, this is the next
     one...  */
  struct interp *next;

  /* This is a cookie that an instance of the interpreter can use.
     This is a bit confused right now as the exact initialization
     sequence for it, and how it relates to the interpreter's uiout
     object is a bit confused.  */
  void *data;

  /* Has the init_proc been run?  */
  int inited;

  const struct interp_procs *procs;
  int quiet_p;
};

/* The magic initialization routine for this module.  */

void _initialize_interpreter (void);

static struct interp *interp_lookup_existing (struct ui *ui,
					      const char *name);

/* interp_new - This allocates space for a new interpreter,
   fills the fields from the inputs, and returns a pointer to the
   interpreter.  */
struct interp *
interp_new (const char *name, const struct interp_procs *procs, void *data)
{
  struct interp *new_interp;

  new_interp = XNEW (struct interp);

  new_interp->name = xstrdup (name);
  new_interp->data = data;
  new_interp->quiet_p = 0;
  new_interp->procs = procs;
  new_interp->inited = 0;

  return new_interp;
}

/* An interpreter factory.  Maps an interpreter name to the factory
   function that instantiates an interpreter by that name.  */

struct interp_factory
{
  /* This is the name in "-i=INTERP" and "interpreter-exec INTERP".  */
  const char *name;

  /* The function that creates the interpreter.  */
  interp_factory_func func;
};

typedef struct interp_factory *interp_factory_p;
DEF_VEC_P(interp_factory_p);

/* The registered interpreter factories.  */
static VEC(interp_factory_p) *interpreter_factories = NULL;

/* See interps.h.  */

void
interp_factory_register (const char *name, interp_factory_func func)
{
  struct interp_factory *f;
  int ix;

  /* Assert that no factory for NAME is already registered.  */
  for (ix = 0;
       VEC_iterate (interp_factory_p, interpreter_factories, ix, f);
       ++ix)
    if (strcmp (f->name, name) == 0)
      {
	internal_error (__FILE__, __LINE__,
			_("interpreter factory already registered: \"%s\"\n"),
			name);
      }

  f = XNEW (struct interp_factory);
  f->name = name;
  f->func = func;
  VEC_safe_push (interp_factory_p, interpreter_factories, f);
}

/* Add interpreter INTERP to the gdb interpreter list.  The
   interpreter must not have previously been added.  */
void
interp_add (struct ui *ui, struct interp *interp)
{
  struct ui_interp_info *ui_interp = get_interp_info (ui);

  gdb_assert (interp_lookup_existing (ui, interp->name) == NULL);

  interp->next = ui_interp->interp_list;
  ui_interp->interp_list = interp;
}

/* This sets the current interpreter to be INTERP.  If INTERP has not
   been initialized, then this will also run the init proc.  If the
   init proc is successful, return 1, if it fails, set the old
   interpreter back in place and return 0.  If we can't restore the
   old interpreter, then raise an internal error, since we are in
   pretty bad shape at this point.

   The TOP_LEVEL parameter tells if this new interpreter is
   the top-level one.  The top-level is what is requested
   on the command line, and is responsible for reporting general
   notification about target state changes.  For example, if
   MI is the top-level interpreter, then it will always report
   events such as target stops and new thread creation, even if they
   are caused by CLI commands.  */
int
interp_set (struct interp *interp, int top_level)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *old_interp = ui_interp->current_interpreter;
  int first_time = 0;
  char buffer[64];

  /* If we already have an interpreter, then trying to
     set top level interpreter is kinda pointless.  */
  gdb_assert (!top_level || !ui_interp->current_interpreter);
  gdb_assert (!top_level || !ui_interp->top_level_interpreter);

  if (old_interp != NULL)
    {
      ui_out_flush (current_uiout);
      if (old_interp->procs->suspend_proc
	  && !old_interp->procs->suspend_proc (old_interp->data))
	{
	  error (_("Could not suspend interpreter \"%s\"."),
		 old_interp->name);
	}
    }
  else
    {
      first_time = 1;
    }

  ui_interp->current_interpreter = interp;
  if (top_level)
    ui_interp->top_level_interpreter = interp;

  /* We use interpreter_p for the "set interpreter" variable, so we need
     to make sure we have a malloc'ed copy for the set command to free.  */
  if (interpreter_p != NULL
      && strcmp (interp->name, interpreter_p) != 0)
    {
      xfree (interpreter_p);

      interpreter_p = xstrdup (interp->name);
    }

  /* Run the init proc.  If it fails, try to restore the old interp.  */

  if (!interp->inited)
    {
      if (interp->procs->init_proc != NULL)
	{
	  interp->data = interp->procs->init_proc (interp, top_level);
	}
      interp->inited = 1;
    }

  /* Do this only after the interpreter is initialized.  */
  current_uiout = interp->procs->ui_out_proc (interp);

  /* Clear out any installed interpreter hooks/event handlers.  */
  clear_interpreter_hooks ();

  if (interp->procs->resume_proc != NULL
      && (!interp->procs->resume_proc (interp->data)))
    {
      if (old_interp == NULL || !interp_set (old_interp, 0))
	internal_error (__FILE__, __LINE__,
			_("Failed to initialize new interp \"%s\" %s"),
			interp->name, "and could not restore old interp!\n");
      return 0;
    }

  if (!first_time && !interp_quiet_p (interp))
    {
      xsnprintf (buffer, sizeof (buffer),
		 "Switching to interpreter \"%.24s\".\n", interp->name);
      ui_out_text (current_uiout, buffer);
    }

  return 1;
}

/* Look up the interpreter for NAME.  If no such interpreter exists,
   return NULL, otherwise return a pointer to the interpreter.  */

static struct interp *
interp_lookup_existing (struct ui *ui, const char *name)
{
  struct ui_interp_info *ui_interp = get_interp_info (ui);
  struct interp *interp;

  for (interp = ui_interp->interp_list;
       interp != NULL;
       interp = interp->next)
    {
      if (strcmp (interp->name, name) == 0)
	return interp;
    }

  return NULL;
}

/* See interps.h.  */

struct interp *
interp_lookup (struct ui *ui, const char *name)
{
  struct interp_factory *factory;
  struct interp *interp;
  int ix;

  if (name == NULL || strlen (name) == 0)
    return NULL;

  /* Only create each interpreter once per top level.  */
  interp = interp_lookup_existing (ui, name);
  if (interp != NULL)
    return interp;

  for (ix = 0;
       VEC_iterate (interp_factory_p, interpreter_factories, ix, factory);
       ++ix)
    if (strcmp (factory->name, name) == 0)
      {
	interp = factory->func (name);
	interp_add (ui, interp);
	return interp;
      }

  return NULL;
}

/* See interps.h.  */

void
set_top_level_interpreter (const char *name)
{
  /* Find it.  */
  struct interp *interp = interp_lookup (current_ui, name);

  if (interp == NULL)
    error (_("Interpreter `%s' unrecognized"), name);
  /* Install it.  */
  if (!interp_set (interp, 1))
    error (_("Interpreter `%s' failed to initialize."), name);
}

/* Returns the current interpreter.  */

struct ui_out *
interp_ui_out (struct interp *interp)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  if (interp == NULL)
    interp = ui_interp->current_interpreter;
  return interp->procs->ui_out_proc (interp);
}

int
current_interp_set_logging (int start_log, struct ui_file *out,
			    struct ui_file *logfile)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *interp = ui_interp->current_interpreter;

  if (interp == NULL
      || interp->procs->set_logging_proc == NULL)
    return 0;

  return interp->procs->set_logging_proc (interp, start_log, out, logfile);
}

/* Temporarily overrides the current interpreter.  */
struct interp *
interp_set_temp (const char *name)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *interp = interp_lookup (current_ui, name);
  struct interp *old_interp = ui_interp->current_interpreter;

  if (interp)
    ui_interp->current_interpreter = interp;
  return old_interp;
}

/* Returns the interpreter's cookie.  */

void *
interp_data (struct interp *interp)
{
  return interp->data;
}

/* Returns the interpreter's name.  */

const char *
interp_name (struct interp *interp)
{
  return interp->name;
}

/* Returns true if the current interp is the passed in name.  */
int
current_interp_named_p (const char *interp_name)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *interp = ui_interp->current_interpreter;

  if (interp != NULL)
    return (strcmp (interp->name, interp_name) == 0);

  return 0;
}

/* The interpreter that was active when a command was executed.
   Normally that'd always be CURRENT_INTERPRETER, except that MI's
   -interpreter-exec command doesn't actually flip the current
   interpreter when running its sub-command.  The
   `command_interpreter' global tracks when interp_exec is called
   (IOW, when -interpreter-exec is called).  If that is set, it is
   INTERP in '-interpreter-exec INTERP "CMD"' or in 'interpreter-exec
   INTERP "CMD".  Otherwise, interp_exec isn't active, and so the
   interpreter running the command is the current interpreter.  */

struct interp *
command_interp (void)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  if (ui_interp->command_interpreter != NULL)
    return ui_interp->command_interpreter;
  else
    return ui_interp->current_interpreter;
}

/* See interps.h.  */

void
interp_pre_command_loop (struct interp *interp)
{
  gdb_assert (interp != NULL);

  if (interp->procs->pre_command_loop_proc != NULL)
    interp->procs->pre_command_loop_proc (interp);
}

/* See interp.h  */

int
interp_supports_command_editing (struct interp *interp)
{
  if (interp->procs->supports_command_editing_proc != NULL)
    return interp->procs->supports_command_editing_proc (interp);
  return 0;
}

int
interp_quiet_p (struct interp *interp)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  if (interp != NULL)
    return interp->quiet_p;
  else
    return ui_interp->current_interpreter->quiet_p;
}

static int
interp_set_quiet (struct interp *interp, int quiet)
{
  int old_val = interp->quiet_p;

  interp->quiet_p = quiet;
  return old_val;
}

/* interp_exec - This executes COMMAND_STR in the current 
   interpreter.  */

struct gdb_exception
interp_exec (struct interp *interp, const char *command_str)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  struct gdb_exception ex;
  struct interp *save_command_interp;

  gdb_assert (interp->procs->exec_proc != NULL);

  /* See `command_interp' for why we do this.  */
  save_command_interp = ui_interp->command_interpreter;
  ui_interp->command_interpreter = interp;

  ex = interp->procs->exec_proc (interp->data, command_str);

  ui_interp->command_interpreter = save_command_interp;

  return ex;
}

/* A convenience routine that nulls out all the common command hooks.
   Use it when removing your interpreter in its suspend proc.  */
void
clear_interpreter_hooks (void)
{
  deprecated_print_frame_info_listing_hook = 0;
  /*print_frame_more_info_hook = 0; */
  deprecated_query_hook = 0;
  deprecated_warning_hook = 0;
  deprecated_interactive_hook = 0;
  deprecated_readline_begin_hook = 0;
  deprecated_readline_hook = 0;
  deprecated_readline_end_hook = 0;
  deprecated_context_hook = 0;
  deprecated_target_wait_hook = 0;
  deprecated_call_command_hook = 0;
  deprecated_error_begin_hook = 0;
}

static void
interpreter_exec_cmd (char *args, int from_tty)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *old_interp, *interp_to_use;
  char **prules = NULL;
  char **trule = NULL;
  unsigned int nrules;
  unsigned int i;
  int old_quiet, use_quiet;
  struct cleanup *cleanup;

  if (args == NULL)
    error_no_arg (_("interpreter-exec command"));

  prules = gdb_buildargv (args);
  cleanup = make_cleanup_freeargv (prules);

  nrules = 0;
  for (trule = prules; *trule != NULL; trule++)
    nrules++;

  if (nrules < 2)
    error (_("usage: interpreter-exec <interpreter> [ <command> ... ]"));

  old_interp = ui_interp->current_interpreter;

  interp_to_use = interp_lookup (current_ui, prules[0]);
  if (interp_to_use == NULL)
    error (_("Could not find interpreter \"%s\"."), prules[0]);

  /* Temporarily set interpreters quiet.  */
  old_quiet = interp_set_quiet (old_interp, 1);
  use_quiet = interp_set_quiet (interp_to_use, 1);

  if (!interp_set (interp_to_use, 0))
    error (_("Could not switch to interpreter \"%s\"."), prules[0]);

  for (i = 1; i < nrules; i++)
    {
      struct gdb_exception e = interp_exec (interp_to_use, prules[i]);

      if (e.reason < 0)
	{
	  interp_set (old_interp, 0);
	  interp_set_quiet (interp_to_use, use_quiet);
	  interp_set_quiet (old_interp, old_quiet);
	  error (_("error in command: \"%s\"."), prules[i]);
	}
    }

  interp_set (old_interp, 0);
  interp_set_quiet (interp_to_use, use_quiet);
  interp_set_quiet (old_interp, old_quiet);

  do_cleanups (cleanup);
}

/* See interps.h.  */

VEC (char_ptr) *
interpreter_completer (struct cmd_list_element *ignore,
		       const char *text, const char *word)
{
  struct interp_factory *interp;
  int textlen;
  VEC (char_ptr) *matches = NULL;
  int ix;

  textlen = strlen (text);
  for (ix = 0;
       VEC_iterate (interp_factory_p, interpreter_factories, ix, interp);
       ++ix)
    {
      if (strncmp (interp->name, text, textlen) == 0)
	{
	  char *match;

	  match = (char *) xmalloc (strlen (word) + strlen (interp->name) + 1);
	  if (word == text)
	    strcpy (match, interp->name);
	  else if (word > text)
	    {
	      /* Return some portion of interp->name.  */
	      strcpy (match, interp->name + (word - text));
	    }
	  else
	    {
	      /* Return some of text plus interp->name.  */
	      strncpy (match, word, text - word);
	      match[text - word] = '\0';
	      strcat (match, interp->name);
	    }
	  VEC_safe_push (char_ptr, matches, match);
	}
    }

  return matches;
}

struct interp *
top_level_interpreter (void)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  return ui_interp->top_level_interpreter;
}

void *
top_level_interpreter_data (void)
{
  struct interp *interp;

  interp = top_level_interpreter ();
  gdb_assert (interp != NULL);
  return interp->data;
}

/* See interps.h.  */

struct interp *
current_interpreter (void)
{
  struct ui_interp_info *ui_interp = get_interp_info (current_ui);

  return ui_interp->current_interpreter;
}

/* This just adds the "interpreter-exec" command.  */
void
_initialize_interpreter (void)
{
  struct cmd_list_element *c;

  c = add_cmd ("interpreter-exec", class_support,
	       interpreter_exec_cmd, _("\
Execute a command in an interpreter.  It takes two arguments:\n\
The first argument is the name of the interpreter to use.\n\
The second argument is the command to execute.\n"), &cmdlist);
  set_cmd_completer (c, interpreter_completer);
}
