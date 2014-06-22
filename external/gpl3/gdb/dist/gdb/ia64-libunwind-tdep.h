/* Frame unwinder for ia64 frames with libunwind frame information.

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

   Contributed by Jeff Johnston.

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

#ifndef IA64_LIBUNWIND_TDEP_H
#define IA64_LIBUNWIND_TDEP_H 1

struct frame_info;
struct frame_id;
struct regcache;
struct gdbarch;
struct frame_unwind;

/* IA-64 is the only target that currently uses libunwind.  If some
   other target wants to use it, we will need to do some abstracting
   in order to make it possible to have more than one
   ia64-libunwind-tdep instance.  Including "libunwind.h" is wrong as
   that ends up including the libunwind-$(arch).h for the host gdb is
   running on.  */
#include "libunwind-ia64.h"

struct libunwind_descr
{
  int (*gdb2uw) (int);
  int (*uw2gdb) (int);
  int (*is_fpreg) (int);
  void *accessors;
  void *special_accessors;
};

int libunwind_frame_sniffer (const struct frame_unwind *self,
                             struct frame_info *this_frame,
                             void **this_cache);
                          
int libunwind_sigtramp_frame_sniffer (const struct frame_unwind *self,
                                      struct frame_info *this_frame,
                                      void **this_cache);

void libunwind_frame_set_descr (struct gdbarch *arch,
				struct libunwind_descr *descr);

void libunwind_frame_this_id (struct frame_info *this_frame, void **this_cache,
			      struct frame_id *this_id);
struct value *libunwind_frame_prev_register (struct frame_info *this_frame,
                                             void **this_cache, int regnum);
void libunwind_frame_dealloc_cache (struct frame_info *self, void *cache);

int libunwind_is_initialized (void);

int libunwind_search_unwind_table (void *as, long ip, void *di,
				   void *pi, int need_unwind_info, void *args);

unw_word_t libunwind_find_dyn_list (unw_addr_space_t, unw_dyn_info_t *,
				    void *);

int libunwind_get_reg_special (struct gdbarch *gdbarch,
			       struct regcache *regcache,
			       int regnum, void *buf);

#endif /* IA64_LIBUNWIND_TDEP_H */
