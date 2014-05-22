/* Manages interpreters for GDB, the GNU debugger.

   Copyright (C) 2000-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "gdb_assert.h"
#include "top.h"		/* For command_loop.  */
#include "exceptions.h"
#include "continuations.h"

/* True if the current interpreter in is async mode.  See interps.h
   for more details.  This starts out disabled, until all the explicit
   command line arguments (e.g., `gdb -ex "start" -ex "next"') are
   processed.  */
int interpreter_async = 0;

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

/* Functions local to this file.  */
static void initialize_interps (void);

/* The magic initialization routine for this module.  */

void _initialize_interpreter (void);

/* Variables local to this file: */

static struct interp *interp_list = NULL;
static struct interp *current_interpreter = NULL;
static struct interp *top_level_interpreter_ptr = NULL;

static int interpreter_initialized = 0;

/* interp_new - This allocates space for a new interpreter,
   fills the fields from the inputs, and returns a pointer to the
   interpreter.  */
struct interp *
interp_new (const char *name, const struct interp_procs *procs)
{
  struct interp *new_interp;

  new_interp = XMALLOC (struct interp);

  new_interp->name = xstrdup (name);
  new_interp->data = NULL;
  new_interp->quiet_p = 0;
  new_interp->procs = procs;
  new_interp->inited = 0;

  return new_interp;
}

/* Add interpreter INTERP to the gdb interpreter list.  The
   interpreter must not have previously been added.  */
