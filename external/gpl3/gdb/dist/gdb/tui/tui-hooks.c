/* GDB hooks for TUI.

   Copyright (C) 2001-2013 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "inferior.h"
#include "command.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "target.h"
#include "gdbcore.h"
#include "event-loop.h"
#include "event-top.h"
#include "frame.h"
#include "breakpoint.h"
#include "ui-out.h"
#include "top.h"
#include "observer.h"
#include <unistd.h>
#include <fcntl.h>

#include "tui/tui.h"
#include "tui/tui-hooks.h"
#include "tui/tui-data.h"
#include "tui/tui-layout.h"
#include "tui/tui-io.h"
#include "tui/tui-regs.h"
#include "tui/tui-win.h"
#include "tui/tui-stack.h"
#include "tui/tui-windata.h"
#include "tui/tui-winsource.h"

#include "gdb_curses.h"

/* This redefines CTRL if it is not already defined, so it must come
   after terminal state releated include files like <term.h> and
   "gdb_curses.h".  */
#include "readline/readline.h"

int tui_target_has_run = 0;

static void
tui_new_objfile_hook (struct objfile* objfile)
{
  if (tui_active)
    tui_display_main ();
}

static int ATTRIBUTE_PRINTF (1, 0)
tui_query_hook (const char *msg, va_list argp)
{
  int retval;
  int ans2;
  int answer;
  char *question;
  struct cleanup *old_chain;

  /* Format the question outside of the loop, to avoid reusing
     ARGP.  */
  question = xstrvprintf (msg, argp);
  old_chain = make_cleanup (xfree, question);

  echo ();
  while (1)
    {
      wrap_here ("");		/* Flush any buffered output.  */
      gdb_flush (gdb_stdout);

      fputs_filtered (question, gdb_stdout);
      printf_filtered (_("(y or n) "));

      wrap_here ("");
      gdb_flush (gdb_stdout);

      answer = tui_getc (stdin);
      clearerr (stdin);		/* in case of C-d */
      if (answer == EOF)	/* C-d */
	{
	  retval = 1;
	  break;
	}
      /* Eat rest of input line, to EOF or newline.  */
      if (answer != '\n')
	do
	  {
            ans2 = tui_getc (stdin);
	    clearerr (stdin);
	  }
	while (ans2 != EOF && ans2 != '\n' && ans2 != '\r');

      if (answer >= 'a')
	answer -= 040;
      if (answer == 'Y')
	{
	  retval = 1;
	  break;
	}
      if (answer == 'N')
	{
	  retval = 0;
	  break;
	}
      printf_filtered (_("Please answer y or n.\n"));
    }
  noecho ();

  do_cleanups (old_chain);
  return retval;
}

/* Prevent recursion of deprecated_register_changed_hook().  */
static int tui_refreshing_registers = 0;

static void
tui_register_changed_hook (int regno)
{
  struct frame_info *fi;

  fi = get_selected_frame (NULL);
  if (tui_refreshing_registers == 0)
    {
      tui_refreshing_registers = 1;
      tui_check_data_values (fi);
      tui_refreshing_registers = 0;
    }
}

/* Breakpoint creation hook.
   Update the screen to show the new breakpoint.  */
static void
tui_event_create_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info ();
}

/* Breakpoint deletion hook.
   Refresh the screen to update the breakpoint marks.  */
static void
tui_event_delete_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info ();
}

static void
tui_event_modify_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info ();
}

/* Called when a command is about to proceed the inferior.  */

static void
tui_about_to_proceed (void)
{
  /* Leave tui mode (optional).  */
#if 0
  if (tui_active)
    {
      target_terminal_ours ();
      endwin ();
      target_terminal_inferior ();
    }
#endif
  tui_target_has_run = 1;
}

/* The selected frame has changed.  This is happens after a target
   stop or when the user explicitly changes the frame
   (up/down/thread/...).  */
