/* TUI display locator.

   Copyright (C) 1998-2020 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

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
#include "breakpoint.h"
#include "frame.h"
#include "command.h"
#include "inferior.h"
#include "target.h"
#include "top.h"
#include "gdb-demangle.h"
#include "source.h"
#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-stack.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-source.h"
#include "tui/tui-winsource.h"
#include "tui/tui-file.h"

#include "gdb_curses.h"

#define PROC_PREFIX             "In: "
#define LINE_PREFIX             "L"
#define PC_PREFIX               "PC: "

/* Strings to display in the TUI status line.  */
#define SINGLE_KEY              "(SingleKey)"

/* Minimum/Maximum length of some fields displayed in the TUI status
   line.  */
#define MIN_LINE_WIDTH     4	/* Use at least 4 digits for line
				   numbers.  */
#define MIN_PROC_WIDTH    12
#define MAX_TARGET_WIDTH  10
#define MAX_PID_WIDTH     19

static struct tui_locator_window _locator;



/* Accessor for the locator win info.  Answers a pointer to the static
   locator win info struct.  */
struct tui_locator_window *
tui_locator_win_info_ptr (void)
{
  return &_locator;
}

std::string
tui_locator_window::make_status_line () const
{
  char line_buf[50];
  int status_size;
  int proc_width;
  const char *pid_name;
  int target_width;
  int pid_width;
  int line_width;

  std::string pid_name_holder;
  if (inferior_ptid == null_ptid)
    pid_name = "No process";
  else
    {
      pid_name_holder = target_pid_to_str (inferior_ptid);
      pid_name = pid_name_holder.c_str ();
    }

  target_width = strlen (target_shortname);
  if (target_width > MAX_TARGET_WIDTH)
    target_width = MAX_TARGET_WIDTH;

  pid_width = strlen (pid_name);
  if (pid_width > MAX_PID_WIDTH)
    pid_width = MAX_PID_WIDTH;

  status_size = width;

  /* Translate line number and obtain its size.  */
  if (line_no > 0)
    xsnprintf (line_buf, sizeof (line_buf), "%d", line_no);
  else
    strcpy (line_buf, "??");
  line_width = strlen (line_buf);
  if (line_width < MIN_LINE_WIDTH)
    line_width = MIN_LINE_WIDTH;

  /* Translate PC address.  */
  std::string pc_out (gdbarch
		      ? paddress (gdbarch, addr)
		      : "??");
  const char *pc_buf = pc_out.c_str ();
  int pc_width = pc_out.size ();

  /* First determine the amount of proc name width we have available.
     The +1 are for a space separator between fields.
     The -1 are to take into account the \0 counted by sizeof.  */
  proc_width = (status_size
                - (target_width + 1)
                - (pid_width + 1)
                - (sizeof (PROC_PREFIX) - 1 + 1)
                - (sizeof (LINE_PREFIX) - 1 + line_width + 1)
                - (sizeof (PC_PREFIX) - 1 + pc_width + 1)
                - (tui_current_key_mode == TUI_SINGLE_KEY_MODE
                   ? (sizeof (SINGLE_KEY) - 1 + 1)
                   : 0));

  /* If there is no room to print the function name, try by removing
     some fields.  */
  if (proc_width < MIN_PROC_WIDTH)
    {
      proc_width += target_width + 1;
      target_width = 0;
      if (proc_width < MIN_PROC_WIDTH)
        {
          proc_width += pid_width + 1;
          pid_width = 0;
          if (proc_width <= MIN_PROC_WIDTH)
            {
              proc_width += pc_width + sizeof (PC_PREFIX) - 1 + 1;
              pc_width = 0;
              if (proc_width < 0)
                {
                  proc_width += line_width + sizeof (LINE_PREFIX) - 1 + 1;
                  line_width = 0;
                  if (proc_width < 0)
                    proc_width = 0;
                }
            }
        }
    }

  /* Now create the locator line from the string version of the
     elements.  */
  string_file string;

  if (target_width > 0)
    string.printf ("%*.*s ", -target_width, target_width, target_shortname);
  if (pid_width > 0)
    string.printf ("%*.*s ", -pid_width, pid_width, pid_name);

  /* Show whether we are in SingleKey mode.  */
  if (tui_current_key_mode == TUI_SINGLE_KEY_MODE)
    {
      string.puts (SINGLE_KEY);
      string.puts (" ");
    }

  /* Procedure/class name.  */
  if (proc_width > 0)
    {
      if (proc_name.size () > proc_width)
        string.printf ("%s%*.*s* ", PROC_PREFIX,
		       1 - proc_width, proc_width - 1, proc_name.c_str ());
      else
        string.printf ("%s%*.*s ", PROC_PREFIX,
		       -proc_width, proc_width, proc_name.c_str ());
    }

  if (line_width > 0)
    string.printf ("%s%*.*s ", LINE_PREFIX,
		   -line_width, line_width, line_buf);
  if (pc_width > 0)
    {
      string.puts (PC_PREFIX);
      string.puts (pc_buf);
    }

  if (string.size () < status_size)
    string.puts (n_spaces (status_size - string.size ()));
  else if (string.size () > status_size)
    string.string ().erase (status_size, string.size ());

  return std::move (string.string ());
}