void
interp_add (struct interp *interp)
{
  if (!interpreter_initialized)
    initialize_interps ();

  gdb_assert (interp_lookup (interp->name) == NULL);

  interp->next = interp_list;
  interp_list = interp;
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
  struct interp *old_interp = current_interpreter;
  int first_time = 0;
  char buffer[64];

  /* If we already have an interpreter, then trying to
     set top level interpreter is kinda pointless.  */
  gdb_assert (!top_level || !current_interpreter);
  gdb_assert (!top_level || !top_level_interpreter_ptr);

  if (current_interpreter != NULL)
    {
      ui_out_flush (current_uiout);
      if (current_interpreter->procs->suspend_proc
	  && !current_interpreter->procs->suspend_proc (current_interpreter->
							data))
	{
	  error (_("Could not suspend interpreter \"%s\"."),
		 current_interpreter->name);
	}
    }
  else
    {
      first_time = 1;
    }

  current_interpreter = interp;
  if (top_level)
    top_level_interpreter_ptr = interp;

  /* We use interpreter_p for the "set interpreter" variable, so we need
     to make sure we have a malloc'ed copy for the set command to free.  */
  if (interpreter_p != NULL
      && strcmp (current_interpreter->name, interpreter_p) != 0)
    {
      xfree (interpreter_p);

      interpreter_p = xstrdup (current_interpreter->name);
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

  /* Finally, put up the new prompt to show that we are indeed here. 
     Also, display_gdb_prompt for the console does some readline magic
     which is needed for the console interpreter, at least...  */

  if (!first_time)
    {
      if (!interp_quiet_p (interp))
	{
	  xsnprintf (buffer, sizeof (buffer),
		     "Switching to interpreter \"%.24s\".\n", interp->name);
	  ui_out_text (current_uiout, buffer);
	}
      display_gdb_prompt (NULL);
    }

  return 1;
}

/* interp_lookup - Looks up the interpreter for NAME.  If no such
   interpreter exists, return NULL, otherwise return a pointer to the
   interpreter.  */
struct interp *
interp_lookup (const char *name)
{
  struct interp *interp;

  if (name == NULL || strlen (name) == 0)
    return NULL;

  for (interp = interp_list; interp != NULL; interp = interp->next)
    {
      if (strcmp (interp->name, name) == 0)
	return interp;
    }

  return NULL;
}

/* Returns the current interpreter.  */

struct ui_out *
interp_ui_out (struct interp *interp)
{
  if (interp != NULL)
    return interp->procs->ui_out_proc (interp);

  return current_interpreter->procs->ui_out_proc (current_interpreter);
}

int
current_interp_set_logging (int start_log, struct ui_file *out,
			    struct ui_file *logfile)
{
  if (current_interpreter == NULL
      || current_interpreter->procs->set_logging_proc == NULL)
    return 0;

  return current_interpreter->procs->set_logging_proc (current_interpreter,
						       start_log, out,
						       logfile);
}

/* Temporarily overrides the current interpreter.  */
struct interp *
interp_set_temp (const char *name)
{
  struct interp *interp = interp_lookup (name);
  struct interp *old_interp = current_interpreter;

  if (interp)
    current_interpreter = interp;
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
  if (current_interpreter)
    return (strcmp (current_interpreter->name, interp_name) == 0);

  return 0;
}

/* This is called in display_gdb_prompt.  If the proc returns a zero
   value, display_gdb_prompt will return without displaying the
   prompt.  */
int
current_interp_display_prompt_p (void)
{
  if (current_interpreter == NULL
      || current_interpreter->procs->prompt_proc_p == NULL)
    return 0;
  else
    return current_interpreter->procs->prompt_proc_p (current_interpreter->
						      data);
}

/* Run the current command interpreter's main loop.  */
void
current_interp_command_loop (void)
{
  /* Somewhat messy.  For the moment prop up all the old ways of
     selecting the command loop.  `deprecated_command_loop_hook'
     should be deprecated.  */
  if (deprecated_command_loop_hook != NULL)
    deprecated_command_loop_hook ();
  else if (current_interpreter != NULL
	   && current_interpreter->procs->command_loop_proc != NULL)
    current_interpreter->procs->command_loop_proc (current_interpreter->data);
  else
    cli_command_loop ();
}

int
interp_quiet_p (struct interp *interp)
{
  if (interp != NULL)
    return interp->quiet_p;
  else
    return current_interpreter->quiet_p;
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
int
interp_exec_p (struct interp *interp)
{
  return interp->procs->exec_proc != NULL;
}

struct gdb_exception
interp_exec (struct interp *interp, const char *command_str)
{
  if (interp->procs->exec_proc != NULL)
    {
      return interp->procs->exec_proc (interp->data, command_str);
    }
  return exception_none;
}

/* A convenience routine that nulls out all the common command hooks.
   Use it when removing your interpreter in its suspend proc.  */
void
clear_interpreter_hooks (void)
{
  deprecated_init_ui_hook = 0;
  deprecated_print_frame_info_listing_hook = 0;
  /*print_frame_more_info_hook = 0; */
  deprecated_query_hook = 0;
  deprecated_warning_hook = 0;
  deprecated_interactive_hook = 0;
  deprecated_readline_begin_hook = 0;
  deprecated_readline_hook = 0;
  deprecated_readline_end_hook = 0;
  deprecated_register_changed_hook = 0;
  deprecated_context_hook = 0;
  deprecated_target_wait_hook = 0;
  deprecated_call_command_hook = 0;
  deprecated_error_begin_hook = 0;
  deprecated_command_loop_hook = 0;
}

/* This is a lazy init routine, called the first time the interpreter
   module is used.  I put it here just in case, but I haven't thought
   of a use for it yet.  I will probably bag it soon, since I don't
   think it will be necessary.  */
static void
initialize_interps (void)
{
  interpreter_initialized = 1;
  /* Don't know if anything needs to be done here...  */
}

static void
interpreter_exec_cmd (char *args, int from_tty)
{
  struct interp *old_interp, *interp_to_use;
  char **prules = NULL;
  char **trule = NULL;
  unsigned int nrules;
  unsigned int i;
  int old_quiet, use_quiet;

  if (args == NULL)
    error_no_arg (_("interpreter-exec command"));

  prules = gdb_buildargv (args);
  make_cleanup_freeargv (prules);

  nrules = 0;
  for (trule = prules; *trule != NULL; trule++)
    nrules++;

  if (nrules < 2)
    error (_("usage: interpreter-exec <interpreter> [ <command> ... ]"));

  old_interp = current_interpreter;

  interp_to_use = interp_lookup (prules[0]);
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
}

/* List the possible interpreters which could complete the given text.  */
static VEC (char_ptr) *
interpreter_completer (struct cmd_list_element *ignore, char *text, char *word)
{
  int textlen;
  VEC (char_ptr) *matches = NULL;
  struct interp *interp;

  textlen = strlen (text);
  for (interp = interp_list; interp != NULL; interp = interp->next)
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
  return top_level_interpreter_ptr;  
}

void *
top_level_interpreter_data (void)
{
  gdb_assert (top_level_interpreter_ptr);
  return top_level_interpreter_ptr->data;  
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
