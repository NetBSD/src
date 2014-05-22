/* Linux-dependent part of branch trace support for GDB, and GDBserver.

   Copyright (C) 2013 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#endif

#include "linux-btrace.h"
#include "common-utils.h"
#include "gdb_assert.h"
#include "regcache.h"
#include "gdbthread.h"

#if HAVE_LINUX_PERF_EVENT_H

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/* A branch trace record in perf_event.  */
struct perf_event_bts
{
  /* The linear address of the branch source.  */
  uint64_t from;

  /* The linear address of the branch destination.  */
  uint64_t to;
};

/* A perf_event branch trace sample.  */
struct perf_event_sample
{
  /* The perf_event sample header.  */
  struct perf_event_header header;

  /* The perf_event branch tracing payload.  */
  struct perf_event_bts bts;
};

/* Get the perf_event header.  */

static inline volatile struct perf_event_mmap_page *
perf_event_header (struct btrace_target_info* tinfo)
{
  return tinfo->buffer;
}

/* Get the size of the perf_event mmap buffer.  */

static inline size_t
perf_event_mmap_size (const struct btrace_target_info *tinfo)
{
  /* The branch trace buffer is preceded by a configuration page.  */
  return (tinfo->size + 1) * PAGE_SIZE;
}

/* Get the size of the perf_event buffer.  */

static inline size_t
perf_event_buffer_size (struct btrace_target_info* tinfo)
{
  return tinfo->size * PAGE_SIZE;
}

/* Get the start address of the perf_event buffer.  */

static inline const uint8_t *
perf_event_buffer_begin (struct btrace_target_info* tinfo)
{
  return ((const uint8_t *) tinfo->buffer) + PAGE_SIZE;
}

/* Get the end address of the perf_event buffer.  */

static inline const uint8_t *
perf_event_buffer_end (struct btrace_target_info* tinfo)
{
  return perf_event_buffer_begin (tinfo) + perf_event_buffer_size (tinfo);
}

/* Check whether an address is in the kernel.  */

static inline int
perf_event_is_kernel_addr (const struct btrace_target_info *tinfo,
			   uint64_t addr)
{
  uint64_t mask;

  /* If we don't know the size of a pointer, we can't check.  Let's assume it's
     not a kernel address in this case.  */
  if (tinfo->ptr_bits == 0)
    return 0;

  /* A bit mask for the most significant bit in an address.  */
  mask = (uint64_t) 1 << (tinfo->ptr_bits - 1);

  /* Check whether the most significant bit in the address is set.  */
  return (addr & mask) != 0;
}

/* Check whether a perf event record should be skipped.  */

static inline int
perf_event_skip_record (const struct btrace_target_info *tinfo,
			const struct perf_event_bts *bts)
{
  /* The hardware may report branches from kernel into user space.  Branches
     from user into kernel space will be suppressed.  We filter the former to
     provide a consistent branch trace excluding kernel.  */
  return perf_event_is_kernel_addr (tinfo, bts->from);
}

/* Perform a few consistency checks on a perf event sample record.  This is
   meant to catch cases when we get out of sync with the perf event stream.  */

static inline int
perf_event_sample_ok (const struct perf_event_sample *sample)
{
  if (sample->header.type != PERF_RECORD_SAMPLE)
    return 0;

  if (sample->header.size != sizeof (*sample))
    return 0;

  return 1;
}

/* Branch trace is collected in a circular buffer [begin; end) as pairs of from
   and to addresses (plus a header).

   Start points into that buffer at the next sample position.
   We read the collected samples backwards from start.

   While reading the samples, we convert the information into a list of blocks.
   For two adjacent samples s1 and s2, we form a block b such that b.begin =
   s1.to and b.end = s2.from.

   In case the buffer overflows during sampling, one sample may have its lower
   part at the end and its upper part at the beginning of the buffer.  */

