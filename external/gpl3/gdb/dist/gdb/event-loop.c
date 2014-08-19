/* Event loop machinery for GDB, the GNU debugger.
   Copyright (C) 1999-2014 Free Software Foundation, Inc.
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
#include "event-loop.h"
#include "event-top.h"
#include "queue.h"

#ifdef HAVE_POLL
#if defined (HAVE_POLL_H)
#include <poll.h>
#elif defined (HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
#endif

#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "exceptions.h"
#include "gdb_assert.h"
#include "gdb_select.h"

/* Tell create_file_handler what events we are interested in.
   This is used by the select version of the event loop.  */

#define GDB_READABLE	(1<<1)
#define GDB_WRITABLE	(1<<2)
#define GDB_EXCEPTION	(1<<3)

/* Data point to pass to the event handler.  */
typedef union event_data
{
  void *ptr;
  int integer;
} event_data;

typedef struct gdb_event gdb_event;
typedef void (event_handler_func) (event_data);

/* Event for the GDB event system.  Events are queued by calling
   async_queue_event and serviced later on by gdb_do_one_event.  An
   event can be, for instance, a file descriptor becoming ready to be
   read.  Servicing an event simply means that the procedure PROC will
   be called.  We have 2 queues, one for file handlers that we listen
   to in the event loop, and one for the file handlers+events that are
   ready.  The procedure PROC associated with each event is dependant
   of the event source.  In the case of monitored file descriptors, it
   is always the same (handle_file_event).  Its duty is to invoke the
   handler associated with the file descriptor whose state change
   generated the event, plus doing other cleanups and such.  In the
   case of async signal handlers, it is
   invoke_async_signal_handler.  */

typedef struct gdb_event
  {
    /* Procedure to call to service this event.  */
    event_handler_func *proc;

    /* Data to pass to the event handler.  */
    event_data data;
  } *gdb_event_p;

/* Information about each file descriptor we register with the event
   loop.  */

typedef struct file_handler
  {
    int fd;			/* File descriptor.  */
    int mask;			/* Events we want to monitor: POLLIN, etc.  */
    int ready_mask;		/* Events that have been seen since
				   the last time.  */
    handler_func *proc;		/* Procedure to call when fd is ready.  */
    gdb_client_data client_data;	/* Argument to pass to proc.  */
    int error;			/* Was an error detected on this fd?  */
    struct file_handler *next_file;	/* Next registered file descriptor.  */
  }
file_handler;

/* PROC is a function to be invoked when the READY flag is set.  This
   happens when there has been a signal and the corresponding signal
   handler has 'triggered' this async_signal_handler for execution.
   The actual work to be done in response to a signal will be carried
   out by PROC at a later time, within process_event.  This provides a
   deferred execution of signal handlers.

   Async_init_signals takes care of setting up such an
   async_signal_handler for each interesting signal.  */

typedef struct async_signal_handler
  {
    int ready;			    /* If ready, call this handler
				       from the main event loop, using
				       invoke_async_handler.  */
    struct async_signal_handler *next_handler;	/* Ptr to next handler.  */
    sig_handler_func *proc;	    /* Function to call to do the work.  */
    gdb_client_data client_data;    /* Argument to async_handler_func.  */
  }
async_signal_handler;

/* PROC is a function to be invoked when the READY flag is set.  This
   happens when the event has been marked with
   MARK_ASYNC_EVENT_HANDLER.  The actual work to be done in response
   to an event will be carried out by PROC at a later time, within
   process_event.  This provides a deferred execution of event
   handlers.  */
typedef struct async_event_handler
  {
    /* If ready, call this handler from the main event loop, using
       invoke_event_handler.  */
    int ready;

    /* Point to next handler.  */
    struct async_event_handler *next_handler;

    /* Function to call to do the work.  */
    async_event_handler_func *proc;

    /* Argument to PROC.  */
    gdb_client_data client_data;
  }
async_event_handler;

DECLARE_QUEUE_P(gdb_event_p);
DEFINE_QUEUE_P(gdb_event_p);
static QUEUE(gdb_event_p) *event_queue = NULL;

/* Gdb_notifier is just a list of file descriptors gdb is interested in.
   These are the input file descriptor, and the target file
   descriptor.  We have two flavors of the notifier, one for platforms
   that have the POLL function, the other for those that don't, and
   only support SELECT.  Each of the elements in the gdb_notifier list is
   basically a description of what kind of events gdb is interested
   in, for each fd.  */

/* As of 1999-04-30 only the input file descriptor is registered with the
   event loop.  */

/* Do we use poll or select ? */
#ifdef HAVE_POLL
#define USE_POLL 1
#else
#define USE_POLL 0
#endif /* HAVE_POLL */

static unsigned char use_poll = USE_POLL;

