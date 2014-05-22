/* Remote notification in GDB protocol

   Copyright (C) 1988-2013 Free Software Foundation, Inc.

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

/* Remote async notification is sent from remote target over RSP.
   Each type of notification is represented by an object of
   'struct notif', which has a field 'pending_reply'.  It is not
   NULL when GDB receives a notification from GDBserver, but hasn't
   acknowledge yet.  Before GDB acknowledges the notification,
   GDBserver shouldn't send notification again (see the header comments
   in gdbserver/notif.c).

   Notifications are processed in an almost-unified approach for both
   all-stop mode and non-stop mode, except the timing to process them.
   In non-stop mode, notifications are processed in
   remote_async_get_pending_events_handler, while in all-stop mode,
   they are processed in remote_resume.  */

#include "defs.h"
#include "remote.h"
#include "remote-notif.h"
#include "observer.h"
#include "event-loop.h"
#include "target.h"
#include "inferior.h"
#include "gdbcmd.h"

#include <string.h>

int notif_debug = 0;

/* Supported clients of notifications.  */

static struct notif_client *notifs[] =
{
  &notif_client_stop,
};

static void do_notif_event_xfree (void *arg);

/* Parse the BUF for the expected notification NC, and send packet to
   acknowledge.  */

void
remote_notif_ack (struct notif_client *nc, char *buf)
{
  struct notif_event *event = nc->alloc_event ();
  struct cleanup *old_chain
    = make_cleanup (do_notif_event_xfree, event);

  if (notif_debug)
    fprintf_unfiltered (gdb_stdlog, "notif: ack '%s'\n",
			nc->ack_command);

  nc->parse (nc, buf, event);
  nc->ack (nc, buf, event);

  discard_cleanups (old_chain);
}

/* Parse the BUF for the expected notification NC.  */

struct notif_event *
remote_notif_parse (struct notif_client *nc, char *buf)
{
  struct notif_event *event = nc->alloc_event ();
  struct cleanup *old_chain
    = make_cleanup (do_notif_event_xfree, event);

  if (notif_debug)
    fprintf_unfiltered (gdb_stdlog, "notif: parse '%s'\n", nc->name);

  nc->parse (nc, buf, event);

  discard_cleanups (old_chain);
  return event;
}

DECLARE_QUEUE_P (notif_client_p);
DEFINE_QUEUE_P (notif_client_p);

static QUEUE(notif_client_p) *notif_queue;

/* Process notifications one by one.  EXCEPT is not expected in
   the queue.  */

void
remote_notif_process (struct notif_client *except)
{
  while (!QUEUE_is_empty (notif_client_p, notif_queue))
    {
      struct notif_client *nc = QUEUE_deque (notif_client_p,
					     notif_queue);

      gdb_assert (nc != except);

      if (nc->can_get_pending_events (nc))
	remote_notif_get_pending_events (nc);
    }
}

static void
remote_async_get_pending_events_handler (gdb_client_data data)
{
  gdb_assert (non_stop);
  remote_notif_process (NULL);
}

/* Asynchronous signal handle registered as event loop source for when
   the remote sent us a notification.  The registered callback
   will do a ACK sequence to pull the rest of the events out of
   the remote side into our event queue.  */

static struct async_event_handler *remote_async_get_pending_events_token;

/* Register async_event_handler for notification.  */

void
remote_notif_register_async_event_handler (void)
{
  remote_async_get_pending_events_token
    = create_async_event_handler (remote_async_get_pending_events_handler,
				  NULL);
}

/* Unregister async_event_handler for notification.  */

void
remote_notif_unregister_async_event_handler (void)
{
  if (remote_async_get_pending_events_token)
    delete_async_event_handler (&remote_async_get_pending_events_token);
}

/* Remote notification handler.  */

void
handle_notification (char *buf)
{
  struct notif_client *nc = NULL;
  int i;

  for (i = 0; i < ARRAY_SIZE (notifs); i++)
    {
      nc = notifs[i];
      if (strncmp (buf, nc->name, strlen (nc->name)) == 0
	  && buf[strlen (nc->name)] == ':')
	break;
    }

  /* We ignore notifications we don't recognize, for compatibility
     with newer stubs.  */
  if (nc == NULL)
    return;

  if (nc->pending_event)
    {
      /* We've already parsed the in-flight reply, but the stub for some
	 reason thought we didn't, possibly due to timeout on its side.
	 Just ignore it.  */
      if (notif_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "notif: ignoring resent notification\n");
    }
  else
    {
      struct notif_event *event
	= remote_notif_parse (nc, buf + strlen (nc->name) + 1);

      /* Be careful to only set it after parsing, since an error
	 may be thrown then.  */
      nc->pending_event = event;

      /* Notify the event loop there's a stop reply to acknowledge
	 and that there may be more events to fetch.  */
      QUEUE_enque (notif_client_p, notif_queue, nc);
      if (non_stop)
	{
	  /* In non-stop, We mark REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN
	     in order to go on what we were doing and postpone
	     querying notification events to some point safe to do so.
	     See details in the function comment of
	     remote.c:remote_notif_get_pending_events.

	     In all-stop, GDB may be blocked to wait for the reply, we
	     shouldn't return to event loop until the expected reply
	     arrives.  For example:

	     1.1) --> vCont;c
	       GDB expects getting stop reply 'T05 thread:2'.
	     1.2) <-- %Notif
	       <GDB marks the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN>

	     After step #1.2, we return to the event loop, which
	     notices there is a new event on the
	     REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN and calls the
	     handler, which will send 'vNotif' packet.
	     1.3) --> vNotif
	     It is not safe to start a new sequence, because target
	     is still running and GDB is expecting the stop reply
	     from stub.

	     To solve this, whenever we parse a notification
	     successfully, we don't mark the
	     REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN and let GDB blocked
	     there as before to get the sequence done.

	     2.1) --> vCont;c
	       GDB expects getting stop reply 'T05 thread:2'
	     2.2) <-- %Notif
	       <Don't mark the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN>
	     2.3) <-- T05 thread:2

	     These pending notifications can be processed later.  */
	  mark_async_event_handler (remote_async_get_pending_events_token);
	}

      if (notif_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "notif: Notification '%s' captured\n",
			    nc->name);
    }
}

/* Cleanup wrapper.  */

static void
do_notif_event_xfree (void *arg)
{
  struct notif_event *event = arg;

  if (event && event->dtr)
    event->dtr (event);

  xfree (event);
}

static void
notif_xfree (struct notif_client *notif)
{
  if (notif->pending_event != NULL
      && notif->pending_event->dtr != NULL)
    notif->pending_event->dtr (notif->pending_event);

  xfree (notif->pending_event);
  xfree (notif);
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_notif;

void
_initialize_notif (void)
{
  notif_queue = QUEUE_alloc (notif_client_p, notif_xfree);

  add_setshow_boolean_cmd ("notification", no_class, &notif_debug,
			   _("\
Set debugging of async remote notification."), _("\
Show debugging of async remote notification."), _("\
When non-zero, debugging output about async remote notifications"
" is enabled."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);
}
