/* Target description related code for GNU/Linux x86 (i386 and x86-64).

   Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

#ifndef GDB_ARCH_X86_LINUX_TDESC_FEATURES_H
#define GDB_ARCH_X86_LINUX_TDESC_FEATURES_H

#include "gdbsupport/x86-xstate.h"
#include "gdbsupport/gdb_assert.h"

/* Return a mask of X86_STATE_* feature flags.  The returned mask indicates
   the set of features which are checked for when creating the target
   description for each of amd64, x32, and i386.  */

extern uint64_t x86_linux_amd64_xstate_bv_feature_mask ();
extern uint64_t x86_linux_x32_xstate_bv_feature_mask ();
extern uint64_t x86_linux_i386_xstate_bv_feature_mask ();

#ifdef GDBSERVER

/* Convert an XSTATE_BV value into an integer.  XSTATE_BV has the same
   format than the state component bitmap and does include user and
   supervisor state components.  The integer will be passed from gdbserver
   to the in-process-agent where it will then be passed through
   x86_linux_tdesc_idx_to_xstate_bv to get back the original value.  */


extern int x86_linux_xstate_bv_to_tdesc_idx (uint64_t xstate_bv);

#endif /* GDBSERVER */

#ifdef IN_PROCESS_AGENT

/* Return the maximum possible number of target descriptions for each of
   amd64, x32, and i386.  These are used by the in-process-agent to
   generate every possible target description.  */

extern int x86_linux_amd64_tdesc_count ();
extern int x86_linux_x32_tdesc_count ();
extern int x86_linux_i386_tdesc_count ();

/* Convert an index number (as returned from
   x86_linux_xstate_bv_to_tdesc_idx) into an xstate_bv value which can
   then be used to create a target description.
   The return mask has the same format than the state component bitmap
   and does include user and supervisor state components.  */

extern uint64_t x86_linux_tdesc_idx_to_xstate_bv (int idx);

#endif /* IN_PROCESS_AGENT */

#endif /* GDB_ARCH_X86_LINUX_TDESC_FEATURES_H */
