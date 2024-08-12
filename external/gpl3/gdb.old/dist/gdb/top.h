/* Top level stuff for GDB, the GNU debugger.

   Copyright (C) 1986-2023 Free Software Foundation, Inc.

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

#ifndef TOP_H
#define TOP_H

#include "gdbsupport/buffer.h"
#include "gdbsupport/event-loop.h"
#include "gdbsupport/next-iterator.h"
#include "value.h"

/* Prompt state.  */

enum prompt_state
{
  /* The command line is blocked simulating synchronous execution.
     This is used to implement the foreground execution commands
     ('run', 'continue', etc.).  We won't display the prompt and
     accept further commands until the execution is actually over.  */
  PROMPT_BLOCKED,

  /* The command finished; display the prompt before returning back to
     the top level.  */
  PROMPT_NEEDED,

  /* We've displayed the prompt already, ready for input.  */
  PROMPTED,
};

/* All about a user interface instance.  Each user interface has its
   own I/O files/streams, readline state, its own top level
   interpreter (for the main UI, this is the interpreter specified
   with -i on the command line) and secondary interpreters (for
   interpreter-exec ...), etc.  There's always one UI associated with
   stdin/stdout/stderr, but the user can create secondary UIs, for
   example, to create a separate MI channel on its own stdio
   streams.  */

struct ui
{
  /* Create a new UI.  */
  ui (FILE *instream, FILE *outstream, FILE *errstream);
  ~ui ();

  DISABLE_COPY_AND_ASSIGN (ui);

  /* Pointer to next in singly-linked list.  */
  struct ui *next = nullptr;

  /* Convenient handle (UI number).  Unique across all UIs.  */
  int num;

  /* The UI's command line buffer.  This is to used to accumulate
     input until we have a whole command line.  */
  std::string line_buffer;

  /* The callback used by the event loop whenever an event is detected
     on the UI's input file descriptor.  This function incrementally
     builds a buffer where it accumulates the line read up to the
     point of invocation.  In the special case in which the character
     read is newline, the function invokes the INPUT_HANDLER callback
     (see below).  */
  void (*call_readline) (gdb_client_data) = nullptr;

  /* The function to invoke when a complete line of input is ready for
     processing.  */
  void (*input_handler) (gdb::unique_xmalloc_ptr<char> &&) = nullptr;

  /* True if this UI is using the readline library for command
     editing; false if using GDB's own simple readline emulation, with
     no editing support.  */
  int command_editing = 0;

  /* Each UI has its own independent set of interpreters.  */
  struct ui_interp_info *interp_info = nullptr;

  /* True if the UI is in async mode, false if in sync mode.  If in
     sync mode, a synchronous execution command (e.g, "next") does not
     return until the command is finished.  If in async mode, then
     running a synchronous command returns right after resuming the
     target.  Waiting for the command's completion is later done on
     the top event loop.  For the main UI, this starts out disabled,
     until all the explicit command line arguments (e.g., `gdb -ex
     "start" -ex "next"') are processed.  */
  int async = 0;

  /* The number of nested readline secondary prompts that are
     currently active.  */
  int secondary_prompt_depth = 0;

  /* The UI's stdin.  Set to stdin for the main UI.  */
  FILE *stdin_stream;

  /* stdio stream that command input is being read from.  Set to stdin
     normally.  Set by source_command to the file we are sourcing.
     Set to NULL if we are executing a user-defined command or
     interacting via a GUI.  */
  FILE *instream;
  /* Standard output stream.  */
  FILE *outstream;
  /* Standard error stream.  */
  FILE *errstream;

  /* The file descriptor for the input stream, so that we can register
     it with the event loop.  This can be set to -1 to prevent this
     registration.  */
  int input_fd;

  /* Whether ISATTY returns true on input_fd.  Cached here because
     quit_force needs to know this _after_ input_fd might be
     closed.  */
  bool m_input_interactive_p;

  /* See enum prompt_state's description.  */
  enum prompt_state prompt_state = PROMPT_NEEDED;

  /* The fields below that start with "m_" are "private".  They're
     meant to be accessed through wrapper macros that make them look
     like globals.  */

  /* The ui_file streams.  */
  /* Normal results */
  struct ui_file *m_gdb_stdout;
  /* Input stream */
  struct ui_file *m_gdb_stdin;
  /* Serious error notifications */
  struct ui_file *m_gdb_stderr;
  /* Log/debug/trace messages that should bypass normal stdout/stderr
     filtering.  */
  struct ui_file *m_gdb_stdlog;

