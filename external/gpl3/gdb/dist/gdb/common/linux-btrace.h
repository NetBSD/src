/* Linux-dependent part of branch trace support for GDB, and GDBserver.

   Copyright (C) 2013-2014 Free Software Foundation, Inc.

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
#include "config.h"
#include "vec.h"
#include "ptid.h"
#include <stddef.h>
#include <stdint.h>

#if HAVE_LINUX_PERF_EVENT_H
#  include <linux/perf_event.h>
#endif

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

/* Check whether branch tracing is supported.  */
extern int linux_supports_btrace (void);

/* Enable branch tracing for @ptid.  */
extern struct btrace_target_info *linux_enable_btrace (ptid_t ptid);

/* Disable branch tracing and deallocate @tinfo.  */
extern int linux_disable_btrace (struct btrace_target_info *tinfo);

/* Read branch trace data.  */
extern VEC (btrace_block_s) *linux_read_btrace (struct btrace_target_info *,
						enum btrace_read_type);

#endif /* LINUX_BTRACE_H */