static VEC (btrace_block_s) *
perf_event_read_bts (struct btrace_target_info* tinfo, const uint8_t *begin,
		     const uint8_t *end, const uint8_t *start)
{
  VEC (btrace_block_s) *btrace = NULL;
  struct perf_event_sample sample;
  size_t read = 0, size = (end - begin);
  struct btrace_block block = { 0, 0 };
  struct regcache *regcache;

  gdb_assert (begin <= start);
  gdb_assert (start <= end);

  /* The first block ends at the current pc.  */
#ifdef GDBSERVER
  regcache = get_thread_regcache (find_thread_ptid (tinfo->ptid), 1);
#else
  regcache = get_thread_regcache (tinfo->ptid);
#endif
  block.end = regcache_read_pc (regcache);

  /* The buffer may contain a partial record as its last entry (i.e. when the
     buffer size is not a multiple of the sample size).  */
  read = sizeof (sample) - 1;

  for (; read < size; read += sizeof (sample))
    {
      const struct perf_event_sample *psample;

      /* Find the next perf_event sample in a backwards traversal.  */
      start -= sizeof (sample);

      /* If we're still inside the buffer, we're done.  */
      if (begin <= start)
	psample = (const struct perf_event_sample *) start;
      else
	{
	  int missing;

	  /* We're to the left of the ring buffer, we will wrap around and
	     reappear at the very right of the ring buffer.  */

	  missing = (begin - start);
	  start = (end - missing);

	  /* If the entire sample is missing, we're done.  */
	  if (missing == sizeof (sample))
	    psample = (const struct perf_event_sample *) start;
	  else
	    {
	      uint8_t *stack;

	      /* The sample wrapped around.  The lower part is at the end and
		 the upper part is at the beginning of the buffer.  */
	      stack = (uint8_t *) &sample;

	      /* Copy the two parts so we have a contiguous sample.  */
	      memcpy (stack, start, missing);
	      memcpy (stack + missing, begin, sizeof (sample) - missing);

	      psample = &sample;
	    }
	}

      if (!perf_event_sample_ok (psample))
	{
	  warning (_("Branch trace may be incomplete."));
	  break;
	}

      if (perf_event_skip_record (tinfo, &psample->bts))
	continue;

      /* We found a valid sample, so we can complete the current block.  */
      block.begin = psample->bts.to;

      VEC_safe_push (btrace_block_s, btrace, &block);

      /* Start the next block.  */
      block.end = psample->bts.from;
    }

  return btrace;
}

/* Check whether the kernel supports branch tracing.  */

static int
kernel_supports_btrace (void)
{
  struct perf_event_attr attr;
  pid_t child, pid;
  int status, file;

  errno = 0;
  child = fork ();
  switch (child)
    {
    case -1:
      warning (_("test branch tracing: cannot fork: %s."), strerror (errno));
      return 0;

    case 0:
      status = ptrace (PTRACE_TRACEME, 0, NULL, NULL);
      if (status != 0)
	{
	  warning (_("test branch tracing: cannot PTRACE_TRACEME: %s."),
		   strerror (errno));
	  _exit (1);
	}

      status = raise (SIGTRAP);
      if (status != 0)
	{
	  warning (_("test branch tracing: cannot raise SIGTRAP: %s."),
		   strerror (errno));
	  _exit (1);
	}

      _exit (1);

    default:
      pid = waitpid (child, &status, 0);
      if (pid != child)
	{
	  warning (_("test branch tracing: bad pid %ld, error: %s."),
		   (long) pid, strerror (errno));
	  return 0;
	}

      if (!WIFSTOPPED (status))
	{
	  warning (_("test branch tracing: expected stop. status: %d."),
		   status);
	  return 0;
	}

      memset (&attr, 0, sizeof (attr));

      attr.type = PERF_TYPE_HARDWARE;
      attr.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
      attr.sample_period = 1;
      attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR;
      attr.exclude_kernel = 1;
      attr.exclude_hv = 1;
      attr.exclude_idle = 1;

      file = syscall (SYS_perf_event_open, &attr, child, -1, -1, 0);
      if (file >= 0)
	close (file);

      kill (child, SIGKILL);
      ptrace (PTRACE_KILL, child, NULL, NULL);

      pid = waitpid (child, &status, 0);
      if (pid != child)
	{
	  warning (_("test branch tracing: bad pid %ld, error: %s."),
		   (long) pid, strerror (errno));
	  if (!WIFSIGNALED (status))
	    warning (_("test branch tracing: expected killed. status: %d."),
		     status);
	}

      return (file >= 0);
    }
}

