/* GDB hooks for TUI.

   Copyright 2001, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* FIXME: cagney/2002-02-28: The GDB coding standard indicates that
   "defs.h" should be included first.  Unfortunatly some systems
   (currently Debian GNU/Linux) include the <stdbool.h> via <curses.h>
   and they clash with "bfd.h"'s definiton of true/false.  The correct
   fix is to remove true/false from "bfd.h", however, until that
   happens, hack around it by including "config.h" and <curses.h>
   first.  */

#include "config.h"
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#endif

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
#include "gdb-events.h"
#include "ui-out.h"
#include "top.h"
#include <readline/readline.h>
#include <unistd.h>
#include <fcntl.h>

#include "tui.h"
#include "tuiData.h"
#include "tuiLayout.h"
#include "tuiIO.h"
#include "tuiRegs.h"
#include "tuiWin.h"
#include "tuiStack.h"
#include "tuiDataWin.h"
#include "tuiSourceWin.h"

int tui_target_has_run = 0;

static void (* tui_target_new_objfile_chain) (struct objfile*);
extern void (*selected_frame_level_changed_hook) (int);
static void tui_event_loop (void);
static void tui_command_loop (void);

static void
tui_new_objfile_hook (struct objfile* objfile)
{
  if (tui_active)
    tui_display_main ();
  
  if (tui_target_new_objfile_chain)
    tui_target_new_objfile_chain (objfile);
}

static int
tui_query_hook (const char * msg, va_list argp)
{
  int retval;
  int ans2;
  int answer;

  /* Automatically answer "yes" if input is not from a terminal.  */
  if (!input_from_terminal_p ())
    return 1;

  echo ();
  while (1)
    {
      wrap_here ("");		/* Flush any buffered output */
      gdb_flush (gdb_stdout);

      vfprintf_filtered (gdb_stdout, msg, argp);
      printf_filtered ("(y or n) ");

      wrap_here ("");
      gdb_flush (gdb_stdout);

      answer = tui_getc (stdin);
      clearerr (stdin);		/* in case of C-d */
      if (answer == EOF)	/* C-d */
	{
	  retval = 1;
	  break;
	}
      /* Eat rest of input line, to EOF or newline */
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
      printf_filtered ("Please answer y or n.\n");
    }
  noecho ();
  return retval;
}

/* Prevent recursion of registers_changed_hook().  */
static int tui_refreshing_registers = 0;

static void
tui_registers_changed_hook (void)
{
  struct frame_info *fi;

  fi = selected_frame;
  if (fi && tui_refreshing_registers == 0)
    {
      tui_refreshing_registers = 1;
#if 0
      tuiCheckDataValues (fi);
#endif
      tui_refreshing_registers = 0;
    }
}

static void
tui_register_changed_hook (int regno)
{
  struct frame_info *fi;

  fi = selected_frame;
  if (fi && tui_refreshing_registers == 0)
    {
      tui_refreshing_registers = 1;
      tuiCheckDataValues (fi);
      tui_refreshing_registers = 0;
    }
}

/* Breakpoint creation hook.
   Update the screen to show the new breakpoint.  */
static void
tui_event_create_breakpoint (int number)
{
  tui_update_all_breakpoint_info ();
}

/* Breakpoint deletion hook.
   Refresh the screen to update the breakpoint marks.  */
static void
tui_event_delete_breakpoint (int number)
{
  tui_update_all_breakpoint_info ();
}

static void
tui_event_modify_breakpoint (int number)
{
  tui_update_all_breakpoint_info ();
}

static void
tui_event_default (int number)
{
  ;
}

static struct gdb_events *tui_old_event_hooks;

static struct gdb_events tui_event_hooks =
{
  tui_event_create_breakpoint,
  tui_event_delete_breakpoint,
  tui_event_modify_breakpoint,
  tui_event_default,
  tui_event_default,
  tui_event_default
};

/* Called when going to wait for the target.
   Leave curses mode and setup program mode.  */
static ptid_t
tui_target_wait_hook (ptid_t pid, struct target_waitstatus *status)
{
  ptid_t res;

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
  res = target_wait (pid, status);

  if (tui_active)
    {
      /* TODO: need to refresh (optional).  */
    }
  return res;
}

/* The selected frame has changed.  This is happens after a target
   stop or when the user explicitly changes the frame (up/down/thread/...).  */
static void
tui_selected_frame_level_changed_hook (int level)
{
  struct frame_info *fi;

  fi = selected_frame;
  /* Ensure that symbols for this frame are read in.  Also, determine the
     source language of this frame, and switch to it if desired.  */
  if (fi)
    {
      struct symtab *s;
      
      s = find_pc_symtab (fi->pc);
      /* elz: this if here fixes the problem with the pc not being displayed
         in the tui asm layout, with no debug symbols. The value of s 
         would be 0 here, and select_source_symtab would abort the
         command by calling the 'error' function */
      if (s)
        select_source_symtab (s);

      /* Display the frame position (even if there is no symbols).  */
      tuiShowFrameInfo (fi);

      /* Refresh the register window if it's visible.  */
      if (tui_is_window_visible (DATA_WIN))
        {
          tui_refreshing_registers = 1;
          tuiCheckDataValues (fi);
          tui_refreshing_registers = 0;
        }
    }
}

