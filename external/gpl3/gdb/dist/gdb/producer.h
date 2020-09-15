/* Producer string parsers for GDB.

   Copyright (C) 2012-2020 Free Software Foundation, Inc.

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

#ifndef PRODUCER_H
#define PRODUCER_H

/* Check for GCC >= 4.x according to the symtab->producer string.  Return minor
   version (x) of 4.x in such case.  If it is not GCC or it is GCC older than
   4.x return -1.  If it is GCC 5.x or higher return INT_MAX.  */
extern int producer_is_gcc_ge_4 (const char *producer);

/* Returns nonzero if the given PRODUCER string is GCC and sets the MAJOR
   and MINOR versions when not NULL.  Returns zero if the given PRODUCER
   is NULL or it isn't GCC.  */
extern int producer_is_gcc (const char *producer, int *major, int *minor);

/* Returns true if the given PRODUCER string is Intel or false
   otherwise.  Sets the MAJOR and MINOR versions when not NULL.

   Internal and external ICC versions have to be taken into account.
   PRODUCER strings for internal releases are slightly different than
   for public ones.  Internal releases have a major release number and
   0 as minor release.  External releases have 4 fields, 3 of them are
   not 0 and only two are of interest, major and update.

   Examples are:

     Public release:
       "Intel(R) Fortran Intel(R) 64 Compiler XE for applications
        running on Intel(R) 64, Version 14.0.1.074 Build 20130716";
        "Intel(R) C++ Intel(R) 64 Compiler XE for applications
        running on Intel(R) 64, Version 14.0.1.074 Build 20130716";

    Internal releases:
      "Intel(R) C++ Intel(R) 64 Compiler for applications
       running on Intel(R) 64, Version 18.0 Beta ....".  */
extern bool producer_is_icc (const char *producer, int *major, int *minor);

/* Returns true if the given PRODUCER string is LLVM (clang/flang) or
   false otherwise.*/
extern bool producer_is_llvm (const char *producer);

#endif