#ifdef USE_WIN32API
#include <windows.h>
#include <io.h>
#endif

static struct
  {
    /* Ptr to head of file handler list.  */
    file_handler *first_file_handler;

#ifdef HAVE_POLL
    /* Ptr to array of pollfd structures.  */
    struct pollfd *poll_fds;

    /* Timeout in milliseconds for calls to poll().  */
    int poll_timeout;
#endif

    /* Masks to be used in the next call to select.
       Bits are set in response to calls to create_file_handler.  */
    fd_set check_masks[3];

    /* What file descriptors were found ready by select.  */
    fd_set ready_masks[3];

    /* Number of file descriptors to monitor (for poll).  */
    /* Number of valid bits (highest fd value + 1) (for select).  */
    int num_fds;

    /* Time structure for calls to select().  */
    struct timeval select_timeout;

    /* Flag to tell whether the timeout should be used.  */
    int timeout_valid;
  }
gdb_notifier;

/* Structure associated with a timer.  PROC will be executed at the
   first occasion after WHEN.  */
struct gdb_timer
  {
    struct timeval when;
    int timer_id;
    struct gdb_timer *next;
    timer_handler_func *proc;	    /* Function to call to do the work.  */
    gdb_client_data client_data;    /* Argument to async_handler_func.  */
  };

/* List of currently active timers.  It is sorted in order of
   increasing timers.  */
static struct
  {
    /* Pointer to first in timer list.  */
    struct gdb_timer *first_timer;

    /* Id of the last timer created.  */
    int num_timers;
  }
timer_list;

/* All the async_signal_handlers gdb is interested in are kept onto
   this list.  */
static struct
  {
    /* Pointer to first in handler list.  */
    async_signal_handler *first_handler;

    /* Pointer to last in handler list.  */
    async_signal_handler *last_handler;
  }
sighandler_list;

/* All the async_event_handlers gdb is interested in are kept onto
   this list.  */
static struct
  {
    /* Pointer to first in handler list.  */
    async_event_handler *first_handler;

    /* Pointer to last in handler list.  */
    async_event_handler *last_handler;
  }
async_event_handler_list;

static int invoke_async_signal_handlers (void);
static void create_file_handler (int fd, int mask, handler_func *proc,
				 gdb_client_data client_data);
static void handle_file_event (event_data data);
static void check_async_event_handlers (void);
static int gdb_wait_for_event (int);
static void poll_timers (void);


/* Create a generic event, to be enqueued in the event queue for
   processing.  PROC is the procedure associated to the event.  DATA
   is passed to PROC upon PROC invocation.  */

static gdb_event *
create_event (event_handler_func proc, event_data data)
{
  gdb_event *event;

  event = xmalloc (sizeof (*event));
  event->proc = proc;
  event->data = data;

  return event;
}

/* Create a file event, to be enqueued in the event queue for
   processing.  The procedure associated to this event is always
   handle_file_event, which will in turn invoke the one that was
   associated to FD when it was registered with the event loop.  */
static gdb_event *
create_file_event (int fd)
{
  event_data data;

  data.integer = fd;
  return create_event (handle_file_event, data);
}


/* Free EVENT.  */

static void
gdb_event_xfree (struct gdb_event *event)
{
  xfree (event);
}

/* Initialize the event queue.  */

void
initialize_event_loop (void)
{
  event_queue = QUEUE_alloc (gdb_event_p, gdb_event_xfree);
}

/* Process one event.
   The event can be the next one to be serviced in the event queue,
   or an asynchronous event handler can be invoked in response to
   the reception of a signal.
   If an event was processed (either way), 1 is returned otherwise
   0 is returned.
   Scan the queue from head to tail, processing therefore the high
   priority events first, by invoking the associated event handler
   procedure.  */
static int
process_event (void)
{
  /* First let's see if there are any asynchronous event handlers that
     are ready.  These would be the result of invoking any of the
     signal handlers.  */

  if (invoke_async_signal_handlers ())
    return 1;

  /* Look in the event queue to find an event that is ready
     to be processed.  */

  if (!QUEUE_is_empty (gdb_event_p, event_queue))
    {
      /* Let's get rid of the event from the event queue.  We need to
	 do this now because while processing the event, the proc
	 function could end up calling 'error' and therefore jump out
	 to the caller of this function, gdb_do_one_event.  In that
	 case, we would have on the event queue an event wich has been
	 processed, but not deleted.  */
      gdb_event *event_ptr = QUEUE_deque (gdb_event_p, event_queue);
      /* Call the handler for the event.  */
      event_handler_func *proc = event_ptr->proc;
      event_data data = event_ptr->data;

      gdb_event_xfree (event_ptr);

      /* Now call the procedure associated with the event.  */
      (*proc) (data);
      return 1;
    }

  /* This is the case if there are no event on the event queue.  */
  return 0;
}