static void
tui_selected_frame_level_changed_hook (int level)
{
  struct frame_info *fi;
  CORE_ADDR pc;

  /* Negative level means that the selected frame was cleared.  */
  if (level < 0)
    return;

  fi = get_selected_frame (NULL);
  /* Ensure that symbols for this frame are read in.  Also, determine
     the source language of this frame, and switch to it if
     desired.  */
  if (get_frame_pc_if_available (fi, &pc))
    {
      struct symtab *s;

      s = find_pc_symtab (pc);
      /* elz: This if here fixes the problem with the pc not being
	 displayed in the tui asm layout, with no debug symbols.  The
	 value of s would be 0 here, and select_source_symtab would
	 abort the command by calling the 'error' function.  */
      if (s)
	select_source_symtab (s);
    }

  /* Display the frame position (even if there is no symbols or the PC
     is not known).  */
  tui_show_frame_info (fi);

  /* Refresh the register window if it's visible.  */
  if (tui_is_window_visible (DATA_WIN))
    {
      tui_refreshing_registers = 1;
      tui_check_data_values (fi);
      tui_refreshing_registers = 0;
    }
}

/* Called from print_frame_info to list the line we stopped in.  */
static void
tui_print_frame_info_listing_hook (struct symtab *s,
				   int line,
                                   int stopline, 
				   int noerror)
{
  select_source_symtab (s);
  tui_show_frame_info (get_selected_frame (NULL));
}

/* Perform all necessary cleanups regarding our module's inferior data
   that is required after the inferior INF just exited.  */

static void
tui_inferior_exit (struct inferior *inf)
{
  /* Leave the SingleKey mode to make sure the gdb prompt is visible.  */
  tui_set_key_mode (TUI_COMMAND_MODE);
  tui_show_frame_info (0);
  tui_display_main ();
}

/* Observers created when installing TUI hooks.  */
static struct observer *tui_bp_created_observer;
static struct observer *tui_bp_deleted_observer;
static struct observer *tui_bp_modified_observer;
static struct observer *tui_inferior_exit_observer;
static struct observer *tui_about_to_proceed_observer;

/* Install the TUI specific hooks.  */
void
tui_install_hooks (void)
{
  deprecated_selected_frame_level_changed_hook
    = tui_selected_frame_level_changed_hook;
  deprecated_print_frame_info_listing_hook
    = tui_print_frame_info_listing_hook;

  deprecated_query_hook = tui_query_hook;

  /* Install the event hooks.  */
  tui_bp_created_observer
    = observer_attach_breakpoint_created (tui_event_create_breakpoint);
  tui_bp_deleted_observer
    = observer_attach_breakpoint_deleted (tui_event_delete_breakpoint);
  tui_bp_modified_observer
    = observer_attach_breakpoint_modified (tui_event_modify_breakpoint);
  tui_inferior_exit_observer
    = observer_attach_inferior_exit (tui_inferior_exit);
  tui_about_to_proceed_observer
    = observer_attach_about_to_proceed (tui_about_to_proceed);

  deprecated_register_changed_hook = tui_register_changed_hook;
}

/* Remove the TUI specific hooks.  */
void
tui_remove_hooks (void)
{
  deprecated_selected_frame_level_changed_hook = 0;
  deprecated_print_frame_info_listing_hook = 0;
  deprecated_query_hook = 0;
  deprecated_register_changed_hook = 0;

  /* Remove our observers.  */
  observer_detach_breakpoint_created (tui_bp_created_observer);
  tui_bp_created_observer = NULL;
  observer_detach_breakpoint_deleted (tui_bp_deleted_observer);
  tui_bp_deleted_observer = NULL;
  observer_detach_breakpoint_modified (tui_bp_modified_observer);
  tui_bp_modified_observer = NULL;
  observer_detach_inferior_exit (tui_inferior_exit_observer);
  tui_inferior_exit_observer = NULL;
  observer_detach_about_to_proceed (tui_about_to_proceed_observer);
  tui_about_to_proceed_observer = NULL;
}

void _initialize_tui_hooks (void);

void
_initialize_tui_hooks (void)
{
  /* Install the permanent hooks.  */
  observer_attach_new_objfile (tui_new_objfile_hook);
}
