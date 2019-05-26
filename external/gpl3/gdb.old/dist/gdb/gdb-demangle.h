/* Basic C++ demangling support for GDB.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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

#ifndef GDB_DEMANGLE_H
#define GDB_DEMANGLE_H

/* Nonzero means that encoded C++/ObjC names should be printed out in their
   C++/ObjC form rather than raw.  */
extern int demangle;

/* Nonzero means that encoded C++/ObjC names should be printed out in their
   C++/ObjC form even in assembler language displays.  If this is set, but
   DEMANGLE is zero, names are printed raw, i.e. DEMANGLE controls.  */
extern int asm_demangle;

/* Check if a character is one of the commonly used C++ marker characters.  */
extern int is_cplus_marker (int);

#endif /* GDB_DEMANGLE_H */
