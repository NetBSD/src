/* Event loop machinery for the remote server for GDB.
   Copyright (C) 1999-2017 Free Software Foundation, Inc.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* Based on src/gdb/event-loop.c.  */

#include "server.h"
#include "queue.h"

#include <sys/types.h>
#include "gdb_sys_time.h"

#ifdef USE_WIN32API
#include <windows.h>
#include <io.h>
#endif

#include <unistd.h>

typedef struct gdb_event gdb_event;
typedef int (event_handler_func) (gdb_fildes_t);

/* Tell create_file_handler what events we are interested in.  */

#define GDB_READABLE	(1<<1)
#define GDB_WRITABLE	(1<<2)
#define GDB_EXCEPTION	(1<<3)

/* Events are queued by calling 'QUEUE_enque (gdb_event_p, event_queue,
   file_event_ptr)' and serviced later
   on by do_one_event.  An event can be, for instance, a file
   descriptor becoming ready to be read.  Servicing an event simply
   means that the procedure PROC will be called.  We have 2 queues,
   one for file handlers that we listen to in the event loop, and one
   for the file handlers+events that are ready.  The procedure PROC
   associated with each event is always the same (handle_file_event).
   Its duty is to invoke the handler associated with the file
   descriptor whose state change generated the event, plus doing other
   cleanups and such.  */

typedef struct gdb_event
  {
    /* Procedure to call to service this event.  */
    event_handler_func *proc;

    /* File descriptor that is ready.  */
    gdb_fildes_t fd;
  } *gdb_event_p;

/* Information about each file descriptor we register with the event
   loop.  */

typedef struct file_handler
  {
    /* File descriptor.  */
    gdb_fildes_t fd;

    /* Events we want to monitor.  */
    int mask;

    /* Events that have been seen since the last time.  */
    int ready_mask;

    /* Procedure to call when fd is ready.  */
    handler_func *proc;

    /* Argument to pass to proc.  */
    gdb_client_data client_data;

    /* Was an error detected on this fd?  */
    int error;

    /* Next registered file descriptor.  */
    struct file_handler *next_file;
  }
file_handler;

DECLARE_QUEUE_P(gdb_event_p);
static QUEUE(gdb_event_p) *event_queue = NULL;
DEFINE_QUEUE_P(gdb_event_p);

/* Gdb_notifier is just a list of file descriptors gdb is interested
   in.  These are the input file descriptor, and the target file
   descriptor.  Each of the elements in the gdb_notifier list is
   basically a description of what kind of events gdb is interested
   in, for each fd.  */

static struct
  {
    /* Ptr to head of file handler list.  */
    file_handler *first_file_handler;

    /* Masks to be used in the next call to select.  Bits are set in
       response to calls to create_file_handler.  */
    fd_set check_masks[3];

    /* What file descriptors were found ready by select.  */
    fd_set ready_masks[3];

    /* Number of valid bits (highest fd value + 1). (for select) */
    int num_fds;
  }
gdb_notifier;

/* Callbacks are just routines that are executed before waiting for the
   next event.  In GDB this is struct gdb_timer.  We don't need timers
   so rather than copy all that complexity in gdbserver, we provide what
   we need, but we do so in a way that if/when the day comes that we need
   that complexity, it'll be easier to add - replace callbacks with timers
   and use a delta of zero (which is all gdb currently uses timers for anyway).

   PROC will be executed before gdbserver goes to sleep to wait for the
   next event.  */

struct callback_event
  {
    int id;
    callback_handler_func *proc;
    gdb_client_data data;
    struct callback_event *next;
  };

/* Table of registered callbacks.  */

static struct
  {
    struct callback_event *first;
    struct callback_event *last;

    /* Id of the last callback created.  */
    int num_callbacks;
  }
callback_list;

/* Free EVENT.  */

static void
gdb_event_xfree (struct gdb_event *event)
{
  xfree (event);
}

void
initialize_event_loop (void)
{
  event_queue = QUEUE_alloc (gdb_event_p, gdb_event_xfree);
}

