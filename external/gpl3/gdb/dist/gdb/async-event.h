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

#ifndef ASYNC_EVENT_H
#define ASYNC_EVENT_H

#include "gdbsupport/event-loop.h"

struct async_signal_handler;
struct async_event_handler;
typedef void (sig_handler_func) (gdb_client_data);
typedef void (async_event_handler_func) (gdb_client_data);

extern struct async_signal_handler *
  create_async_signal_handler (sig_handler_func *proc, 
			       gdb_client_data client_data);
extern void delete_async_signal_handler (struct async_signal_handler **);

/* Call the handler from HANDLER the next time through the event
   loop.  */
extern void mark_async_signal_handler (struct async_signal_handler *handler);

/* Returns true if HANDLER is marked ready.  */

extern int
  async_signal_handler_is_marked (struct async_signal_handler *handler);

/* Mark HANDLER as NOT ready.  */

extern void clear_async_signal_handler (struct async_signal_handler *handler);

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

/* Mark the handler (ASYNC_HANDLER_PTR) as NOT ready.  */

extern void clear_async_event_handler (struct async_event_handler *handler);

extern void initialize_async_signal_handlers (void);

#endif /* ASYNC_EVENT_H */
