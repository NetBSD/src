/* Definitions used by the GDB event loop.
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

/* An event loop listens for events from multiple event sources.  When
   an event arrives, it is queued and processed by calling the
   appropriate event handler.  The event loop then continues to listen
   for more events.  An event loop completes when there are no event
   sources to listen on.  External event sources can be plugged into
   the loop.

   There are 4 main components:
   - a list of file descriptors to be monitored, GDB_NOTIFIER.
   - a list of asynchronous event sources to be monitored,
     ASYNC_EVENT_HANDLER_LIST.
   - a list of events that have occurred, EVENT_QUEUE.
   - a list of signal handling functions, SIGHANDLER_LIST.

   GDB_NOTIFIER keeps track of the file descriptor based event
   sources.  ASYNC_EVENT_HANDLER_LIST keeps track of asynchronous
   event sources that are signalled by some component of gdb, usually
   a target_ops instance.  Event sources for gdb are currently the UI
   and the target.  Gdb communicates with the command line user
   interface via the readline library and usually communicates with
   remote targets via a serial port.  Serial ports are represented in
   GDB as file descriptors and select/poll calls.  For native targets
   instead, the communication varies across operating system debug
   APIs, but usually consists of calls to ptrace and waits (via
   signals) or calls to poll/select (via file descriptors).  In the
   current gdb, the code handling events related to the target resides
   in wait_for_inferior for synchronous targets; or, for asynchronous
   capable targets, by having the target register either a target
   controlled file descriptor and/or an asynchronous event source in
   the event loop, with the fetch_inferior_event function as the event
   callback.  In both the synchronous and asynchronous cases, usually
   the target event is collected through the target_wait interface.
   The target is free to install other event sources in the event loop
   if it so requires.

   EVENT_QUEUE keeps track of the events that have happened during the
   last iteration of the event loop, and need to be processed.  An
   event is represented by a procedure to be invoked in order to
   process the event.  The queue is scanned head to tail.  If the
   event of interest is a change of state in a file descriptor, then a
   call to poll or select will be made to detect it.

   If the events generate signals, they are also queued by special
   functions that are invoked through traditional signal handlers.
   The actions to be taken is response to such events will be executed
   when the SIGHANDLER_LIST is scanned, the next time through the
   infinite loop.

   Corollary tasks are the creation and deletion of event sources.  */

typedef void *gdb_client_data;
struct async_signal_handler;
struct async_event_handler;
typedef void (handler_func) (int, gdb_client_data);
typedef void (sig_handler_func) (gdb_client_data);
typedef void (async_event_handler_func) (gdb_client_data);
typedef void (timer_handler_func) (gdb_client_data);

/* Exported functions from event-loop.c */

extern void initialize_event_loop (void);
extern void start_event_loop (void);
extern int gdb_do_one_event (void);
extern void delete_file_handler (int fd);
extern void add_file_handler (int fd, handler_func *proc, 
			      gdb_client_data client_data);
extern struct async_signal_handler *
  create_async_signal_handler (sig_handler_func *proc, 
			       gdb_client_data client_data);
extern void delete_async_signal_handler (struct async_signal_handler **);
extern int create_timer (int milliseconds, 
			 timer_handler_func *proc, 
			 gdb_client_data client_data);
extern void delete_timer (int id);

/* Call the handler from HANDLER immediately.  This function
   runs signal handlers when returning to the event loop would be too
   slow.  Do not call this directly; use gdb_call_async_signal_handler,
   below, with IMMEDIATE_P == 1.  */
void call_async_signal_handler (struct async_signal_handler *handler);

/* Call the handler from HANDLER the next time through the event loop.
   Do not call this directly; use gdb_call_async_signal_handler,
   below, with IMMEDIATE_P == 0.  */
void mark_async_signal_handler (struct async_signal_handler *handler);

/* Wrapper for the body of signal handlers.  Call this function from
   any SIGINT handler which needs to access GDB data structures or
   escape via longjmp.  If IMMEDIATE_P is set, this triggers either
   immediately (for POSIX platforms), or from gdb_select (for
   MinGW).  If IMMEDIATE_P is clear, the handler will run the next
   time we return to the event loop and any current select calls
   will be interrupted.  */

void gdb_call_async_signal_handler (struct async_signal_handler *handler,
				    int immediate_p);

/* Create and register an asynchronous event source in the event loop,
   and set PROC as its callback.  CLIENT_DATA is passed as argument to
   PROC upon its invocation.  Returns a pointer to an opaque structure
   used to mark as ready and to later delete this event source from
   the event loop.  */
extern struct async_event_handler *
  create_async_event_handler (async_event_handler_func *proc,
			      gdb_client_data client_data);

/* Remove the event source pointed by HANDLER_PTR created by
   CREATE_ASYNC_EVENT_HANDLER from the event loop, and release it.  */
extern void
  delete_async_event_handler (struct async_event_handler **handler_ptr);

/* Call the handler from HANDLER the next time through the event
   loop.  */
extern void mark_async_event_handler (struct async_event_handler *handler);
