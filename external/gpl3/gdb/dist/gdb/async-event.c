/* Async events for the GDB event loop.
   Copyright (C) 1999-2020 Free Software Foundation, Inc.

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
#include "async-event.h"

#include "ser-event.h"
#include "top.h"

/* PROC is a function to be invoked when the READY flag is set.  This
   happens when there has been a signal and the corresponding signal
   handler has 'triggered' this async_signal_handler for execution.
   The actual work to be done in response to a signal will be carried
   out by PROC at a later time, within process_event.  This provides a
   deferred execution of signal handlers.

   Async_init_signals takes care of setting up such an
   async_signal_handler for each interesting signal.  */

struct async_signal_handler
{
  /* If ready, call this handler  from the main event loop, using
     invoke_async_handler.  */
  int ready;

  /* Pointer to next handler.  */
  struct async_signal_handler *next_handler;

  /* Function to call to do the work.  */
  sig_handler_func *proc;

  /* Argument to PROC.  */
  gdb_client_data client_data;
};

/* PROC is a function to be invoked when the READY flag is set.  This
   happens when the event has been marked with
   MARK_ASYNC_EVENT_HANDLER.  The actual work to be done in response
   to an event will be carried out by PROC at a later time, within
   process_event.  This provides a deferred execution of event
   handlers.  */
struct async_event_handler
{
  /* If ready, call this handler from the main event loop, using
     invoke_event_handler.  */
  int ready;

  /* Pointer to next handler.  */
  struct async_event_handler *next_handler;

  /* Function to call to do the work.  */
  async_event_handler_func *proc;

  /* Argument to PROC.  */
  gdb_client_data client_data;
};

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

int
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

int
check_async_event_handlers ()
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
