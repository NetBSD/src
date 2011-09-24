/* Target signal translation functions for GDB.
   Copyright (C) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002, 2003, 2006, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

#ifndef COMMON_GDB_SIGNALS_H
#define COMMON_GDB_SIGNALS_H

#include "gdb/signals.h"

/* Predicate to target_signal_to_host(). Return non-zero if the enum
   targ_signal SIGNO has an equivalent ``host'' representation.  */
/* FIXME: cagney/1999-11-22: The name below was chosen in preference
   to the shorter target_signal_p() because it is far less ambigious.
   In this context ``target_signal'' refers to GDB's internal
   representation of the target's set of signals while ``host signal''
   refers to the target operating system's signal.  Confused?  */
extern int target_signal_to_host_p (enum target_signal signo);

/* Convert between host signal numbers and enum target_signal's.
   target_signal_to_host() returns 0 and prints a warning() on GDB's
   console if SIGNO has no equivalent host representation.  */
/* FIXME: cagney/1999-11-22: Here ``host'' is used incorrectly, it is
   refering to the target operating system's signal numbering.
   Similarly, ``enum target_signal'' is named incorrectly, ``enum
   gdb_signal'' would probably be better as it is refering to GDB's
   internal representation of a target operating system's signal.  */
extern enum target_signal target_signal_from_host (int);
extern int target_signal_to_host (enum target_signal);

/* Return the string for a signal.  */
extern const char *target_signal_to_string (enum target_signal);

/* Return the name (SIGHUP, etc.) for a signal.  */
extern const char *target_signal_to_name (enum target_signal);

/* Given a name (SIGHUP, etc.), return its signal.  */
enum target_signal target_signal_from_name (const char *);

#endif /* COMMON_GDB_SIGNALS_H */
