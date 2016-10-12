/* GDB hooks for TUI.

   Copyright (C) 2001-2016 Free Software Foundation, Inc.

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

static void
tui_new_objfile_hook (struct objfile* objfile)
{
  if (tui_active)
    tui_display_main ();
}

/* Prevent recursion of deprecated_register_changed_hook().  */
static int tui_refreshing_registers = 0;

/* Observer for the register_changed notification.  */

static void
tui_register_changed (struct frame_info *frame, int regno)
{
  struct frame_info *fi;

  /* The frame of the register that was changed may differ from the selected
     frame, but we only want to show the register values of the selected frame.
     And even if the frames differ a register change made in one can still show
     up in the other.  So we always use the selected frame here, and ignore
     FRAME.  */
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

/* Refresh TUI's frame and register information.  This is a hook intended to be
   used to update the screen after potential frame and register changes.

   REGISTERS_TOO_P controls whether to refresh our register information even
   if frame information hasn't changed.  */

static void
tui_refresh_frame_and_register_information (int registers_too_p)
{
  struct frame_info *fi;
  CORE_ADDR pc;
  struct cleanup *old_chain;
  int frame_info_changed_p;

  if (!has_stack_frames ())
    return;

  old_chain = make_cleanup_restore_target_terminal ();
  target_terminal_ours_for_output ();

  fi = get_selected_frame (NULL);
  /* Ensure that symbols for this frame are read in.  Also, determine
     the source language of this frame, and switch to it if
     desired.  */
  if (get_frame_pc_if_available (fi, &pc))
    {
      struct symtab *s;

      s = find_pc_line_symtab (pc);
      /* elz: This if here fixes the problem with the pc not being
	 displayed in the tui asm layout, with no debug symbols.  The
	 value of s would be 0 here, and select_source_symtab would
	 abort the command by calling the 'error' function.  */
      if (s)
	select_source_symtab (s);
    }

  /* Display the frame position (even if there is no symbols or the PC
     is not known).  */
  frame_info_changed_p = tui_show_frame_info (fi);

  /* Refresh the register window if it's visible.  */
  if (tui_is_window_visible (DATA_WIN)
      && (frame_info_changed_p || registers_too_p))
    {
      tui_refreshing_registers = 1;
      tui_check_data_values (fi);
      tui_refreshing_registers = 0;
    }

  do_cleanups (old_chain);
}

/* Dummy callback for deprecated_print_frame_info_listing_hook which is called
   from print_frame_info.  */

static void
tui_dummy_print_frame_info_listing_hook (struct symtab *s,
					 int line,
					 int stopline, 
					 int noerror)
{
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

/* Observer for the before_prompt notification.  */

static void
tui_before_prompt (const char *current_gdb_prompt)
{
  /* This refresh is intended to catch changes to the selected frame following
     a call to "up", "down" or "frame".  As such we don't necessarily want to
     refresh registers here unless the frame actually changed by one of these
     commands.  Registers will otherwise be refreshed after a normal stop or by
     our tui_register_changed_hook.  */
  tui_refresh_frame_and_register_information (/*registers_too_p=*/0);
}

/* Observer for the normal_stop notification.  */

static void
tui_normal_stop (struct bpstats *bs, int print_frame)
{
  /* This refresh is intended to catch changes to the selected frame and to
     registers following a normal stop.  */
  tui_refresh_frame_and_register_information (/*registers_too_p=*/1);
}

/* Observers created when installing TUI hooks.  */
static struct observer *tui_bp_created_observer;
static struct observer *tui_bp_deleted_observer;
static struct observer *tui_bp_modified_observer;
static struct observer *tui_inferior_exit_observer;
static struct observer *tui_before_prompt_observer;
static struct observer *tui_normal_stop_observer;
static struct observer *tui_register_changed_observer;

/* Install the TUI specific hooks.  */
void
tui_install_hooks (void)
{
  /* If this hook is not set to something then print_frame_info will
     assume that the CLI, not the TUI, is active, and will print the frame info
     for us in such a way that we are not prepared to handle.  This hook is
     otherwise effectively obsolete.  */
  deprecated_print_frame_info_listing_hook
    = tui_dummy_print_frame_info_listing_hook;

  /* Install the event hooks.  */
  tui_bp_created_observer
    = observer_attach_breakpoint_created (tui_event_create_breakpoint);
  tui_bp_deleted_observer
    = observer_attach_breakpoint_deleted (tui_event_delete_breakpoint);
  tui_bp_modified_observer
    = observer_attach_breakpoint_modified (tui_event_modify_breakpoint);
  tui_inferior_exit_observer
    = observer_attach_inferior_exit (tui_inferior_exit);
  tui_before_prompt_observer
    = observer_attach_before_prompt (tui_before_prompt);
  tui_normal_stop_observer
    = observer_attach_normal_stop (tui_normal_stop);
  tui_register_changed_observer
    = observer_attach_register_changed (tui_register_changed);
}

/* Remove the TUI specific hooks.  */
void
tui_remove_hooks (void)
{
  deprecated_print_frame_info_listing_hook = 0;
  deprecated_query_hook = 0;
  /* Remove our observers.  */
  observer_detach_breakpoint_created (tui_bp_created_observer);
  tui_bp_created_observer = NULL;
  observer_detach_breakpoint_deleted (tui_bp_deleted_observer);
  tui_bp_deleted_observer = NULL;
  observer_detach_breakpoint_modified (tui_bp_modified_observer);
  tui_bp_modified_observer = NULL;
  observer_detach_inferior_exit (tui_inferior_exit_observer);
  tui_inferior_exit_observer = NULL;
  observer_detach_before_prompt (tui_before_prompt_observer);
  tui_before_prompt_observer = NULL;
  observer_detach_normal_stop (tui_normal_stop_observer);
  tui_normal_stop_observer = NULL;
  observer_detach_register_changed (tui_register_changed_observer);
  tui_register_changed_observer = NULL;
}

void _initialize_tui_hooks (void);

void
_initialize_tui_hooks (void)
{
  /* Install the permanent hooks.  */
  observer_attach_new_objfile (tui_new_objfile_hook);
}