/* Called from print_frame_info to list the line we stopped in.  */
static void
tui_print_frame_info_listing_hook (struct symtab *s, int line,
                                   int stopline, int noerror)
{
  select_source_symtab (s);
  tuiShowFrameInfo (selected_frame);
}

/* Called when the target process died or is detached.
   Update the status line.  */
static void
tui_detach_hook (void)
{
  tuiShowFrameInfo (0);
  tui_display_main ();
}

/* Install the TUI specific hooks.  */
void
tui_install_hooks (void)
{
  target_wait_hook = tui_target_wait_hook;
  selected_frame_level_changed_hook = tui_selected_frame_level_changed_hook;
  print_frame_info_listing_hook = tui_print_frame_info_listing_hook;

  query_hook = tui_query_hook;

  /* Install the event hooks.  */
  tui_old_event_hooks = set_gdb_event_hooks (&tui_event_hooks);

  registers_changed_hook = tui_registers_changed_hook;
  register_changed_hook = tui_register_changed_hook;
  detach_hook = tui_detach_hook;
}

/* Remove the TUI specific hooks.  */
void
tui_remove_hooks (void)
{
  target_wait_hook = 0;
  selected_frame_level_changed_hook = 0;
  print_frame_info_listing_hook = 0;
  query_hook = 0;
  registers_changed_hook = 0;
  register_changed_hook = 0;
  detach_hook = 0;

  /* Restore the previous event hooks.  */
  set_gdb_event_hooks (tui_old_event_hooks);
}

/* Cleanup the tui before exiting.  */
static void
tui_exit (void)
{
  /* Disable the tui.  Curses mode is left leaving the screen
     in a clean state (see endwin()).  */
  tui_disable ();
}

/* Initialize all the necessary variables, start the event loop,
   register readline, and stdin, start the loop. */
static void
tui_command_loop (void)
{
  int length;
  char *a_prompt;
  char *gdb_prompt = get_prompt ();

  /* If we are using readline, set things up and display the first
     prompt, otherwise just print the prompt. */
  if (async_command_editing_p)
    {
      /* Tell readline what the prompt to display is and what function it
         will need to call after a whole line is read. This also displays
         the first prompt. */
      length = strlen (PREFIX (0)) + strlen (gdb_prompt) + strlen (SUFFIX (0)) + 1;
      a_prompt = (char *) xmalloc (length);
      strcpy (a_prompt, PREFIX (0));
      strcat (a_prompt, gdb_prompt);
      strcat (a_prompt, SUFFIX (0));
      rl_callback_handler_install (a_prompt, input_handler);
    }
  else
    display_gdb_prompt (0);

  /* Now it's time to start the event loop. */
  tui_event_loop ();
}

/* Start up the event loop. This is the entry point to the event loop
   from the command loop. */

static void
tui_event_loop (void)
{
  /* Loop until there is nothing to do. This is the entry point to the
     event loop engine. gdb_do_one_event, called via catch_errors()
     will process one event for each invocation.  It blocks waits for
     an event and then processes it.  >0 when an event is processed, 0
     when catch_errors() caught an error and <0 when there are no
     longer any event sources registered. */
  while (1)
    {
      int result = catch_errors (gdb_do_one_event, 0, "", RETURN_MASK_ALL);
      if (result < 0)
	break;

      /* Update gdb output according to TUI mode.  Since catch_errors
         preserves the uiout from changing, this must be done at top
         level of event loop.  */
      if (tui_active)
        uiout = tui_out;
      else
        uiout = tui_old_uiout;
      
      if (result == 0)
	{
	  /* FIXME: this should really be a call to a hook that is
	     interface specific, because interfaces can display the
	     prompt in their own way. */
	  display_gdb_prompt (0);
	  /* This call looks bizarre, but it is required.  If the user
	     entered a command that caused an error,
	     after_char_processing_hook won't be called from
	     rl_callback_read_char_wrapper.  Using a cleanup there
	     won't work, since we want this function to be called
	     after a new prompt is printed.  */
	  if (after_char_processing_hook)
	    (*after_char_processing_hook) ();
	  /* Maybe better to set a flag to be checked somewhere as to
	     whether display the prompt or not. */
	}
    }

  /* We are done with the event loop. There are no more event sources
     to listen to.  So we exit GDB. */
  return;
}

/* Initialize the tui by installing several gdb hooks, initializing
   the tui IO and preparing the readline with the kind binding.  */
static void
tui_init_hook (char *argv0)
{
  /* Don't enable the TUI if a specific interpreter is installed.  */
  if (interpreter_p)
    return;

  /* Install exit handler to leave the screen in a good shape.  */
  atexit (tui_exit);

  initializeStaticData ();

  /* Install the permanent hooks.  */
  tui_target_new_objfile_chain = target_new_objfile_hook;
  target_new_objfile_hook = tui_new_objfile_hook;

  tui_initialize_io ();
  tui_initialize_readline ();

  /* Tell gdb to use the tui_command_loop as the main loop. */
  command_loop_hook = tui_command_loop;

  /* Decide in which mode to start using GDB (based on -tui).  */
  if (tui_version)
    {
      tui_enable ();
    }
}

/* Initialize the tui.  */
void
_initialize_tui (void)
{
  /* Setup initialization hook.  */
  init_ui_hook = tui_init_hook;
}

