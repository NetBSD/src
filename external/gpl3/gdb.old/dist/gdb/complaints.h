/* Definitions for complaint handling during symbol reading in GDB.

   Copyright (C) 1990-2020 Free Software Foundation, Inc.

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


#if !defined (COMPLAINTS_H)
#define COMPLAINTS_H

/* Helper for complaint.  */
extern void complaint_internal (const char *fmt, ...)
  ATTRIBUTE_PRINTF (1, 2);

/* This controls whether complaints are emitted.  */

extern int stop_whining;

/* Register a complaint.  This is a macro around complaint_internal to
   avoid computing complaint's arguments when complaints are disabled.
   Running FMT via gettext [i.e., _(FMT)] can be quite expensive, for
   example.  */
#define complaint(FMT, ...)					\
  do								\
    {								\
      if (stop_whining > 0)					\
	complaint_internal (FMT, ##__VA_ARGS__);		\
    }								\
  while (0)

/* Clear out / initialize all complaint counters that have ever been
   incremented.  */

extern void clear_complaints ();


#endif /* !defined (COMPLAINTS_H) */