/* Check whether an Intel cpu supports branch tracing.  */

static int
intel_supports_btrace (void)
{
#if defined __i386__ || defined __x86_64__
    unsigned int cpuid, model, family;

    __asm__ __volatile__ ("movl   $1, %%eax;"
			  "cpuid;"
			  : "=a" (cpuid)
			  :: "%ebx", "%ecx", "%edx");

    family = (cpuid >> 8) & 0xf;
    model = (cpuid >> 4) & 0xf;

    switch (family)
      {
      case 0x6:
	model += (cpuid >> 12) & 0xf0;

	switch (model)
	  {
	  case 0x1a: /* Nehalem */
	  case 0x1f:
	  case 0x1e:
	  case 0x2e:
	  case 0x25: /* Westmere */
	  case 0x2c:
	  case 0x2f:
	  case 0x2a: /* Sandy Bridge */
	  case 0x2d:
	  case 0x3a: /* Ivy Bridge */

	    /* AAJ122: LBR, BTM, or BTS records may have incorrect branch
	       "from" information afer an EIST transition, T-states, C1E, or
	       Adaptive Thermal Throttling.  */
	    return 0;
	  }
      }

  return 1;

#else /* !defined __i386__ && !defined __x86_64__ */

  return 0;

#endif /* !defined __i386__ && !defined __x86_64__ */
}

/* Check whether the cpu supports branch tracing.  */

static int
cpu_supports_btrace (void)
{
#if defined __i386__ || defined __x86_64__
  char vendor[13];

  __asm__ __volatile__ ("xorl   %%ebx, %%ebx;"
			"xorl   %%ecx, %%ecx;"
			"xorl   %%edx, %%edx;"
			"movl   $0,    %%eax;"
			"cpuid;"
			"movl   %%ebx,  %0;"
			"movl   %%edx,  %1;"
			"movl   %%ecx,  %2;"
			: "=m" (vendor[0]),
			  "=m" (vendor[4]),
			  "=m" (vendor[8])
			:
			: "%eax", "%ebx", "%ecx", "%edx");
  vendor[12] = '\0';

  if (strcmp (vendor, "GenuineIntel") == 0)
    return intel_supports_btrace ();

  /* Don't know about others.  Let's assume they do.  */
  return 1;

#else /* !defined __i386__ && !defined __x86_64__ */

  return 0;

#endif /* !defined __i386__ && !defined __x86_64__ */
}

/* See linux-btrace.h.  */

int
linux_supports_btrace (void)
{
  static int cached;

  if (cached == 0)
    {
      if (!kernel_supports_btrace ())
	cached = -1;
      else if (!cpu_supports_btrace ())
	cached = -1;
      else
	cached = 1;
    }

  return cached > 0;
}

/* See linux-btrace.h.  */

