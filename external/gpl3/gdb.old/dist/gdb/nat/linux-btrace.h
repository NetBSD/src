/* Linux-dependent part of branch trace support for GDB, and GDBserver.

   Copyright (C) 2013-2015 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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

#ifndef LINUX_BTRACE_H
#define LINUX_BTRACE_H

#include "btrace-common.h"
#include "vec.h"
#include <stdint.h>

#if HAVE_LINUX_PERF_EVENT_H
#  include <linux/perf_event.h>
#endif

struct target_ops;

/* Branch trace target information per thread.  */
struct btrace_target_info
{
#if HAVE_LINUX_PERF_EVENT_H
  /* The Linux perf_event configuration for collecting the branch trace.  */
  struct perf_event_attr attr;

  /* The ptid of this thread.  */
  ptid_t ptid;

  /* The mmap configuration mapping the branch trace perf_event buffer.

     file      .. the file descriptor
     buffer    .. the mmapped memory buffer
     size      .. the buffer's size in pages without the configuration page
     data_head .. the data head from the last read  */
  int file;
  void *buffer;
  size_t size;
  unsigned long data_head;
#endif /* HAVE_LINUX_PERF_EVENT_H */

  /* The size of a pointer in bits for this thread.
     The information is used to identify kernel addresses in order to skip
     records from/to kernel space.  */
  int ptr_bits;
};

/* See to_supports_btrace in target.h.  */
extern int linux_supports_btrace (struct target_ops *);

/* See to_enable_btrace in target.h.  */
extern struct btrace_target_info *linux_enable_btrace (ptid_t ptid);

/* See to_disable_btrace in target.h.  */
extern enum btrace_error linux_disable_btrace (struct btrace_target_info *ti);

/* See to_read_btrace in target.h.  */
extern enum btrace_error linux_read_btrace (VEC (btrace_block_s) **btrace,
					    struct btrace_target_info *btinfo,
					    enum btrace_read_type type);

#endif /* LINUX_BTRACE_H */
