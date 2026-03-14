/* Python interface to ui_file_style::color objects.

   Copyright (C) 2009-2025 Free Software Foundation, Inc.

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

#ifndef GDB_PYTHON_PY_COLOR_H
#define GDB_PYTHON_PY_COLOR_H

#include "python-internal.h"
#include "ui-style.h"

/* Create a new gdb.Color object from COLOR.  */
extern gdbpy_ref<> create_color_object (const ui_file_style::color &color);

/* Check if OBJ is instance of a gdb.Color type.  */
extern bool gdbpy_is_color (PyObject *obj);

/* Extracts value from OBJ object of gdb.Color type.  */
extern const ui_file_style::color &gdbpy_get_color (PyObject *obj);

#endif /* GDB_PYTHON_PY_COLOR_H */
