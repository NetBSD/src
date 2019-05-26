/* Event loop machinery for GDB, the GNU debugger.
   Copyright (C) 1999-2017 Free Software Foundation, Inc.
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
#include "ser-event.h"

#ifdef HAVE_POLL
#if defined (HAVE_POLL_H)
#include <poll.h>
#elif defined (HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
#endif

#include <sys/types.h>
#include "gdb_sys_time.h"
#include "gdb_select.h"
#include "observer.h"
#include "top.h"

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

    /* Next file handler to handle, for the select variant.  To level
       the fairness across event sources, we serve file handlers in a
       round-robin-like fashion.  The number and order of the polled
       file handlers may change between invocations, but this is good
       enough.  */
    file_handler *next_file_handler;

#ifdef HAVE_POLL
    /* Ptr to array of pollfd structures.  */
    struct pollfd *poll_fds;

    /* Next file descriptor to handle, for the poll variant.  To level
       the fairness across event sources, we poll the file descriptors
       in a round-robin-like fashion.  The number and order of the
       polled file descriptors may change between invocations, but
       this is good enough.  */
    int next_poll_fds_index;

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
    std::chrono::steady_clock::time_point when;
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
static int check_async_event_handlers (void);
static int gdb_wait_for_event (int);
static int update_wait_timeout (void);
static int poll_timers (void);


/* This event is signalled whenever an asynchronous handler needs to
   defer an action to the event loop.  */
static struct serial_event *async_signal_handlers_serial_event;

/* Callback registered with ASYNC_SIGNAL_HANDLERS_SERIAL_EVENT.  */

static void
async_signals_handler (int error, gdb_client_data client_data)
{
  /* Do nothing.  Handlers are run by invoke_async_signal_handlers
     from instead.  */
}

