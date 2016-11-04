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

#ifndef INTERPS_H
#define INTERPS_H

struct ui_out;
struct interp;
struct ui;

typedef struct interp *(*interp_factory_func) (const char *name);

/* Each interpreter kind (CLI, MI, etc.) registers itself with a call
   to this function, passing along its name, and a pointer to a
   function that creates a new instance of an interpreter with that
   name.  */
extern void interp_factory_register (const char *name,
				     interp_factory_func func);

extern int interp_resume (struct interp *interp);
extern int interp_suspend (struct interp *interp);
extern struct gdb_exception interp_exec (struct interp *interp,
					 const char *command);
extern int interp_quiet_p (struct interp *interp);

typedef void *(interp_init_ftype) (struct interp *self, int top_level);
typedef int (interp_resume_ftype) (void *data);
typedef int (interp_suspend_ftype) (void *data);
typedef struct gdb_exception (interp_exec_ftype) (void *data,
						  const char *command);
typedef void (interp_pre_command_loop_ftype) (struct interp *self);
typedef struct ui_out *(interp_ui_out_ftype) (struct interp *self);

typedef int (interp_set_logging_ftype) (struct interp *self, int start_log,
					struct ui_file *out,
					struct ui_file *logfile);

typedef int (interp_supports_command_editing_ftype) (struct interp *self);

struct interp_procs
{
  interp_init_ftype *init_proc;
  interp_resume_ftype *resume_proc;
  interp_suspend_ftype *suspend_proc;
  interp_exec_ftype *exec_proc;

  /* Returns the ui_out currently used to collect results for this
     interpreter.  It can be a formatter for stdout, as is the case
     for the console & mi outputs, or it might be a result
     formatter.  */
  interp_ui_out_ftype *ui_out_proc;

  /* Provides a hook for interpreters to do any additional
     setup/cleanup that they might need when logging is enabled or
     disabled.  */
  interp_set_logging_ftype *set_logging_proc;

  /* Called before starting an event loop, to give the interpreter a
     chance to e.g., print a prompt.  */
  interp_pre_command_loop_ftype *pre_command_loop_proc;

  /* Returns true if this interpreter supports using the readline
     library; false if it uses GDB's own simplified readline
     emulation.  */
  interp_supports_command_editing_ftype *supports_command_editing_proc;
};

extern struct interp *interp_new (const char *name,
				  const struct interp_procs *procs,
				  void *data);
extern void interp_add (struct ui *ui, struct interp *interp);
extern int interp_set (struct interp *interp, int top_level);

/* Look up the interpreter for NAME, creating one if none exists yet.
   If NAME is not a interpreter type previously registered with
   interp_factory_register, return NULL; otherwise return a pointer to
   the interpreter.  */
extern struct interp *interp_lookup (struct ui *ui, const char *name);

/* Set the current UI's top level interpreter to the interpreter named
   NAME.  Throws an error if NAME is not a known interpreter or the
   interpreter fails to initialize.  */
extern void set_top_level_interpreter (const char *name);

extern struct ui_out *interp_ui_out (struct interp *interp);
extern void *interp_data (struct interp *interp);
extern const char *interp_name (struct interp *interp);
extern struct interp *interp_set_temp (const char *name);

extern int current_interp_named_p (const char *name);

/* Call this function to give the current interpreter an opportunity
   to do any special handling of streams when logging is enabled or
   disabled.  START_LOG is 1 when logging is starting, 0 when it ends,
   and OUT is the stream for the log file; it will be NULL when
   logging is ending.  LOGFILE is non-NULL if the output streams
   are to be tees, with the log file as one of the outputs.  */

extern int current_interp_set_logging (int start_log, struct ui_file *out,
				       struct ui_file *logfile);

/* Returns opaque data associated with the top-level interpreter.  */
extern void *top_level_interpreter_data (void);
extern struct interp *top_level_interpreter (void);

/* Return the current UI's current interpreter.  */
extern struct interp *current_interpreter (void);

extern struct interp *command_interp (void);

extern void clear_interpreter_hooks (void);

/* Returns true if INTERP supports using the readline library; false
   if it uses GDB's own simplified form of readline.  */
extern int interp_supports_command_editing (struct interp *interp);

/* Called before starting an event loop, to give the interpreter a
   chance to e.g., print a prompt.  */
extern void interp_pre_command_loop (struct interp *interp);

/* List the possible interpreters which could complete the given
   text.  */
extern VEC (char_ptr) *interpreter_completer (struct cmd_list_element *ignore,
					      const char *text,
					      const char *word);

/* well-known interpreters */
#define INTERP_CONSOLE		"console"
#define INTERP_MI1             "mi1"
#define INTERP_MI2             "mi2"
#define INTERP_MI3             "mi3"
#define INTERP_MI		"mi"
#define INTERP_TUI		"tui"
#define INTERP_INSIGHT		"insight"

#endif