/* Get a printable name for the function at the address.  The symbol
   name is demangled if demangling is turned on.  Returns a pointer to
   a static area holding the result.  */
static char*
tui_get_function_from_frame (struct frame_info *fi)
{
  static char name[256];
  string_file stream;

  print_address_symbolic (get_frame_arch (fi), get_frame_pc (fi),
			  &stream, demangle, "");

  /* Use simple heuristics to isolate the function name.  The symbol
     can be demangled and we can have function parameters.  Remove
     them because the status line is too short to display them.  */
  const char *d = stream.c_str ();
  if (*d == '<')
    d++;
  strncpy (name, d, sizeof (name) - 1);
  name[sizeof (name) - 1] = 0;

  char *p = strchr (name, '(');
  if (!p)
    p = strchr (name, '>');
  if (p)
    *p = 0;
  p = strchr (name, '+');
  if (p)
    *p = 0;
  return name;
}

void
tui_locator_window::rerender ()
{
  if (handle != NULL)
    {
      std::string string = make_status_line ();
      scrollok (handle.get (), FALSE);
      wmove (handle.get (), 0, 0);
      /* We ignore the return value from wstandout and wstandend, casting
	 them to void in order to avoid a compiler warning.  The warning
	 itself was introduced by a patch to ncurses 5.7 dated 2009-08-29,
	 changing these macro to expand to code that causes the compiler
	 to generate an unused-value warning.  */
      (void) wstandout (handle.get ());
      waddstr (handle.get (), string.c_str ());
      wclrtoeol (handle.get ());
      (void) wstandend (handle.get ());
      refresh_window ();
      wmove (handle.get (), 0, 0);
    }
}

/* See tui-stack.h.  */

void
tui_locator_window::set_locator_fullname (const char *fullname)
{
  full_name = fullname;
  rerender ();
}

/* See tui-stack.h.  */

bool
tui_locator_window::set_locator_info (struct gdbarch *gdbarch_in,
				      const struct symtab_and_line &sal,
				      const char *procname)
{
  bool locator_changed_p = false;

  gdb_assert (procname != NULL);

  const char *fullname = (sal.symtab == nullptr
			  ? "??"
			  : symtab_to_fullname (sal.symtab));

  locator_changed_p |= proc_name != procname;
  locator_changed_p |= sal.line != line_no;
  locator_changed_p |= sal.pc != addr;
  locator_changed_p |= gdbarch_in != gdbarch;
  locator_changed_p |= full_name != fullname;

  proc_name = procname;
  line_no = sal.line;
  addr = sal.pc;
  gdbarch = gdbarch_in;
  set_locator_fullname (fullname);

  return locator_changed_p;
}

/* Update only the full_name portion of the locator.  */
void
tui_update_locator_fullname (struct symtab *symtab)
{
  struct tui_locator_window *locator = tui_locator_win_info_ptr ();

  const char *fullname;
  if (symtab != nullptr)
    fullname = symtab_to_fullname (symtab);
  else
    fullname = "??";
  locator->set_locator_fullname (fullname);
}

/* Function to print the frame information for the TUI.  The windows are
   refreshed only if frame information has changed since the last refresh.

   Return true if frame information has changed (and windows
   subsequently refreshed), false otherwise.  */

bool
tui_show_frame_info (struct frame_info *fi)
{
  bool locator_changed_p;
  struct tui_locator_window *locator = tui_locator_win_info_ptr ();

  if (fi)
    {
      symtab_and_line sal = find_frame_sal (fi);

      const char *func_name;
      /* find_frame_sal does not always set PC, but we want to ensure
	 that it is available in the SAL.  */
      if (get_frame_pc_if_available (fi, &sal.pc))
	func_name = tui_get_function_from_frame (fi);
      else
	func_name = _("<unavailable>");

      locator_changed_p = locator->set_locator_info (get_frame_arch (fi),
						     sal, func_name);

      /* If the locator information has not changed, then frame information has
	 not changed.  If frame information has not changed, then the windows'
	 contents will not change.  So don't bother refreshing the windows.  */
      if (!locator_changed_p)
	return false;

      for (struct tui_source_window_base *win_info : tui_source_windows ())
	{
	  win_info->maybe_update (fi, sal);
	  win_info->update_exec_info ();
	}
    }
  else
    {
      symtab_and_line sal {};

      locator_changed_p = locator->set_locator_info (NULL, sal, "");

      if (!locator_changed_p)
	return false;

      for (struct tui_source_window_base *win_info : tui_source_windows ())
	win_info->erase_source_content ();
    }

  return true;
}

void
tui_show_locator_content ()
{
  struct tui_locator_window *locator = tui_locator_win_info_ptr ();
  locator->rerender ();
}

/* Command to update the display with the current execution point.  */
static void
tui_update_command (const char *arg, int from_tty)
{
  execute_command ("frame 0", from_tty);
}

/* Function to initialize gdb commands, for tui window stack
   manipulation.  */

void _initialize_tui_stack ();
void
_initialize_tui_stack ()
{
  add_com ("update", class_tui, tui_update_command,
	   _("Update the source window and locator to "
	     "display the current execution point.\n\
Usage: update"));
}
