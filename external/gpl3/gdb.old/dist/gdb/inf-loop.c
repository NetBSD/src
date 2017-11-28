/* Handling of inferior events for the event loop for GDB, the GNU debugger.
   Copyright (C) 1999-2016 Free Software Foundation, Inc.
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
#include "inferior.h"
#include "infrun.h"
#include "target.h"             /* For enum inferior_event_type.  */
#include "event-loop.h"
#include "event-top.h"
#include "inf-loop.h"
#include "remote.h"
#include "language.h"
#include "gdbthread.h"
#include "continuations.h"
#include "interps.h"
#include "top.h"
#include "observer.h"

/* General function to handle events in the inferior.  */

void
inferior_event_handler (enum inferior_event_type event_type, 
			gdb_client_data client_data)
{
  switch (event_type)
    {
    case INF_REG_EVENT:
      fetch_inferior_event (client_data);
      break;

    case INF_EXEC_COMPLETE:
      if (!non_stop)
	{
	  /* Unregister the inferior from the event loop.  This is done
	     so that when the inferior is not running we don't get
	     distracted by spurious inferior output.  */
	  if (target_has_execution && target_can_async_p ())
	    target_async (0);
	}

      /* Do all continuations associated with the whole inferior (not
	 a particular thread).  */
      if (!ptid_equal (inferior_ptid, null_ptid))
	do_all_inferior_continuations (0);

      /* When running a command list (from a user command, say), these
	 are only run when the command list is all done.  */
      if (current_ui->async)
	{
	  check_frame_language_change ();

	  /* Don't propagate breakpoint commands errors.  Either we're
	     stopping or some command resumes the inferior.  The user will
	     be informed.  */
	  TRY
	    {
	      bpstat_do_actions ();
	    }
	  CATCH (e, RETURN_MASK_ALL)
	    {
	      exception_print (gdb_stderr, e);
	    }
	  END_CATCH
	}
      break;

    default:
      printf_unfiltered (_("Event type not recognized.\n"));
      break;
    }
}