struct btrace_target_info *
linux_enable_btrace (ptid_t ptid)
{
  struct btrace_target_info *tinfo;
  int pid;

  tinfo = xzalloc (sizeof (*tinfo));
  tinfo->ptid = ptid;

  tinfo->attr.size = sizeof (tinfo->attr);
  tinfo->attr.type = PERF_TYPE_HARDWARE;
  tinfo->attr.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
  tinfo->attr.sample_period = 1;

  /* We sample from and to address.  */
  tinfo->attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR;

  tinfo->attr.exclude_kernel = 1;
  tinfo->attr.exclude_hv = 1;
  tinfo->attr.exclude_idle = 1;

  tinfo->ptr_bits = 0;

  pid = ptid_get_lwp (ptid);
  if (pid == 0)
    pid = ptid_get_pid (ptid);

  errno = 0;
  tinfo->file = syscall (SYS_perf_event_open, &tinfo->attr, pid, -1, -1, 0);
  if (tinfo->file < 0)
    goto err;

  /* We hard-code the trace buffer size.
     At some later time, we should make this configurable.  */
  tinfo->size = 1;
  tinfo->buffer = mmap (NULL, perf_event_mmap_size (tinfo),
			PROT_READ, MAP_SHARED, tinfo->file, 0);
  if (tinfo->buffer == MAP_FAILED)
    goto err_file;

  return tinfo;

 err_file:
  close (tinfo->file);

 err:
  xfree (tinfo);
  return NULL;
}

/* See linux-btrace.h.  */

int
linux_disable_btrace (struct btrace_target_info *tinfo)
{
  int errcode;

  errno = 0;
  errcode = munmap (tinfo->buffer, perf_event_mmap_size (tinfo));
  if (errcode != 0)
    return errno;

  close (tinfo->file);
  xfree (tinfo);

  return 0;
}

/* Check whether the branch trace has changed.  */

static int
linux_btrace_has_changed (struct btrace_target_info *tinfo)
{
  volatile struct perf_event_mmap_page *header = perf_event_header (tinfo);

  return header->data_head != tinfo->data_head;
}

/* See linux-btrace.h.  */

VEC (btrace_block_s) *
linux_read_btrace (struct btrace_target_info *tinfo,
		   enum btrace_read_type type)
{
  VEC (btrace_block_s) *btrace = NULL;
  volatile struct perf_event_mmap_page *header;
  const uint8_t *begin, *end, *start;
  unsigned long data_head, retries = 5;
  size_t buffer_size;

  if (type == btrace_read_new && !linux_btrace_has_changed (tinfo))
    return NULL;

  header = perf_event_header (tinfo);
  buffer_size = perf_event_buffer_size (tinfo);

  /* We may need to retry reading the trace.  See below.  */
  while (retries--)
    {
      data_head = header->data_head;

      /* If there's new trace, let's read it.  */
      if (data_head != tinfo->data_head)
	{
	  /* Data_head keeps growing; the buffer itself is circular.  */
	  begin = perf_event_buffer_begin (tinfo);
	  start = begin + data_head % buffer_size;

	  if (data_head <= buffer_size)
	    end = start;
	  else
	    end = perf_event_buffer_end (tinfo);

	  btrace = perf_event_read_bts (tinfo, begin, end, start);
	}

      /* The stopping thread notifies its ptracer before it is scheduled out.
	 On multi-core systems, the debugger might therefore run while the
	 kernel might be writing the last branch trace records.

	 Let's check whether the data head moved while we read the trace.  */
      if (data_head == header->data_head)
	break;
    }

  tinfo->data_head = data_head;

  return btrace;
}

#else /* !HAVE_LINUX_PERF_EVENT_H */

/* See linux-btrace.h.  */

int
linux_supports_btrace (void)
{
  return 0;
}

/* See linux-btrace.h.  */

struct btrace_target_info *
linux_enable_btrace (ptid_t ptid)
{
  return NULL;
}

/* See linux-btrace.h.  */

int
linux_disable_btrace (struct btrace_target_info *tinfo)
{
  return ENOSYS;
}

/* See linux-btrace.h.  */

VEC (btrace_block_s) *
linux_read_btrace (struct btrace_target_info *tinfo,
		   enum btrace_read_type type)
{
  return NULL;
}

#endif /* !HAVE_LINUX_PERF_EVENT_H */
