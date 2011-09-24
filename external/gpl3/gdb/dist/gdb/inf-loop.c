/* Handling of inferior events for the event loop for GDB, the GNU debugger.
   Copyright (C) 1999, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.
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

static int fetch_inferior_event_wrapper (gdb_client_data client_data);

void
inferior_event_handler_wrapper (gdb_client_data client_data)
{
  inferior_event_handler (INF_QUIT_REQ, client_data);
}

/* General function to handle events in the inferior.  So far it just
   takes care of detecting errors reported by select() or poll(),
   otherwise it assumes that all is OK, and goes on reading data from
   the fd.  This however may not always be what we want to do.  */
void
inferior_event_handler (enum inferior_event_type event_type, 
			gdb_client_data client_data)
{
  struct gdb_exception e;
  int was_sync = 0;

  switch (event_type)
    {
    case INF_ERROR:
      printf_unfiltered (_("error detected from target.\n"));
      pop_all_targets_above (file_stratum, 0);
      discard_all_intermediate_continuations ();
      discard_all_continuations ();
      async_enable_stdin ();
      break;

    case INF_REG_EVENT:
      /* Use catch errors for now, until the inner layers of
	 fetch_inferior_event (i.e. readchar) can return meaningful
	 error status.  If an error occurs while getting an event from
	 the target, just get rid of the target.  */
      if (!catch_errors (fetch_inferior_event_wrapper, 
			 client_data, "", RETURN_MASK_ALL))
	{
	  pop_all_targets_above (file_stratum, 0);
	  discard_all_intermediate_continuations ();
	  discard_all_continuations ();
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

      /* The call to async_enable_stdin below resets 'sync_execution'.
	 However, if sync_execution is 1 now, we also need to show the
	 prompt below, so save the current value.  */
      was_sync = sync_execution;
      async_enable_stdin ();

      /* Do all continuations associated with the whole inferior (not
	 a particular thread).  */
      if (!ptid_equal (inferior_ptid, null_ptid))
	do_all_inferior_continuations ();

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
	do_all_intermediate_continuations_thread (inferior_thread ());
      else
	do_all_intermediate_continuations ();

      /* Always finish the previous command before running any
	 breakpoint commands.  Any stop cancels the previous command.
	 E.g. a "finish" or "step-n" command interrupted by an
	 unrelated breakpoint is canceled.  */
      if (non_stop
	  && target_has_execution
	  && !ptid_equal (inferior_ptid, null_ptid))
	do_all_continuations_thread (inferior_thread ());
      else
	do_all_continuations ();

      if (current_language != expected_language
	  && language_mode == language_mode_auto)
	language_info (1);	/* Print what changed.  */

      /* Don't propagate breakpoint commands errors.  Either we're
	 stopping or some command resumes the inferior.  The user will
	 be informed.  */
      TRY_CATCH (e, RETURN_MASK_ALL)
	{
	  bpstat_do_actions ();
	}

      if (!was_sync
	  && exec_done_display_p
	  && (ptid_equal (inferior_ptid, null_ptid)
	      || !is_running (inferior_ptid)))
	printf_unfiltered (_("completed.\n"));
      break;

    case INF_EXEC_CONTINUE:
      /* Is there anything left to do for the command issued to
         complete?  */

      if (non_stop)
	do_all_intermediate_continuations_thread (inferior_thread ());
      else
	do_all_intermediate_continuations ();
      break;

    case INF_QUIT_REQ: 
      /* FIXME: ezannoni 1999-10-04.  This call should really be a
	 target vector entry, so that it can be used for any kind of
	 targets.  */
      async_remote_interrupt_twice (NULL);
      break;

    case INF_TIMER:
    default:
      printf_unfiltered (_("Event type not recognized.\n"));
      break;
    }
}

static int 
fetch_inferior_event_wrapper (gdb_client_data client_data)
{
  fetch_inferior_event (client_data);
  return 1;
}
