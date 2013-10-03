/* Handling of inferior events for the event loop for GDB, the GNU debugger.
   Copyright (C) 1999-2013 Free Software Foundation, Inc.
   Written by Elena Zannoni <ezannoni@cygnus.com> of Cygnus Solutions.

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
#include "inferior.h"		/* For fetch_inferior_event.  */
#include "target.h"             /* For enum inferior_event_type.  */
#include "event-loop.h"
#include "event-top.h"
#include "inf-loop.h"
#include "remote.h"
#include "exceptions.h"
#include "language.h"
#include "gdbthread.h"
#include "continuations.h"
#include "interps.h"
#include "top.h"

static int fetch_inferior_event_wrapper (gdb_client_data client_data);

/* General function to handle events in the inferior.  So far it just
   takes care of detecting errors reported by select() or poll(),
   otherwise it assumes that all is OK, and goes on reading data from
   the fd.  This however may not always be what we want to do.  */
void
inferior_event_handler (enum inferior_event_type event_type, 
			gdb_client_data client_data)
{
  struct cleanup *cleanup_if_error = make_bpstat_clear_actions_cleanup ();

  switch (event_type)
    {
    case INF_REG_EVENT:
      /* Use catch errors for now, until the inner layers of
	 fetch_inferior_event (i.e. readchar) can return meaningful
	 error status.  If an error occurs while getting an event from
	 the target, just cancel the current command.  */
      if (!catch_errors (fetch_inferior_event_wrapper, 
			 client_data, "", RETURN_MASK_ALL))
	{
	  bpstat_clear_actions ();
	  do_all_intermediate_continuations (1);
	  do_all_continuations (1);
	  async_enable_stdin ();
	  display_gdb_prompt (0);
	}
      break;

    case INF_EXEC_COMPLETE:
      if (!non_stop)
	{
	  /* Unregister the inferior from the event loop.  This is done
	     so that when the inferior is not running we don't get
	     distracted by spurious inferior output.  */
	  if (target_has_execution)
	    target_async (NULL, 0);
	}

      /* Do all continuations associated with the whole inferior (not
	 a particular thread).  */
      if (!ptid_equal (inferior_ptid, null_ptid))
	do_all_inferior_continuations (0);

      /* If we were doing a multi-step (eg: step n, next n), but it
	 got interrupted by a breakpoint, still do the pending
	 continuations.  The continuation itself is responsible for
	 distinguishing the cases.  The continuations are allowed to
	 touch the inferior memory, e.g. to remove breakpoints, so run
	 them before running breakpoint commands, which may resume the
	 target.  */
      if (non_stop
	  && target_has_execution
	  && !ptid_equal (inferior_ptid, null_ptid))
	do_all_intermediate_continuations_thread (inferior_thread (), 0);
      else
	do_all_intermediate_continuations (0);

      /* Always finish the previous command before running any
	 breakpoint commands.  Any stop cancels the previous command.
	 E.g. a "finish" or "step-n" command interrupted by an
	 unrelated breakpoint is canceled.  */
      if (non_stop
	  && target_has_execution
	  && !ptid_equal (inferior_ptid, null_ptid))
	do_all_continuations_thread (inferior_thread (), 0);
      else
	do_all_continuations (0);

      /* When running a command list (from a user command, say), these
	 are only run when the command list is all done.  */
      if (interpreter_async)
	{
	  volatile struct gdb_exception e;

	  check_frame_language_change ();

	  /* Don't propagate breakpoint commands errors.  Either we're
	     stopping or some command resumes the inferior.  The user will
	     be informed.  */
	  TRY_CATCH (e, RETURN_MASK_ALL)
	    {
	      bpstat_do_actions ();
	    }
	  exception_print (gdb_stderr, e);
	}
      break;

    case INF_EXEC_CONTINUE:
      /* Is there anything left to do for the command issued to
         complete?  */

      if (non_stop)
	do_all_intermediate_continuations_thread (inferior_thread (), 0);
      else
	do_all_intermediate_continuations (0);
      break;

    case INF_TIMER:
    default:
      printf_unfiltered (_("Event type not recognized.\n"));
      break;
    }

  discard_cleanups (cleanup_if_error);
}

static int 
fetch_inferior_event_wrapper (gdb_client_data client_data)
{
  fetch_inferior_event (client_data);
  return 1;
}