/* Process one event.  If an event was processed, 1 is returned
   otherwise 0 is returned.  Scan the queue from head to tail,
   processing therefore the high priority events first, by invoking
   the associated event handler procedure.  */

static int
process_event (void)
{
  /* Let's get rid of the event from the event queue.  We need to
     do this now because while processing the event, since the
     proc function could end up jumping out to the caller of this
     function.  In that case, we would have on the event queue an
     event which has been processed, but not deleted.  */
  if (!QUEUE_is_empty (gdb_event_p, event_queue))
    {
      gdb_event *event_ptr = QUEUE_deque (gdb_event_p, event_queue);
      event_handler_func *proc = event_ptr->proc;
      gdb_fildes_t fd = event_ptr->fd;

      gdb_event_xfree (event_ptr);
      /* Now call the procedure associated with the event.  */
      if ((*proc) (fd))
	return -1;
      return 1;
    }

  /* This is the case if there are no event on the event queue.  */
  return 0;
}

/* Append PROC to the callback list.
   The result is the "id" of the callback that can be passed back to
   delete_callback_event.  */

int
append_callback_event (callback_handler_func *proc, gdb_client_data data)
{
  struct callback_event *event_ptr = XNEW (struct callback_event);

  event_ptr->id = callback_list.num_callbacks++;
  event_ptr->proc = proc;
  event_ptr->data = data;
  event_ptr->next = NULL;
  if (callback_list.first == NULL)
    callback_list.first = event_ptr;
  if (callback_list.last != NULL)
    callback_list.last->next = event_ptr;
  callback_list.last = event_ptr;
  return event_ptr->id;
}

/* Delete callback ID.
   It is not an error callback ID doesn't exist.  */

void
delete_callback_event (int id)
{
  struct callback_event **p;

  for (p = &callback_list.first; *p != NULL; p = &(*p)->next)
    {
      struct callback_event *event_ptr = *p;

      if (event_ptr->id == id)
	{
	  *p = event_ptr->next;
	  if (event_ptr == callback_list.last)
	    callback_list.last = NULL;
	  free (event_ptr);
	  break;
	}
    }
}

/* Run the next callback.
   The result is 1 if a callback was called and event processing
   should continue, -1 if the callback wants the event loop to exit,
   and 0 if there are no more callbacks.  */

static int
process_callback (void)
{
  struct callback_event *event_ptr;

  event_ptr = callback_list.first;
  if (event_ptr != NULL)
    {
      callback_handler_func *proc = event_ptr->proc;
      gdb_client_data data = event_ptr->data;

      /* Remove the event before calling PROC,
	 more events may get added by PROC.  */
      callback_list.first = event_ptr->next;
      if (callback_list.first == NULL)
	callback_list.last = NULL;
      free  (event_ptr);
      if ((*proc) (data))
	return -1;
      return 1;
    }

  return 0;
}

/* Add a file handler/descriptor to the list of descriptors we are
   interested in.  FD is the file descriptor for the file/stream to be
   listened to.  MASK is a combination of READABLE, WRITABLE,
   EXCEPTION.  PROC is the procedure that will be called when an event
   occurs for FD.  CLIENT_DATA is the argument to pass to PROC.  */