/* Process one high level event.  If nothing is ready at this time,
   wait for something to happen (via gdb_wait_for_event), then process
   it.  Returns >0 if something was done otherwise returns <0 (this
   can happen if there are no event sources to wait for).  */

int
gdb_do_one_event (void)
{
  static int event_source_head = 0;
  const int number_of_sources = 3;
  int current = 0;

  /* Any events already waiting in the queue?  */
  if (process_event ())
    return 1;

  /* To level the fairness across event sources, we poll them in a
     round-robin fashion.  */
  for (current = 0; current < number_of_sources; current++)
    {
      switch (event_source_head)
	{
	case 0:
	  /* Are any timers that are ready? If so, put an event on the
	     queue.  */
	  poll_timers ();
	  break;
	case 1:
	  /* Are there events already waiting to be collected on the
	     monitored file descriptors?  */
	  gdb_wait_for_event (0);
	  break;
	case 2:
	  /* Are there any asynchronous event handlers ready?  */
	  check_async_event_handlers ();
	  break;
	}

      event_source_head++;
      if (event_source_head == number_of_sources)
	event_source_head = 0;
    }

  /* Handle any new events collected.  */
  if (process_event ())
    return 1;

  /* Block waiting for a new event.  If gdb_wait_for_event returns -1,
     we should get out because this means that there are no event
     sources left.  This will make the event loop stop, and the
     application exit.  */

  if (gdb_wait_for_event (1) < 0)
    return -1;

  /* Handle any new events occurred while waiting.  */
  if (process_event ())
    return 1;

  /* If gdb_wait_for_event has returned 1, it means that one event has
     been handled.  We break out of the loop.  */
  return 1;
}

/* Start up the event loop.  This is the entry point to the event loop
   from the command loop.  */

void
start_event_loop (void)
{
  /* Loop until there is nothing to do.  This is the entry point to
     the event loop engine.  gdb_do_one_event will process one event
     for each invocation.  It blocks waiting for an event and then
     processes it.  */
  while (1)
    {
      volatile struct gdb_exception ex;
      int result = 0;

      TRY_CATCH (ex, RETURN_MASK_ALL)
	{
	  result = gdb_do_one_event ();
	}
      if (ex.reason < 0)
	{
	  exception_print (gdb_stderr, ex);

	  /* If any exception escaped to here, we better enable
	     stdin.  Otherwise, any command that calls async_disable_stdin,
	     and then throws, will leave stdin inoperable.  */
	  async_enable_stdin ();
	  /* If we long-jumped out of do_one_event, we probably didn't
	     get around to resetting the prompt, which leaves readline
	     in a messed-up state.  Reset it here.  */
	  /* FIXME: this should really be a call to a hook that is
	     interface specific, because interfaces can display the
	     prompt in their own way.  */
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
	     whether display the prompt or not.  */
	}
      if (result < 0)
	break;
    }

  /* We are done with the event loop.  There are no more event sources
     to listen to.  So we exit GDB.  */
  return;
}


/* Wrapper function for create_file_handler, so that the caller
   doesn't have to know implementation details about the use of poll
   vs. select.  */
