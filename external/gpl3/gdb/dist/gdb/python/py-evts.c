/* Python interface to inferior events.

   Copyright (C) 2009, 2010, 2011 Free Software Foundation, Inc.

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

#include "py-events.h"

/* Initialize python events.  */

static int
add_new_registry (eventregistry_object **registryp, char *name)
{
  *registryp = create_eventregistry_object ();

  if (*registryp == NULL)
    goto fail;

  if (PyModule_AddObject (gdb_py_events.module,
                             name,
                             (PyObject *)(*registryp)) < 0)
    goto fail;

  return 0;

  fail:
   Py_XDECREF (*registryp);
   return -1;
}

void
gdbpy_initialize_py_events (void)
{
  gdb_py_events.module = Py_InitModule ("events", NULL);

  if (!gdb_py_events.module)
    goto fail;

  if (add_new_registry (&gdb_py_events.stop, "stop") < 0)
    goto fail;

  if (add_new_registry (&gdb_py_events.cont, "cont") < 0)
    goto fail;

  if (add_new_registry (&gdb_py_events.exited, "exited") < 0)
    goto fail;

  Py_INCREF (gdb_py_events.module);
  if (PyModule_AddObject (gdb_module,
                          "events",
                          (PyObject *) gdb_py_events.module) < 0)
    goto fail;

  return;

  fail:
   gdbpy_print_stack ();
}
