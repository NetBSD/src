/* Python interface to instruction objects.

   Copyright 2017 Free Software Foundation, Inc.

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

#ifndef GDB_PY_INSTRUCTION_H
#define GDB_PY_INSTRUCTION_H

#include "python-internal.h"

/* Python type object for the abstract gdb.Instruction class.  This class
   contains getters for four elements: "pc" (int), "data" (buffer), "decode"
   (str) and "size" (int) that must be overriden by sub classes.  */
extern PyTypeObject py_insn_type;

#endif /* GDB_PY_INSTRUCTION_H */
