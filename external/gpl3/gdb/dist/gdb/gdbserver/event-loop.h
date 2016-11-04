/* Event loop machinery for the remote server for GDB.
   Copyright (C) 1993-2016 Free Software Foundation, Inc.

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

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

typedef void *gdb_client_data;
typedef int (handler_func) (int, gdb_client_data);
typedef int (callback_handler_func) (gdb_client_data);

extern void delete_file_handler (gdb_fildes_t fd);
extern void add_file_handler (gdb_fildes_t fd, handler_func *proc,
			      gdb_client_data client_data);
extern int append_callback_event (callback_handler_func *proc,
				   gdb_client_data client_data);
extern void delete_callback_event (int id);

extern void start_event_loop (void);
extern void initialize_event_loop (void);

#endif /* EVENT_LOOP_H */