  /* The current ui_out.  */
  struct ui_out *m_current_uiout = nullptr;

  /* Register the UI's input file descriptor in the event loop.  */
  void register_file_handler ();

  /* Unregister the UI's input file descriptor from the event loop.  */
  void unregister_file_handler ();

  /* Return true if this UI's input fd is a tty.  */
  bool input_interactive_p () const;
};

/* The main UI.  This is the UI that is bound to stdin/stdout/stderr.
   It always exists and is created automatically when GDB starts
   up.  */
extern struct ui *main_ui;

/* The current UI.  */
extern struct ui *current_ui;

/* The list of all UIs.  */
extern struct ui *ui_list;

/* State for SWITCH_THRU_ALL_UIS.  */
class switch_thru_all_uis
{
public:

  switch_thru_all_uis () : m_iter (ui_list), m_save_ui (&current_ui)
  {
    current_ui = ui_list;
  }

  DISABLE_COPY_AND_ASSIGN (switch_thru_all_uis);

  /* If done iterating, return true; otherwise return false.  */
  bool done () const
  {
    return m_iter == NULL;
  }

  /* Move to the next UI, setting current_ui if iteration is not yet
     complete.  */
  void next ()
  {
    m_iter = m_iter->next;
    if (m_iter != NULL)
      current_ui = m_iter;
  }

 private:

  /* Used to iterate through the UIs.  */
  struct ui *m_iter;

  /* Save and restore current_ui.  */
  scoped_restore_tmpl<struct ui *> m_save_ui;
};

  /* Traverse through all UI, and switch the current UI to the one
     being iterated.  */
#define SWITCH_THRU_ALL_UIS()		\
  for (switch_thru_all_uis stau_state; !stau_state.done (); stau_state.next ())

using ui_range = next_range<ui>;

/* An adapter that can be used to traverse over all UIs.  */
static inline
ui_range all_uis ()
{
  return ui_range (ui_list);
}

/* From top.c.  */
extern bool confirm;
extern int inhibit_gdbinit;

/* Print the GDB version banner to STREAM.  If INTERACTIVE is false,
   then information referring to commands (e.g., "show configuration")
   is omitted; this mode is used for the --version command line
   option.  If INTERACTIVE is true, then interactive commands are
   mentioned.  */
extern void print_gdb_version (struct ui_file *stream, bool interactive);

extern void print_gdb_configuration (struct ui_file *);

extern void read_command_file (FILE *);
extern void init_history (void);
extern void command_loop (void);
extern int quit_confirm (void);
extern void quit_force (int *, int);
extern void quit_command (const char *, int);
extern void quit_cover (void);
extern void execute_command (const char *, int);

/* If the interpreter is in sync mode (we're running a user command's
   list, running command hooks or similars), and we just ran a
   synchronous command that started the target, wait for that command
   to end.  WAS_SYNC indicates whether sync_execution was set before
   the command was run.  */

extern void maybe_wait_sync_command_done (int was_sync);

/* Wait for a synchronous execution command to end.  */
extern void wait_sync_command_done (void);

extern void check_frame_language_change (void);

/* Prepare for execution of a command.
   Call this before every command, CLI or MI.
   Returns a cleanup to be run after the command is completed.  */
extern scoped_value_mark prepare_execute_command (void);

/* This function returns a pointer to the string that is used
   by gdb for its command prompt.  */
extern const std::string &get_prompt ();

/* This function returns a pointer to the string that is used
   by gdb for its command prompt.  */
extern void set_prompt (const char *s);

/* Return 1 if UI's current input handler is a secondary prompt, 0
   otherwise.  */

extern int gdb_in_secondary_prompt_p (struct ui *ui);

/* Perform _initialize initialization.  */
extern void gdb_init ();

/* For use by event-top.c.  */
/* Variables from top.c.  */
extern int source_line_number;
extern std::string source_file_name;
extern bool history_expansion_p;
extern bool server_command;
extern char *lim_at_start;

extern void gdb_add_history (const char *);

extern void show_commands (const char *args, int from_tty);

extern void set_verbose (const char *, int, struct cmd_list_element *);

extern const char *handle_line_of_input (std::string &cmd_line_buffer,
					 const char *rl, int repeat,
					 const char *annotation_suffix);

/* Call at startup to see if the user has requested that gdb start up
   quietly.  */

extern bool check_quiet_mode ();

#endif