static void
create_file_handler (gdb_fildes_t fd, int mask, handler_func *proc,
		     gdb_client_data client_data)
{
  file_handler *file_ptr;

  /* Do we already have a file handler for this file? (We may be
     changing its associated procedure).  */
  for (file_ptr = gdb_notifier.first_file_handler;
       file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    if (file_ptr->fd == fd)
      break;

  /* It is a new file descriptor.  Add it to the list.  Otherwise,
     just change the data associated with it.  */
  if (file_ptr == NULL)
    {
      file_ptr = XNEW (struct file_handler);
      file_ptr->fd = fd;
      file_ptr->ready_mask = 0;
      file_ptr->next_file = gdb_notifier.first_file_handler;
      gdb_notifier.first_file_handler = file_ptr;

      if (mask & GDB_READABLE)
	FD_SET (fd, &gdb_notifier.check_masks[0]);
      else
	FD_CLR (fd, &gdb_notifier.check_masks[0]);

      if (mask & GDB_WRITABLE)
	FD_SET (fd, &gdb_notifier.check_masks[1]);
      else
	FD_CLR (fd, &gdb_notifier.check_masks[1]);

      if (mask & GDB_EXCEPTION)
	FD_SET (fd, &gdb_notifier.check_masks[2]);
      else
	FD_CLR (fd, &gdb_notifier.check_masks[2]);

      if (gdb_notifier.num_fds <= fd)
	gdb_notifier.num_fds = fd + 1;
    }

  file_ptr->proc = proc;
  file_ptr->client_data = client_data;
  file_ptr->mask = mask;
}

/* Wrapper function for create_file_handler.  */

void
add_file_handler (gdb_fildes_t fd,
		  handler_func *proc, gdb_client_data client_data)
{
  create_file_handler (fd, GDB_READABLE | GDB_EXCEPTION, proc, client_data);
}

/* Remove the file descriptor FD from the list of monitored fd's:
   i.e. we don't care anymore about events on the FD.  */

void
delete_file_handler (gdb_fildes_t fd)
{
  file_handler *file_ptr, *prev_ptr = NULL;
  int i;

  /* Find the entry for the given file. */

  for (file_ptr = gdb_notifier.first_file_handler;
       file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    if (file_ptr->fd == fd)
      break;

  if (file_ptr == NULL)
    return;

  if (file_ptr->mask & GDB_READABLE)
    FD_CLR (fd, &gdb_notifier.check_masks[0]);
  if (file_ptr->mask & GDB_WRITABLE)
    FD_CLR (fd, &gdb_notifier.check_masks[1]);
  if (file_ptr->mask & GDB_EXCEPTION)
    FD_CLR (fd, &gdb_notifier.check_masks[2]);

  /* Find current max fd.  */

  if ((fd + 1) == gdb_notifier.num_fds)
    {
      gdb_notifier.num_fds--;
      for (i = gdb_notifier.num_fds; i; i--)
	{
	  if (FD_ISSET (i - 1, &gdb_notifier.check_masks[0])
	      || FD_ISSET (i - 1, &gdb_notifier.check_masks[1])
	      || FD_ISSET (i - 1, &gdb_notifier.check_masks[2]))
	    break;
	}
      gdb_notifier.num_fds = i;
    }

  /* Deactivate the file descriptor, by clearing its mask, so that it
     will not fire again.  */

  file_ptr->mask = 0;

  /* Get rid of the file handler in the file handler list.  */
  if (file_ptr == gdb_notifier.first_file_handler)
    gdb_notifier.first_file_handler = file_ptr->next_file;
  else
    {
      for (prev_ptr = gdb_notifier.first_file_handler;
	   prev_ptr->next_file != file_ptr;
	   prev_ptr = prev_ptr->next_file)
	;
      prev_ptr->next_file = file_ptr->next_file;
    }
  free (file_ptr);
}

/* Handle the given event by calling the procedure associated to the
   corresponding file handler.  Called by process_event indirectly,
   through event_ptr->proc.  EVENT_FILE_DESC is file descriptor of the
   event in the front of the event queue.  */

static int
handle_file_event (gdb_fildes_t event_file_desc)
{
  file_handler *file_ptr;
  int mask;

  /* Search the file handler list to find one that matches the fd in
     the event.  */
  for (file_ptr = gdb_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    {
      if (file_ptr->fd == event_file_desc)
	{
	  /* See if the desired events (mask) match the received
	     events (ready_mask).  */

	  if (file_ptr->ready_mask & GDB_EXCEPTION)
	    {
	      warning ("Exception condition detected on fd %s",
		       pfildes (file_ptr->fd));
	      file_ptr->error = 1;
	    }
	  else
	    file_ptr->error = 0;
	  mask = file_ptr->ready_mask & file_ptr->mask;

	  /* Clear the received events for next time around.  */
	  file_ptr->ready_mask = 0;

	  /* If there was a match, then call the handler.  */
	  if (mask != 0)
	    {
	      if ((*file_ptr->proc) (file_ptr->error,
				     file_ptr->client_data) < 0)
		return -1;
	    }
	  break;
	}
    }

  return 0;
}

/* Create a file event, to be enqueued in the event queue for
   processing.  The procedure associated to this event is always
   handle_file_event, which will in turn invoke the one that was
   associated to FD when it was registered with the event loop.  */

static gdb_event *
create_file_event (gdb_fildes_t fd)
{
  gdb_event *file_event_ptr;

  file_event_ptr = XNEW (gdb_event);
  file_event_ptr->proc = handle_file_event;
  file_event_ptr->fd = fd;

  return file_event_ptr;
}

/* Called by do_one_event to wait for new events on the monitored file
   descriptors.  Queue file events as they are detected by the poll.
   If there are no events, this function will block in the call to
   select.  Return -1 if there are no files descriptors to monitor,
   otherwise return 0.  */

static int
wait_for_event (void)
{
  file_handler *file_ptr;
  int num_found = 0;

  /* Make sure all output is done before getting another event.  */
  fflush (stdout);
  fflush (stderr);

  if (gdb_notifier.num_fds == 0)
    return -1;

  gdb_notifier.ready_masks[0] = gdb_notifier.check_masks[0];
  gdb_notifier.ready_masks[1] = gdb_notifier.check_masks[1];
  gdb_notifier.ready_masks[2] = gdb_notifier.check_masks[2];
  num_found = select (gdb_notifier.num_fds,
		      &gdb_notifier.ready_masks[0],
		      &gdb_notifier.ready_masks[1],
		      &gdb_notifier.ready_masks[2],
		      NULL);

  /* Clear the masks after an error from select.  */
  if (num_found == -1)
    {
      FD_ZERO (&gdb_notifier.ready_masks[0]);
      FD_ZERO (&gdb_notifier.ready_masks[1]);
      FD_ZERO (&gdb_notifier.ready_masks[2]);
#ifdef EINTR
      /* Dont print anything if we got a signal, let gdb handle
	 it.  */
      if (errno != EINTR)
	perror_with_name ("select");
#endif
    }

  /* Enqueue all detected file events.  */

  for (file_ptr = gdb_notifier.first_file_handler;
       file_ptr != NULL && num_found > 0;
       file_ptr = file_ptr->next_file)
    {
      int mask = 0;

      if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[0]))
	mask |= GDB_READABLE;
      if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[1]))
	mask |= GDB_WRITABLE;
      if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[2]))
	mask |= GDB_EXCEPTION;

      if (!mask)
	continue;
      else
	num_found--;

      /* Enqueue an event only if this is still a new event for this
	 fd.  */

      if (file_ptr->ready_mask == 0)
	{
	  gdb_event *file_event_ptr = create_file_event (file_ptr->fd);

	  QUEUE_enque (gdb_event_p, event_queue, file_event_ptr);
	}
      file_ptr->ready_mask = mask;
    }

  return 0;
}

/* Start up the event loop.  This is the entry point to the event
   loop.  */

void
start_event_loop (void)
{
  /* Loop until there is nothing to do.  This is the entry point to
     the event loop engine.  If nothing is ready at this time, wait
     for something to happen (via wait_for_event), then process it.
     Return when there are no longer event sources to wait for.  */

  while (1)
    {
      /* Any events already waiting in the queue?  */
      int res = process_event ();

      /* Did the event handler want the event loop to stop?  */
      if (res == -1)
	return;

      if (res)
	continue;

      /* Process any queued callbacks before we go to sleep.  */
      res = process_callback ();

      /* Did the callback want the event loop to stop?  */
      if (res == -1)
	return;

      if (res)
	continue;

      /* Wait for a new event.  If wait_for_event returns -1, we
	 should get out because this means that there are no event
	 sources left.  This will make the event loop stop, and the
	 application exit.  */

      if (wait_for_event () < 0)
	return;
    }

  /* We are done with the event loop.  There are no more event sources
     to listen to.  So we exit gdbserver.  */
}
