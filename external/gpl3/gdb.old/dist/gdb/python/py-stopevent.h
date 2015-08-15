/* Python interface to inferior events.

   Copyright (C) 2009-2014 Free Software Foundation, Inc.

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

#ifndef GDB_PY_STOPEVENT_H
#define GDB_PY_STOPEVENT_H

#include "py-event.h"

extern PyObject *create_stop_event_object (PyTypeObject *py_type);
extern void stop_evpy_dealloc (PyObject *self);

extern int emit_stop_event (struct bpstats *bs,
                            enum gdb_signal stop_signal);

extern PyObject *create_breakpoint_event_object (PyObject *breakpoint_list,
                                                 PyObject *first_bp);

extern PyObject *create_signal_event_object (enum gdb_signal stop_signal);

#endif /* GDB_PY_STOPEVENT_H */