void
add_file_handler (int fd, handler_func * proc, gdb_client_data client_data)
{
#ifdef HAVE_POLL
  struct pollfd fds;
#endif

  if (use_poll)
    {
#ifdef HAVE_POLL
      /* Check to see if poll () is usable.  If not, we'll switch to
         use select.  This can happen on systems like
         m68k-motorola-sys, `poll' cannot be used to wait for `stdin'.
         On m68k-motorola-sysv, tty's are not stream-based and not
         `poll'able.  */
      fds.fd = fd;
      fds.events = POLLIN;
      if (poll (&fds, 1, 0) == 1 && (fds.revents & POLLNVAL))
	use_poll = 0;
#else
      internal_error (__FILE__, __LINE__,
		      _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
    }
  if (use_poll)
    {
#ifdef HAVE_POLL
      create_file_handler (fd, POLLIN, proc, client_data);
#else
      internal_error (__FILE__, __LINE__,
		      _("use_poll without HAVE_POLL"));
#endif
    }
  else
    create_file_handler (fd, GDB_READABLE | GDB_EXCEPTION, 
			 proc, client_data);
}

/* Add a file handler/descriptor to the list of descriptors we are
   interested in.

   FD is the file descriptor for the file/stream to be listened to.

   For the poll case, MASK is a combination (OR) of POLLIN,
   POLLRDNORM, POLLRDBAND, POLLPRI, POLLOUT, POLLWRNORM, POLLWRBAND:
   these are the events we are interested in.  If any of them occurs,
   proc should be called.

   For the select case, MASK is a combination of READABLE, WRITABLE,
   EXCEPTION.  PROC is the procedure that will be called when an event
   occurs for FD.  CLIENT_DATA is the argument to pass to PROC.  */

static void
create_file_handler (int fd, int mask, handler_func * proc, 
		     gdb_client_data client_data)
{
  file_handler *file_ptr;

  /* Do we already have a file handler for this file?  (We may be
     changing its associated procedure).  */
  for (file_ptr = gdb_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    {
      if (file_ptr->fd == fd)
	break;
    }

  /* It is a new file descriptor.  Add it to the list.  Otherwise, just
     change the data associated with it.  */
  if (file_ptr == NULL)
    {
      file_ptr = (file_handler *) xmalloc (sizeof (file_handler));
      file_ptr->fd = fd;
      file_ptr->ready_mask = 0;
      file_ptr->next_file = gdb_notifier.first_file_handler;
      gdb_notifier.first_file_handler = file_ptr;

      if (use_poll)
	{
#ifdef HAVE_POLL
	  gdb_notifier.num_fds++;
	  if (gdb_notifier.poll_fds)
	    gdb_notifier.poll_fds =
	      (struct pollfd *) xrealloc (gdb_notifier.poll_fds,
					  (gdb_notifier.num_fds
					   * sizeof (struct pollfd)));
	  else
	    gdb_notifier.poll_fds =
	      (struct pollfd *) xmalloc (sizeof (struct pollfd));
	  (gdb_notifier.poll_fds + gdb_notifier.num_fds - 1)->fd = fd;
	  (gdb_notifier.poll_fds + gdb_notifier.num_fds - 1)->events = mask;
	  (gdb_notifier.poll_fds + gdb_notifier.num_fds - 1)->revents = 0;
#else
	  internal_error (__FILE__, __LINE__,
			  _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
	}
      else
	{
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
    }

  file_ptr->proc = proc;
  file_ptr->client_data = client_data;
  file_ptr->mask = mask;
}

/* Remove the file descriptor FD from the list of monitored fd's: 
   i.e. we don't care anymore about events on the FD.  */
void
delete_file_handler (int fd)
{
  file_handler *file_ptr, *prev_ptr = NULL;
  int i;
#ifdef HAVE_POLL
  int j;
  struct pollfd *new_poll_fds;
#endif

  /* Find the entry for the given file.  */

  for (file_ptr = gdb_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    {
      if (file_ptr->fd == fd)
	break;
    }

  if (file_ptr == NULL)
    return;

  if (use_poll)
    {
#ifdef HAVE_POLL
      /* Create a new poll_fds array by copying every fd's information
         but the one we want to get rid of.  */

      new_poll_fds = (struct pollfd *) 
	xmalloc ((gdb_notifier.num_fds - 1) * sizeof (struct pollfd));

      for (i = 0, j = 0; i < gdb_notifier.num_fds; i++)
	{
	  if ((gdb_notifier.poll_fds + i)->fd != fd)
	    {
	      (new_poll_fds + j)->fd = (gdb_notifier.poll_fds + i)->fd;
	      (new_poll_fds + j)->events = (gdb_notifier.poll_fds + i)->events;
	      (new_poll_fds + j)->revents
		= (gdb_notifier.poll_fds + i)->revents;
	      j++;
	    }
	}
      xfree (gdb_notifier.poll_fds);
      gdb_notifier.poll_fds = new_poll_fds;
      gdb_notifier.num_fds--;
#else
      internal_error (__FILE__, __LINE__,
		      _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
    }
  else
    {
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
    }

  /* Deactivate the file descriptor, by clearing its mask, 
     so that it will not fire again.  */

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
  xfree (file_ptr);
}

/* Handle the given event by calling the procedure associated to the
   corresponding file handler.  Called by process_event indirectly,
   through event_ptr->proc.  EVENT_FILE_DESC is file descriptor of the
   event in the front of the event queue.  */
static void
handle_file_event (event_data data)
{
  file_handler *file_ptr;
  int mask;
#ifdef HAVE_POLL
  int error_mask;
#endif
  int event_file_desc = data.integer;

  /* Search the file handler list to find one that matches the fd in
     the event.  */
  for (file_ptr = gdb_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    {
      if (file_ptr->fd == event_file_desc)
	{
	  /* With poll, the ready_mask could have any of three events
	     set to 1: POLLHUP, POLLERR, POLLNVAL.  These events
	     cannot be used in the requested event mask (events), but
	     they can be returned in the return mask (revents).  We
	     need to check for those event too, and add them to the
	     mask which will be passed to the handler.  */

	  /* See if the desired events (mask) match the received
	     events (ready_mask).  */

	  if (use_poll)
	    {
#ifdef HAVE_POLL
	      /* POLLHUP means EOF, but can be combined with POLLIN to
		 signal more data to read.  */
	      error_mask = POLLHUP | POLLERR | POLLNVAL;
	      mask = file_ptr->ready_mask & (file_ptr->mask | error_mask);

	      if ((mask & (POLLERR | POLLNVAL)) != 0)
		{
		  /* Work in progress.  We may need to tell somebody
		     what kind of error we had.  */
		  if (mask & POLLERR)
		    printf_unfiltered (_("Error detected on fd %d\n"),
				       file_ptr->fd);
		  if (mask & POLLNVAL)
		    printf_unfiltered (_("Invalid or non-`poll'able fd %d\n"),
				       file_ptr->fd);
		  file_ptr->error = 1;
		}
	      else
		file_ptr->error = 0;
#else
	      internal_error (__FILE__, __LINE__,
			      _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
	    }
	  else
	    {
	      if (file_ptr->ready_mask & GDB_EXCEPTION)
		{
		  printf_unfiltered (_("Exception condition detected "
				       "on fd %d\n"), file_ptr->fd);
		  file_ptr->error = 1;
		}
	      else
		file_ptr->error = 0;
	      mask = file_ptr->ready_mask & file_ptr->mask;
	    }

	  /* Clear the received events for next time around.  */
	  file_ptr->ready_mask = 0;

	  /* If there was a match, then call the handler.  */
	  if (mask != 0)
	    (*file_ptr->proc) (file_ptr->error, file_ptr->client_data);
	  break;
	}
    }
}

/* Called by gdb_do_one_event to wait for new events on the monitored
   file descriptors.  Queue file events as they are detected by the
   poll.  If BLOCK and if there are no events, this function will
   block in the call to poll.  Return -1 if there are no file
   descriptors to monitor, otherwise return 0.  */
static int
gdb_wait_for_event (int block)
{
  file_handler *file_ptr;
  gdb_event *file_event_ptr;
  int num_found = 0;
  int i;

  /* Make sure all output is done before getting another event.  */
  gdb_flush (gdb_stdout);
  gdb_flush (gdb_stderr);

  if (gdb_notifier.num_fds == 0)
    return -1;

  if (use_poll)
    {
#ifdef HAVE_POLL
      int timeout;

      if (block)
	timeout = gdb_notifier.timeout_valid ? gdb_notifier.poll_timeout : -1;
      else
	timeout = 0;

      num_found = poll (gdb_notifier.poll_fds,
			(unsigned long) gdb_notifier.num_fds, timeout);

      /* Don't print anything if we get out of poll because of a
	 signal.  */
      if (num_found == -1 && errno != EINTR)
	perror_with_name (("poll"));
#else
      internal_error (__FILE__, __LINE__,
		      _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
    }
  else
    {
      struct timeval select_timeout;
      struct timeval *timeout_p;

      if (block)
	timeout_p = gdb_notifier.timeout_valid
	  ? &gdb_notifier.select_timeout : NULL;
      else
	{
	  memset (&select_timeout, 0, sizeof (select_timeout));
	  timeout_p = &select_timeout;
	}

      gdb_notifier.ready_masks[0] = gdb_notifier.check_masks[0];
      gdb_notifier.ready_masks[1] = gdb_notifier.check_masks[1];
      gdb_notifier.ready_masks[2] = gdb_notifier.check_masks[2];
      num_found = gdb_select (gdb_notifier.num_fds,
			      &gdb_notifier.ready_masks[0],
			      &gdb_notifier.ready_masks[1],
			      &gdb_notifier.ready_masks[2],
			      timeout_p);

      /* Clear the masks after an error from select.  */
      if (num_found == -1)
	{
	  FD_ZERO (&gdb_notifier.ready_masks[0]);
	  FD_ZERO (&gdb_notifier.ready_masks[1]);
	  FD_ZERO (&gdb_notifier.ready_masks[2]);

	  /* Dont print anything if we got a signal, let gdb handle
	     it.  */
	  if (errno != EINTR)
	    perror_with_name (("select"));
	}
    }

  /* Enqueue all detected file events.  */

  if (use_poll)
    {
#ifdef HAVE_POLL
      for (i = 0; (i < gdb_notifier.num_fds) && (num_found > 0); i++)
	{
	  if ((gdb_notifier.poll_fds + i)->revents)
	    num_found--;
	  else
	    continue;

	  for (file_ptr = gdb_notifier.first_file_handler;
	       file_ptr != NULL;
	       file_ptr = file_ptr->next_file)
	    {
	      if (file_ptr->fd == (gdb_notifier.poll_fds + i)->fd)
		break;
	    }

	  if (file_ptr)
	    {
	      /* Enqueue an event only if this is still a new event for
	         this fd.  */
	      if (file_ptr->ready_mask == 0)
		{
		  file_event_ptr = create_file_event (file_ptr->fd);
		  QUEUE_enque (gdb_event_p, event_queue, file_event_ptr);
		}
	      file_ptr->ready_mask = (gdb_notifier.poll_fds + i)->revents;
	    }
	}
#else
      internal_error (__FILE__, __LINE__,
		      _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
    }
  else
    {
      for (file_ptr = gdb_notifier.first_file_handler;
	   (file_ptr != NULL) && (num_found > 0);
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

	  /* Enqueue an event only if this is still a new event for
	     this fd.  */

	  if (file_ptr->ready_mask == 0)
	    {
	      file_event_ptr = create_file_event (file_ptr->fd);
	      QUEUE_enque (gdb_event_p, event_queue, file_event_ptr);
	    }
	  file_ptr->ready_mask = mask;
	}
    }
  return 0;
}


/* Create an asynchronous handler, allocating memory for it.
   Return a pointer to the newly created handler.
   This pointer will be used to invoke the handler by 
   invoke_async_signal_handler.
   PROC is the function to call with CLIENT_DATA argument 
   whenever the handler is invoked.  */
async_signal_handler *
create_async_signal_handler (sig_handler_func * proc,
			     gdb_client_data client_data)
{
  async_signal_handler *async_handler_ptr;

  async_handler_ptr =
    (async_signal_handler *) xmalloc (sizeof (async_signal_handler));
  async_handler_ptr->ready = 0;
  async_handler_ptr->next_handler = NULL;
  async_handler_ptr->proc = proc;
  async_handler_ptr->client_data = client_data;
  if (sighandler_list.first_handler == NULL)
    sighandler_list.first_handler = async_handler_ptr;
  else
    sighandler_list.last_handler->next_handler = async_handler_ptr;
  sighandler_list.last_handler = async_handler_ptr;
  return async_handler_ptr;
}

/* Call the handler from HANDLER immediately.  This function runs
   signal handlers when returning to the event loop would be too
   slow.  */
void
call_async_signal_handler (struct async_signal_handler *handler)
{
  (*handler->proc) (handler->client_data);
}

/* Mark the handler (ASYNC_HANDLER_PTR) as ready.  This information
   will be used when the handlers are invoked, after we have waited
   for some event.  The caller of this function is the interrupt
   handler associated with a signal.  */
void
mark_async_signal_handler (async_signal_handler * async_handler_ptr)
{
  async_handler_ptr->ready = 1;
}

/* Call all the handlers that are ready.  Returns true if any was
   indeed ready.  */
static int
invoke_async_signal_handlers (void)
{
  async_signal_handler *async_handler_ptr;
  int any_ready = 0;

  /* Invoke ready handlers.  */

  while (1)
    {
      for (async_handler_ptr = sighandler_list.first_handler;
	   async_handler_ptr != NULL;
	   async_handler_ptr = async_handler_ptr->next_handler)
	{
	  if (async_handler_ptr->ready)
	    break;
	}
      if (async_handler_ptr == NULL)
	break;
      any_ready = 1;
      async_handler_ptr->ready = 0;
      (*async_handler_ptr->proc) (async_handler_ptr->client_data);
    }

  return any_ready;
}

/* Delete an asynchronous handler (ASYNC_HANDLER_PTR).
   Free the space allocated for it.  */
void
delete_async_signal_handler (async_signal_handler ** async_handler_ptr)
{
  async_signal_handler *prev_ptr;

  if (sighandler_list.first_handler == (*async_handler_ptr))
    {
      sighandler_list.first_handler = (*async_handler_ptr)->next_handler;
      if (sighandler_list.first_handler == NULL)
	sighandler_list.last_handler = NULL;
    }
  else
    {
      prev_ptr = sighandler_list.first_handler;
      while (prev_ptr && prev_ptr->next_handler != (*async_handler_ptr))
	prev_ptr = prev_ptr->next_handler;
      gdb_assert (prev_ptr);
      prev_ptr->next_handler = (*async_handler_ptr)->next_handler;
      if (sighandler_list.last_handler == (*async_handler_ptr))
	sighandler_list.last_handler = prev_ptr;
    }
  xfree ((*async_handler_ptr));
  (*async_handler_ptr) = NULL;
}

/* Create an asynchronous event handler, allocating memory for it.
   Return a pointer to the newly created handler.  PROC is the
   function to call with CLIENT_DATA argument whenever the handler is
   invoked.  */
async_event_handler *
create_async_event_handler (async_event_handler_func *proc,
			    gdb_client_data client_data)
{
  async_event_handler *h;

  h = xmalloc (sizeof (*h));
  h->ready = 0;
  h->next_handler = NULL;
  h->proc = proc;
  h->client_data = client_data;
  if (async_event_handler_list.first_handler == NULL)
    async_event_handler_list.first_handler = h;
  else
    async_event_handler_list.last_handler->next_handler = h;
  async_event_handler_list.last_handler = h;
  return h;
}

/* Mark the handler (ASYNC_HANDLER_PTR) as ready.  This information
   will be used by gdb_do_one_event.  The caller will be whoever
   created the event source, and wants to signal that the event is
   ready to be handled.  */
void
mark_async_event_handler (async_event_handler *async_handler_ptr)
{
  async_handler_ptr->ready = 1;
}

struct async_event_handler_data
{
  async_event_handler_func* proc;
  gdb_client_data client_data;
};

static void
invoke_async_event_handler (event_data data)
{
  struct async_event_handler_data *hdata = data.ptr;
  async_event_handler_func* proc = hdata->proc;
  gdb_client_data client_data = hdata->client_data;

  xfree (hdata);
  (*proc) (client_data);
}

/* Check if any asynchronous event handlers are ready, and queue
   events in the ready queue for any that are.  */
static void
check_async_event_handlers (void)
{
  async_event_handler *async_handler_ptr;
  struct async_event_handler_data *hdata;
  struct gdb_event *event_ptr;
  event_data data;

  for (async_handler_ptr = async_event_handler_list.first_handler;
       async_handler_ptr != NULL;
       async_handler_ptr = async_handler_ptr->next_handler)
    {
      if (async_handler_ptr->ready)
	{
	  async_handler_ptr->ready = 0;

	  hdata = xmalloc (sizeof (*hdata));

	  hdata->proc = async_handler_ptr->proc;
	  hdata->client_data = async_handler_ptr->client_data;

	  data.ptr = hdata;

	  event_ptr = create_event (invoke_async_event_handler, data);
	  QUEUE_enque (gdb_event_p, event_queue, event_ptr);
	}
    }
}

/* Delete an asynchronous handler (ASYNC_HANDLER_PTR).
   Free the space allocated for it.  */
void
delete_async_event_handler (async_event_handler **async_handler_ptr)
{
  async_event_handler *prev_ptr;

  if (async_event_handler_list.first_handler == *async_handler_ptr)
    {
      async_event_handler_list.first_handler
	= (*async_handler_ptr)->next_handler;
      if (async_event_handler_list.first_handler == NULL)
	async_event_handler_list.last_handler = NULL;
    }
  else
    {
      prev_ptr = async_event_handler_list.first_handler;
      while (prev_ptr && prev_ptr->next_handler != *async_handler_ptr)
	prev_ptr = prev_ptr->next_handler;
      gdb_assert (prev_ptr);
      prev_ptr->next_handler = (*async_handler_ptr)->next_handler;
      if (async_event_handler_list.last_handler == (*async_handler_ptr))
	async_event_handler_list.last_handler = prev_ptr;
    }
  xfree (*async_handler_ptr);
  *async_handler_ptr = NULL;
}

/* Create a timer that will expire in MILLISECONDS from now.  When the
   timer is ready, PROC will be executed.  At creation, the timer is
   aded to the timers queue.  This queue is kept sorted in order of
   increasing timers.  Return a handle to the timer struct.  */
int
create_timer (int milliseconds, timer_handler_func * proc, 
	      gdb_client_data client_data)
{
  struct gdb_timer *timer_ptr, *timer_index, *prev_timer;
  struct timeval time_now, delta;

  /* Compute seconds.  */
  delta.tv_sec = milliseconds / 1000;
  /* Compute microseconds.  */
  delta.tv_usec = (milliseconds % 1000) * 1000;

  gettimeofday (&time_now, NULL);

  timer_ptr = (struct gdb_timer *) xmalloc (sizeof (*timer_ptr));
  timer_ptr->when.tv_sec = time_now.tv_sec + delta.tv_sec;
  timer_ptr->when.tv_usec = time_now.tv_usec + delta.tv_usec;
  /* Carry?  */
  if (timer_ptr->when.tv_usec >= 1000000)
    {
      timer_ptr->when.tv_sec += 1;
      timer_ptr->when.tv_usec -= 1000000;
    }
  timer_ptr->proc = proc;
  timer_ptr->client_data = client_data;
  timer_list.num_timers++;
  timer_ptr->timer_id = timer_list.num_timers;

  /* Now add the timer to the timer queue, making sure it is sorted in
     increasing order of expiration.  */

  for (timer_index = timer_list.first_timer;
       timer_index != NULL;
       timer_index = timer_index->next)
    {
      /* If the seconds field is greater or if it is the same, but the
         microsecond field is greater.  */
      if ((timer_index->when.tv_sec > timer_ptr->when.tv_sec)
	  || ((timer_index->when.tv_sec == timer_ptr->when.tv_sec)
	      && (timer_index->when.tv_usec > timer_ptr->when.tv_usec)))
	break;
    }

  if (timer_index == timer_list.first_timer)
    {
      timer_ptr->next = timer_list.first_timer;
      timer_list.first_timer = timer_ptr;

    }
  else
    {
      for (prev_timer = timer_list.first_timer;
	   prev_timer->next != timer_index;
	   prev_timer = prev_timer->next)
	;

      prev_timer->next = timer_ptr;
      timer_ptr->next = timer_index;
    }

  gdb_notifier.timeout_valid = 0;
  return timer_ptr->timer_id;
}

/* There is a chance that the creator of the timer wants to get rid of
   it before it expires.  */
void
delete_timer (int id)
{
  struct gdb_timer *timer_ptr, *prev_timer = NULL;

  /* Find the entry for the given timer.  */

  for (timer_ptr = timer_list.first_timer; timer_ptr != NULL;
       timer_ptr = timer_ptr->next)
    {
      if (timer_ptr->timer_id == id)
	break;
    }

  if (timer_ptr == NULL)
    return;
  /* Get rid of the timer in the timer list.  */
  if (timer_ptr == timer_list.first_timer)
    timer_list.first_timer = timer_ptr->next;
  else
    {
      for (prev_timer = timer_list.first_timer;
	   prev_timer->next != timer_ptr;
	   prev_timer = prev_timer->next)
	;
      prev_timer->next = timer_ptr->next;
    }
  xfree (timer_ptr);

  gdb_notifier.timeout_valid = 0;
}

/* When a timer event is put on the event queue, it will be handled by
   this function.  Just call the associated procedure and delete the
   timer event from the event queue.  Repeat this for each timer that
   has expired.  */
static void
handle_timer_event (event_data dummy)
{
  struct timeval time_now;
  struct gdb_timer *timer_ptr, *saved_timer;

  gettimeofday (&time_now, NULL);
  timer_ptr = timer_list.first_timer;

  while (timer_ptr != NULL)
    {
      if ((timer_ptr->when.tv_sec > time_now.tv_sec)
	  || ((timer_ptr->when.tv_sec == time_now.tv_sec)
	      && (timer_ptr->when.tv_usec > time_now.tv_usec)))
	break;

      /* Get rid of the timer from the beginning of the list.  */
      timer_list.first_timer = timer_ptr->next;
      saved_timer = timer_ptr;
      timer_ptr = timer_ptr->next;
      /* Call the procedure associated with that timer.  */
      (*saved_timer->proc) (saved_timer->client_data);
      xfree (saved_timer);
    }

  gdb_notifier.timeout_valid = 0;
}

/* Check whether any timers in the timers queue are ready.  If at least
   one timer is ready, stick an event onto the event queue.  Even in
   case more than one timer is ready, one event is enough, because the
   handle_timer_event() will go through the timers list and call the
   procedures associated with all that have expired.l Update the
   timeout for the select() or poll() as well.  */
static void
poll_timers (void)
{
  struct timeval time_now, delta;
  gdb_event *event_ptr;

  if (timer_list.first_timer != NULL)
    {
      gettimeofday (&time_now, NULL);
      delta.tv_sec = timer_list.first_timer->when.tv_sec - time_now.tv_sec;
      delta.tv_usec = timer_list.first_timer->when.tv_usec - time_now.tv_usec;
      /* Borrow?  */
      if (delta.tv_usec < 0)
	{
	  delta.tv_sec -= 1;
	  delta.tv_usec += 1000000;
	}

      /* Oops it expired already.  Tell select / poll to return
         immediately.  (Cannot simply test if delta.tv_sec is negative
         because time_t might be unsigned.)  */
      if (timer_list.first_timer->when.tv_sec < time_now.tv_sec
	  || (timer_list.first_timer->when.tv_sec == time_now.tv_sec
	      && timer_list.first_timer->when.tv_usec < time_now.tv_usec))
	{
	  delta.tv_sec = 0;
	  delta.tv_usec = 0;
	}

      if (delta.tv_sec == 0 && delta.tv_usec == 0)
	{
	  event_ptr = (gdb_event *) xmalloc (sizeof (gdb_event));
	  event_ptr->proc = handle_timer_event;
	  event_ptr->data.integer = timer_list.first_timer->timer_id;
	  QUEUE_enque (gdb_event_p, event_queue, event_ptr);
	}

      /* Now we need to update the timeout for select/ poll, because
         we don't want to sit there while this timer is expiring.  */
      if (use_poll)
	{
#ifdef HAVE_POLL
	  gdb_notifier.poll_timeout = delta.tv_sec * 1000;
#else
	  internal_error (__FILE__, __LINE__,
			  _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
	}
      else
	{
	  gdb_notifier.select_timeout.tv_sec = delta.tv_sec;
	  gdb_notifier.select_timeout.tv_usec = delta.tv_usec;
	}
      gdb_notifier.timeout_valid = 1;
    }
  else
    gdb_notifier.timeout_valid = 0;
}
