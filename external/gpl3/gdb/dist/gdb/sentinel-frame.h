/* Code dealing with register stack frames, for GDB, the GNU debugger.

   Copyright (C) 2003-2025 Free Software Foundation, Inc.

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

#ifndef GDB_SENTINEL_FRAME_H
#define GDB_SENTINEL_FRAME_H

struct frame_unwind;
struct regcache;

/* Implement the sentinel frame.  The sentinel frame terminates the
   inner most end of the frame chain.  If unwound, it returns the
   information need to construct an inner-most frame.  */

/* Pump prime the sentinel frame's cache.  Since this needs the
   REGCACHE provide that here.  */

extern void *sentinel_frame_cache (struct regcache *regcache);

/* At present there is only one type of sentinel frame.  */

extern const struct frame_unwind_legacy sentinel_frame_unwind;

#endif /* GDB_SENTINEL_FRAME_H */