void
initialize_async_signal_handlers (void)
{
  async_signal_handlers_serial_event = make_serial_event ();

  add_file_handler (serial_event_fd (async_signal_handlers_serial_event),
		    async_signals_handler, NULL);
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

  /* First let's see if there are any asynchronous signal handlers
     that are ready.  These would be the result of invoking any of the
     signal handlers.  */
  if (invoke_async_signal_handlers ())
    return 1;

  /* To level the fairness across event sources, we poll them in a
     round-robin fashion.  */
  for (current = 0; current < number_of_sources; current++)
    {
      int res;

      switch (event_source_head)
	{
	case 0:
	  /* Are any timers that are ready?  */
	  res = poll_timers ();
	  break;
	case 1:
	  /* Are there events already waiting to be collected on the
	     monitored file descriptors?  */
	  res = gdb_wait_for_event (0);
	  break;
	case 2:
	  /* Are there any asynchronous event handlers ready?  */
	  res = check_async_event_handlers ();
	  break;
	default:
	  internal_error (__FILE__, __LINE__,
			  "unexpected event_source_head %d",
			  event_source_head);
	}

      event_source_head++;
      if (event_source_head == number_of_sources)
	event_source_head = 0;

      if (res > 0)
	return 1;
    }

  /* Block waiting for a new event.  If gdb_wait_for_event returns -1,
     we should get out because this means that there are no event
     sources left.  This will make the event loop stop, and the
     application exit.  */

  if (gdb_wait_for_event (1) < 0)
    return -1;

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
      int result = 0;

      TRY
	{
	  result = gdb_do_one_event ();
	}
      CATCH (ex, RETURN_MASK_ALL)
	{
	  exception_print (gdb_stderr, ex);

	  /* If any exception escaped to here, we better enable
	     stdin.  Otherwise, any command that calls async_disable_stdin,
	     and then throws, will leave stdin inoperable.  */
	  async_enable_stdin ();
	  /* If we long-jumped out of do_one_event, we probably didn't
	     get around to resetting the prompt, which leaves readline
	     in a messed-up state.  Reset it here.  */
	  current_ui->prompt_state = PROMPT_NEEDED;
	  observer_notify_command_error ();
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
      END_CATCH

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
      file_ptr = XNEW (file_handler);
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
	      XNEW (struct pollfd);
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

/* Return the next file handler to handle, and advance to the next
   file handler, wrapping around if the end of the list is
   reached.  */

static file_handler *
get_next_file_handler_to_handle_and_advance (void)
{
  file_handler *curr_next;

  /* The first time around, this is still NULL.  */
  if (gdb_notifier.next_file_handler == NULL)
    gdb_notifier.next_file_handler = gdb_notifier.first_file_handler;

  curr_next = gdb_notifier.next_file_handler;
  gdb_assert (curr_next != NULL);

  /* Advance.  */
  gdb_notifier.next_file_handler = curr_next->next_file;
  /* Wrap around, if necessary.  */
  if (gdb_notifier.next_file_handler == NULL)
    gdb_notifier.next_file_handler = gdb_notifier.first_file_handler;

  return curr_next;
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

  /* If this file handler was going to be the next one to be handled,
     advance to the next's next, if any.  */
  if (gdb_notifier.next_file_handler == file_ptr)
    {
      if (file_ptr->next_file == NULL
	  && file_ptr == gdb_notifier.first_file_handler)
	gdb_notifier.next_file_handler = NULL;
      else
	get_next_file_handler_to_handle_and_advance ();
    }

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
   corresponding file handler.  */

static void
handle_file_event (file_handler *file_ptr, int ready_mask)
{
  int mask;
#ifdef HAVE_POLL
  int error_mask;
#endif

    {
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
	      mask = ready_mask & (file_ptr->mask | error_mask);

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
	      if (ready_mask & GDB_EXCEPTION)
		{
		  printf_unfiltered (_("Exception condition detected "
				       "on fd %d\n"), file_ptr->fd);
		  file_ptr->error = 1;
		}
	      else
		file_ptr->error = 0;
	      mask = ready_mask & file_ptr->mask;
	    }

	  /* If there was a match, then call the handler.  */
	  if (mask != 0)
	    (*file_ptr->proc) (file_ptr->error, file_ptr->client_data);
	}
    }
}

/* Wait for new events on the monitored file descriptors.  Run the
   event handler if the first descriptor that is detected by the poll.
   If BLOCK and if there are no events, this function will block in
   the call to poll.  Return 1 if an event was handled.  Return -1 if
   there are no file descriptors to monitor.  Return 1 if an event was
   handled, otherwise returns 0.  */

static int
gdb_wait_for_event (int block)
{
  file_handler *file_ptr;
  int num_found = 0;

  /* Make sure all output is done before getting another event.  */
  gdb_flush (gdb_stdout);
  gdb_flush (gdb_stderr);

  if (gdb_notifier.num_fds == 0)
    return -1;

  if (block)
    update_wait_timeout ();

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

  /* Avoid looking at poll_fds[i]->revents if no event fired.  */
  if (num_found <= 0)
    return 0;

  /* Run event handlers.  We always run just one handler and go back
     to polling, in case a handler changes the notifier list.  Since
     events for sources we haven't consumed yet wake poll/select
     immediately, no event is lost.  */

  /* To level the fairness across event descriptors, we handle them in
     a round-robin-like fashion.  The number and order of descriptors
     may change between invocations, but this is good enough.  */
  if (use_poll)
    {
#ifdef HAVE_POLL
      int i;
      int mask;

      while (1)
	{
	  if (gdb_notifier.next_poll_fds_index >= gdb_notifier.num_fds)
	    gdb_notifier.next_poll_fds_index = 0;
	  i = gdb_notifier.next_poll_fds_index++;

	  gdb_assert (i < gdb_notifier.num_fds);
	  if ((gdb_notifier.poll_fds + i)->revents)
	    break;
	}

      for (file_ptr = gdb_notifier.first_file_handler;
	   file_ptr != NULL;
	   file_ptr = file_ptr->next_file)
	{
	  if (file_ptr->fd == (gdb_notifier.poll_fds + i)->fd)
	    break;
	}
      gdb_assert (file_ptr != NULL);

      mask = (gdb_notifier.poll_fds + i)->revents;
      handle_file_event (file_ptr, mask);
      return 1;
#else
      internal_error (__FILE__, __LINE__,
		      _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
    }
  else
    {
      /* See comment about even source fairness above.  */
      int mask = 0;

      do
	{
	  file_ptr = get_next_file_handler_to_handle_and_advance ();

	  if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[0]))
	    mask |= GDB_READABLE;
	  if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[1]))
	    mask |= GDB_WRITABLE;
	  if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[2]))
	    mask |= GDB_EXCEPTION;
	}
      while (mask == 0);

      handle_file_event (file_ptr, mask);
      return 1;
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

  async_handler_ptr = XNEW (async_signal_handler);
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

/* Mark the handler (ASYNC_HANDLER_PTR) as ready.  This information
   will be used when the handlers are invoked, after we have waited
   for some event.  The caller of this function is the interrupt
   handler associated with a signal.  */
void
mark_async_signal_handler (async_signal_handler * async_handler_ptr)
{
  async_handler_ptr->ready = 1;
  serial_event_set (async_signal_handlers_serial_event);
}

/* See event-loop.h.  */

void
clear_async_signal_handler (async_signal_handler *async_handler_ptr)
{
  async_handler_ptr->ready = 0;
}

/* See event-loop.h.  */

int
async_signal_handler_is_marked (async_signal_handler *async_handler_ptr)
{
  return async_handler_ptr->ready;
}

