/* Manages interpreters for GDB, the GNU debugger.

   Copyright (C) 2000-2023 Free Software Foundation, Inc.

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
class completion_tracker;

typedef struct interp *(*interp_factory_func) (const char *name);

/* Each interpreter kind (CLI, MI, etc.) registers itself with a call
   to this function, passing along its name, and a pointer to a
   function that creates a new instance of an interpreter with that
   name.  */
extern void interp_factory_register (const char *name,
				     interp_factory_func func);

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
  virtual void set_logging (ui_file_up logfile, bool logging_redirect,
			    bool debug_redirect) = 0;

  /* Called before starting an event loop, to give the interpreter a
     chance to e.g., print a prompt.  */
  virtual void pre_command_loop ()
  {}

  /* Returns true if this interpreter supports using the readline
     library; false if it uses GDB's own simplified readline
     emulation.  */
  virtual bool supports_command_editing ()
  { return false; }

  const char *name () const
  {
    return m_name.get ();
  }

private:
  /* This is the name in "-i=" and "set interpreter".  */
  gdb::unique_xmalloc_ptr<char> m_name;

public:
  /* Interpreters are stored in a linked list, this is the next
     one...  */
  struct interp *next;

  /* Has the init method been run?  */
  bool inited = false;
};

/* Look up the interpreter for NAME, creating one if none exists yet.
   If NAME is not a interpreter type previously registered with
   interp_factory_register, return NULL; otherwise return a pointer to
   the interpreter.  */
extern struct interp *interp_lookup (struct ui *ui, const char *name);

/* Set the current UI's top level interpreter to the interpreter named
   NAME.  Throws an error if NAME is not a known interpreter or the
   interpreter fails to initialize.  */
extern void set_top_level_interpreter (const char *name);

/* Temporarily set the current interpreter, and reset it on
   destruction.  */
class scoped_restore_interp
{
public:

  scoped_restore_interp (const char *name)
    : m_interp (set_interp (name))
  {
  }

  ~scoped_restore_interp ()
  {
    set_interp (m_interp->name ());
  }

  scoped_restore_interp (const scoped_restore_interp &) = delete;
  scoped_restore_interp &operator= (const scoped_restore_interp &) = delete;

private:

  struct interp *set_interp (const char *name);

  struct interp *m_interp;
};

extern int current_interp_named_p (const char *name);

/* Call this function to give the current interpreter an opportunity
   to do any special handling of streams when logging is enabled or
   disabled.  LOGFILE is the stream for the log file when logging is
   starting and is NULL when logging is ending.  LOGGING_REDIRECT is
   the value of the "set logging redirect" setting.  If true, the
   interpreter should configure the output streams to send output only
   to the logfile.  If false, the interpreter should configure the
   output streams to send output to both the current output stream
   (i.e., the terminal) and the log file.  DEBUG_REDIRECT is same as
   LOGGING_REDIRECT, but for the value of "set logging debugredirect"
   instead.  */
extern void current_interp_set_logging (ui_file_up logfile,
					bool logging_redirect,
					bool debug_redirect);

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
extern void interpreter_completer (struct cmd_list_element *ignore,
				   completion_tracker &tracker,
				   const char *text,
				   const char *word);

/* well-known interpreters */
#define INTERP_CONSOLE		"console"
#define INTERP_MI1             "mi1"
#define INTERP_MI2             "mi2"
#define INTERP_MI3             "mi3"
#define INTERP_MI4             "mi4"
#define INTERP_MI		"mi"
#define INTERP_TUI		"tui"
#define INTERP_INSIGHT		"insight"

#endif
