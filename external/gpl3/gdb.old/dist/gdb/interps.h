/* Manages interpreters for GDB, the GNU debugger.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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

class interp
{
public:
  explicit interp (const char *name);
  virtual ~interp () = 0;

  virtual void init (bool top_level)
  {}

  virtual void resume () = 0;
  virtual void suspend () = 0;

  virtual gdb_exception exec (const char *command) = 0;

  /* Returns the ui_out currently used to collect results for this
     interpreter.  It can be a formatter for stdout, as is the case
     for the console & mi outputs, or it might be a result
     formatter.  */
  virtual ui_out *interp_ui_out () = 0;

  /* Provides a hook for interpreters to do any additional
     setup/cleanup that they might need when logging is enabled or
     disabled.  */
  virtual void set_logging (ui_file_up logfile, bool logging_redirect) = 0;

  /* Called before starting an event loop, to give the interpreter a
     chance to e.g., print a prompt.  */
  virtual void pre_command_loop ()
  {}

  /* Returns true if this interpreter supports using the readline
     library; false if it uses GDB's own simplified readline
     emulation.  */
  virtual bool supports_command_editing ()
  { return false; }

  /* This is the name in "-i=" and "set interpreter".  */
  const char *name;

  /* Interpreters are stored in a linked list, this is the next
     one...  */
  struct interp *next;

  /* Has the init method been run?  */
  bool inited;
};

extern void interp_add (struct ui *ui, struct interp *interp);

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
extern const char *interp_name (struct interp *interp);
extern struct interp *interp_set_temp (const char *name);

extern int current_interp_named_p (const char *name);

/* Call this function to give the current interpreter an opportunity
   to do any special handling of streams when logging is enabled or
   disabled.  LOGFILE is the stream for the log file when logging is
   starting and is NULL when logging is ending.  LOGGING_REDIRECT is
   the value of the "set logging redirect" setting.  If true, the
   interpreter should configure the output streams to send output only
   to the logfile.  If false, the interpreter should configure the
   output streams to send output to both the current output stream
   (i.e., the terminal) and the log file.  */
extern void current_interp_set_logging (ui_file_up logfile,
					bool logging_redirect);

/* Returns the top-level interpreter.  */
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