/* Call all the handlers that are ready.  Returns true if any was
   indeed ready.  */

static int
invoke_async_signal_handlers (void)
{
  async_signal_handler *async_handler_ptr;
  int any_ready = 0;

  /* We're going to handle all pending signals, so no need to wake up
     the event loop again the next time around.  Note this must be
     cleared _before_ calling the callbacks, to avoid races.  */
  serial_event_clear (async_signal_handlers_serial_event);

  /* Invoke all ready handlers.  */

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
      /* Async signal handlers have no connection to whichever was the
	 current UI, and thus always run on the main one.  */
      current_ui = main_ui;
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

  h = XNEW (struct async_event_handler);
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

/* See event-loop.h.  */

void
clear_async_event_handler (async_event_handler *async_handler_ptr)
{
  async_handler_ptr->ready = 0;
}

/* Check if asynchronous event handlers are ready, and call the
   handler function for one that is.  */

static int
check_async_event_handlers (void)
{
  async_event_handler *async_handler_ptr;

  for (async_handler_ptr = async_event_handler_list.first_handler;
       async_handler_ptr != NULL;
       async_handler_ptr = async_handler_ptr->next_handler)
    {
      if (async_handler_ptr->ready)
	{
	  async_handler_ptr->ready = 0;
	  (*async_handler_ptr->proc) (async_handler_ptr->client_data);
	  return 1;
	}
    }

  return 0;
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

/* Create a timer that will expire in MS milliseconds from now.  When
   the timer is ready, PROC will be executed.  At creation, the timer
   is added to the timers queue.  This queue is kept sorted in order
   of increasing timers.  Return a handle to the timer struct.  */

int
create_timer (int ms, timer_handler_func *proc,
	      gdb_client_data client_data)
{
  using namespace std::chrono;
  struct gdb_timer *timer_ptr, *timer_index, *prev_timer;

  steady_clock::time_point time_now = steady_clock::now ();

  timer_ptr = new gdb_timer ();
  timer_ptr->when = time_now + milliseconds (ms);
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
      if (timer_index->when > timer_ptr->when)
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
  delete timer_ptr;

  gdb_notifier.timeout_valid = 0;
}

/* Convert a std::chrono duration to a struct timeval.  */

template<typename Duration>
static struct timeval
duration_cast_timeval (const Duration &d)
{
  using namespace std::chrono;
  seconds sec = duration_cast<seconds> (d);
  microseconds msec = duration_cast<microseconds> (d - sec);

  struct timeval tv;
  tv.tv_sec = sec.count ();
  tv.tv_usec = msec.count ();
  return tv;
}

/* Update the timeout for the select() or poll().  Returns true if the
   timer has already expired, false otherwise.  */

static int
update_wait_timeout (void)
{
  if (timer_list.first_timer != NULL)
    {
      using namespace std::chrono;
      steady_clock::time_point time_now = steady_clock::now ();
      struct timeval timeout;

      if (timer_list.first_timer->when < time_now)
	{
	  /* It expired already.  */
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	}
      else
	{
	  steady_clock::duration d = timer_list.first_timer->when - time_now;
	  timeout = duration_cast_timeval (d);
	}

      /* Update the timeout for select/ poll.  */
      if (use_poll)
	{
#ifdef HAVE_POLL
	  gdb_notifier.poll_timeout = timeout.tv_sec * 1000;
#else
	  internal_error (__FILE__, __LINE__,
			  _("use_poll without HAVE_POLL"));
#endif /* HAVE_POLL */
	}
      else
	{
	  gdb_notifier.select_timeout.tv_sec = timeout.tv_sec;
	  gdb_notifier.select_timeout.tv_usec = timeout.tv_usec;
	}
      gdb_notifier.timeout_valid = 1;

      if (timer_list.first_timer->when < time_now)
	return 1;
    }
  else
    gdb_notifier.timeout_valid = 0;

  return 0;
}

/* Check whether a timer in the timers queue is ready.  If a timer is
   ready, call its handler and return.  Update the timeout for the
   select() or poll() as well.  Return 1 if an event was handled,
   otherwise returns 0.*/

static int
poll_timers (void)
{
  if (update_wait_timeout ())
    {
      struct gdb_timer *timer_ptr = timer_list.first_timer;
      timer_handler_func *proc = timer_ptr->proc;
      gdb_client_data client_data = timer_ptr->client_data;

      /* Get rid of the timer from the beginning of the list.  */
      timer_list.first_timer = timer_ptr->next;

      /* Delete the timer before calling the callback, not after, in
	 case the callback itself decides to try deleting the timer
	 too.  */
      xfree (timer_ptr);

      /* Call the procedure associated with that timer.  */
      (proc) (client_data);

      return 1;
    }

  return 0;
}
