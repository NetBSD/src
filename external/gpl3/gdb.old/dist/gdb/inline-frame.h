/* Definitions for inline frame support.

   Copyright (C) 2008-2016 Free Software Foundation, Inc.

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

#if !defined (INLINE_FRAME_H)
#define INLINE_FRAME_H 1

struct frame_info;
struct frame_unwind;

/* The inline frame unwinder.  */

extern const struct frame_unwind inline_frame_unwind;

/* Skip all inlined functions whose call sites are at the current PC.
   Frames for the hidden functions will not appear in the backtrace until the
   user steps into them.  */

void skip_inline_frames (ptid_t ptid);

/* Forget about any hidden inlined functions in PTID, which is new or
   about to be resumed.  If PTID is minus_one_ptid, forget about all
   hidden inlined functions.  */

void clear_inline_frame_state (ptid_t ptid);

/* Step into an inlined function by unhiding it.  */

void step_into_inline_frame (ptid_t ptid);

/* Return the number of hidden functions inlined into the current
   frame.  */

int inline_skipped_frames (ptid_t ptid);

/* If one or more inlined functions are hidden, return the symbol for
   the function inlined into the current frame.  */

struct symbol *inline_skipped_symbol (ptid_t ptid);

/* Return the number of functions inlined into THIS_FRAME.  Some of
   the callees may not have associated frames (see
   skip_inline_frames).  */

int frame_inlined_callees (struct frame_info *this_frame);

#endif /* !defined (INLINE_FRAME_H) */
