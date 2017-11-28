/* Linux-dependent part of branch trace support for GDB, and GDBserver.

   Copyright (C) 2013-2016 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "linux-btrace.h"
#include "common-regcache.h"
#include "gdb_wait.h"
#include "x86-cpuid.h"
#include "filestuff.h"

#include <inttypes.h>

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#if HAVE_LINUX_PERF_EVENT_H && defined(SYS_perf_event_open)
#include <unistd.h>
#include <sys/mman.h>
#include <sys/user.h>
#include "nat/gdb_ptrace.h"
#include <sys/types.h>
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

/* Identify the cpu we're running on.  */
static struct btrace_cpu
btrace_this_cpu (void)
{
  struct btrace_cpu cpu;
  unsigned int eax, ebx, ecx, edx;
  int ok;

  memset (&cpu, 0, sizeof (cpu));

  ok = x86_cpuid (0, &eax, &ebx, &ecx, &edx);
  if (ok != 0)
    {
      if (ebx == signature_INTEL_ebx && ecx == signature_INTEL_ecx
	  && edx == signature_INTEL_edx)
	{
	  unsigned int cpuid, ignore;

	  ok = x86_cpuid (1, &cpuid, &ignore, &ignore, &ignore);
	  if (ok != 0)
	    {
	      cpu.vendor = CV_INTEL;

	      cpu.family = (cpuid >> 8) & 0xf;
	      cpu.model = (cpuid >> 4) & 0xf;

	      if (cpu.family == 0x6)
		cpu.model += (cpuid >> 12) & 0xf0;
	    }
	}
    }

  return cpu;
}

/* Return non-zero if there is new data in PEVENT; zero otherwise.  */

static int
perf_event_new_data (const struct perf_event_buffer *pev)
{
  return *pev->data_head != pev->last_head;
}

/* Try to determine the size of a pointer in bits for the OS.

   This is the same as the size of a pointer for the inferior process
   except when a 32-bit inferior is running on a 64-bit OS.  */

/* Copy the last SIZE bytes from PEV ending at DATA_HEAD and return a pointer
   to the memory holding the copy.
   The caller is responsible for freeing the memory.  */

static gdb_byte *
perf_event_read (const struct perf_event_buffer *pev, __u64 data_head,
		 size_t size)
{
  const gdb_byte *begin, *end, *start, *stop;
  gdb_byte *buffer;
  size_t buffer_size;
  __u64 data_tail;

  if (size == 0)
    return NULL;

  gdb_assert (size <= data_head);
  data_tail = data_head - size;

  buffer_size = pev->size;
  begin = pev->mem;
  start = begin + data_tail % buffer_size;
  stop = begin + data_head % buffer_size;

  buffer = (gdb_byte *) xmalloc (size);

  if (start < stop)
    memcpy (buffer, start, stop - start);
  else
    {
      end = begin + buffer_size;

      memcpy (buffer, start, end - start);
      memcpy (buffer + (end - start), begin, stop - begin);
    }

  return buffer;
}

/* Copy the perf event buffer data from PEV.
   Store a pointer to the copy into DATA and its size in SIZE.  */

static void
perf_event_read_all (struct perf_event_buffer *pev, gdb_byte **data,
		     size_t *psize)
{
  size_t size;
  __u64 data_head;

  data_head = *pev->data_head;

  size = pev->size;
  if (data_head < size)
    size = (size_t) data_head;

  *data = perf_event_read (pev, data_head, size);
  *psize = size;

  pev->last_head = data_head;
}

/* Determine the event type.
   Returns zero on success and fills in TYPE; returns -1 otherwise.  */

static int
perf_event_pt_event_type (int *type)
{
  FILE *file;
  int found;

  file = fopen ("/sys/bus/event_source/devices/intel_pt/type", "r");
  if (file == NULL)
    return -1;

  found = fscanf (file, "%d", type);

  fclose (file);

  if (found == 1)
    return 0;
  return -1;
}

/* Try to determine the start address of the Linux kernel.  */

static uint64_t
linux_determine_kernel_start (void)
{
  static uint64_t kernel_start;
  static int cached;
  FILE *file;

  if (cached != 0)
    return kernel_start;

  cached = 1;

  file = gdb_fopen_cloexec ("/proc/kallsyms", "r");
  if (file == NULL)
    return kernel_start;

  while (!feof (file))
    {
      char buffer[1024], symbol[8], *line;
      uint64_t addr;
      int match;

      line = fgets (buffer, sizeof (buffer), file);
      if (line == NULL)
	break;

      match = sscanf (line, "%" SCNx64 " %*[tT] %7s", &addr, symbol);
      if (match != 2)
	continue;

      if (strcmp (symbol, "_text") == 0)
	{
	  kernel_start = addr;
	  break;
	}
    }

  fclose (file);

  return kernel_start;
}

/* Check whether an address is in the kernel.  */

static inline int
perf_event_is_kernel_addr (uint64_t addr)
{
  uint64_t kernel_start;

  kernel_start = linux_determine_kernel_start ();
  if (kernel_start != 0ull)
    return (addr >= kernel_start);

  /* If we don't know the kernel's start address, let's check the most
     significant bit.  This will work at least for 64-bit kernels.  */
  return ((addr & (1ull << 63)) != 0);
}

/* Check whether a perf event record should be skipped.  */

static inline int
perf_event_skip_bts_record (const struct perf_event_bts *bts)
{
  /* The hardware may report branches from kernel into user space.  Branches
     from user into kernel space will be suppressed.  We filter the former to
     provide a consistent branch trace excluding kernel.  */
  return perf_event_is_kernel_addr (bts->from);
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
		     const uint8_t *end, const uint8_t *start, size_t size)
{
  VEC (btrace_block_s) *btrace = NULL;
  struct perf_event_sample sample;
  size_t read = 0;
  struct btrace_block block = { 0, 0 };
  struct regcache *regcache;

  gdb_assert (begin <= start);
  gdb_assert (start <= end);

  /* The first block ends at the current pc.  */
  regcache = get_thread_regcache_for_ptid (tinfo->ptid);
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

      if (perf_event_skip_bts_record (&psample->bts))
	continue;

      /* We found a valid sample, so we can complete the current block.  */
      block.begin = psample->bts.to;

      VEC_safe_push (btrace_block_s, btrace, &block);

      /* Start the next block.  */
      block.end = psample->bts.from;
    }

  /* Push the last block (i.e. the first one of inferior execution), as well.
     We don't know where it ends, but we know where it starts.  If we're
     reading delta trace, we can fill in the start address later on.
     Otherwise we will prune it.  */
  block.begin = 0;
  VEC_safe_push (btrace_block_s, btrace, &block);

  return btrace;
}

/* Check whether the kernel supports BTS.  */

static int
kernel_supports_bts (void)
{
  struct perf_event_attr attr;
  pid_t child, pid;
  int status, file;

  errno = 0;
  child = fork ();
  switch (child)
    {
    case -1:
      warning (_("test bts: cannot fork: %s."), safe_strerror (errno));
      return 0;

    case 0:
      status = ptrace (PTRACE_TRACEME, 0, NULL, NULL);
      if (status != 0)
	{
	  warning (_("test bts: cannot PTRACE_TRACEME: %s."),
		   safe_strerror (errno));
	  _exit (1);
	}

      status = raise (SIGTRAP);
      if (status != 0)
	{
	  warning (_("test bts: cannot raise SIGTRAP: %s."),
		   safe_strerror (errno));
	  _exit (1);
	}

      _exit (1);

    default:
      pid = waitpid (child, &status, 0);
      if (pid != child)
	{
	  warning (_("test bts: bad pid %ld, error: %s."),
		   (long) pid, safe_strerror (errno));
	  return 0;
	}

      if (!WIFSTOPPED (status))
	{
	  warning (_("test bts: expected stop. status: %d."),
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
	  warning (_("test bts: bad pid %ld, error: %s."),
		   (long) pid, safe_strerror (errno));
	  if (!WIFSIGNALED (status))
	    warning (_("test bts: expected killed. status: %d."),
		     status);
	}

      return (file >= 0);
    }
}

/* Check whether the kernel supports Intel Processor Trace.  */

static int
kernel_supports_pt (void)
{
  struct perf_event_attr attr;
  pid_t child, pid;
  int status, file, type;

  errno = 0;
  child = fork ();
  switch (child)
    {
    case -1:
      warning (_("test pt: cannot fork: %s."), safe_strerror (errno));
      return 0;

    case 0:
      status = ptrace (PTRACE_TRACEME, 0, NULL, NULL);
      if (status != 0)
	{
	  warning (_("test pt: cannot PTRACE_TRACEME: %s."),
		   safe_strerror (errno));
	  _exit (1);
	}

      status = raise (SIGTRAP);
      if (status != 0)
	{
	  warning (_("test pt: cannot raise SIGTRAP: %s."),
		   safe_strerror (errno));
	  _exit (1);
	}

      _exit (1);

    default:
      pid = waitpid (child, &status, 0);
      if (pid != child)
	{
	  warning (_("test pt: bad pid %ld, error: %s."),
		   (long) pid, safe_strerror (errno));
	  return 0;
	}

      if (!WIFSTOPPED (status))
	{
	  warning (_("test pt: expected stop. status: %d."),
		   status);
	  return 0;
	}

      status = perf_event_pt_event_type (&type);
      if (status != 0)
	file = -1;
      else
	{
	  memset (&attr, 0, sizeof (attr));

	  attr.size = sizeof (attr);
	  attr.type = type;
	  attr.exclude_kernel = 1;
	  attr.exclude_hv = 1;
	  attr.exclude_idle = 1;

	  file = syscall (SYS_perf_event_open, &attr, child, -1, -1, 0);
	  if (file >= 0)
	    close (file);
	}

      kill (child, SIGKILL);
      ptrace (PTRACE_KILL, child, NULL, NULL);

      pid = waitpid (child, &status, 0);
      if (pid != child)
	{
	  warning (_("test pt: bad pid %ld, error: %s."),
		   (long) pid, safe_strerror (errno));
	  if (!WIFSIGNALED (status))
	    warning (_("test pt: expected killed. status: %d."),
		     status);
	}

      return (file >= 0);
    }
}

/* Check whether an Intel cpu supports BTS.  */

static int
intel_supports_bts (const struct btrace_cpu *cpu)
{
  switch (cpu->family)
    {
    case 0x6:
      switch (cpu->model)
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
}

/* Check whether the cpu supports BTS.  */

static int
cpu_supports_bts (void)
{
  struct btrace_cpu cpu;

  cpu = btrace_this_cpu ();
  switch (cpu.vendor)
    {
    default:
      /* Don't know about others.  Let's assume they do.  */
      return 1;

    case CV_INTEL:
      return intel_supports_bts (&cpu);
    }
}

/* Check whether the linux target supports BTS.  */

static int
linux_supports_bts (void)
{
  static int cached;

  if (cached == 0)
    {
      if (!kernel_supports_bts ())
	cached = -1;
      else if (!cpu_supports_bts ())
	cached = -1;
      else
	cached = 1;
    }

  return cached > 0;
}

/* Check whether the linux target supports Intel Processor Trace.  */

static int
linux_supports_pt (void)
{
  static int cached;

  if (cached == 0)
    {
      if (!kernel_supports_pt ())
	cached = -1;
      else
	cached = 1;
    }

  return cached > 0;
}

/* See linux-btrace.h.  */

int
linux_supports_btrace (struct target_ops *ops, enum btrace_format format)
{
  switch (format)
    {
    case BTRACE_FORMAT_NONE:
      return 0;

    case BTRACE_FORMAT_BTS:
      return linux_supports_bts ();

    case BTRACE_FORMAT_PT:
      return linux_supports_pt ();
    }

  internal_error (__FILE__, __LINE__, _("Unknown branch trace format"));
}

/* Enable branch tracing in BTS format.  */

static struct btrace_target_info *
linux_enable_bts (ptid_t ptid, const struct btrace_config_bts *conf)
{
  struct perf_event_mmap_page *header;
  struct btrace_target_info *tinfo;
  struct btrace_tinfo_bts *bts;
  size_t size, pages;
  __u64 data_offset;
  int pid, pg;

  tinfo = XCNEW (struct btrace_target_info);
  tinfo->ptid = ptid;

  tinfo->conf.format = BTRACE_FORMAT_BTS;
  bts = &tinfo->variant.bts;

  bts->attr.size = sizeof (bts->attr);
  bts->attr.type = PERF_TYPE_HARDWARE;
  bts->attr.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
  bts->attr.sample_period = 1;

  /* We sample from and to address.  */
  bts->attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR;

  bts->attr.exclude_kernel = 1;
  bts->attr.exclude_hv = 1;
  bts->attr.exclude_idle = 1;

  pid = ptid_get_lwp (ptid);
  if (pid == 0)
    pid = ptid_get_pid (ptid);

  errno = 0;
  bts->file = syscall (SYS_perf_event_open, &bts->attr, pid, -1, -1, 0);
  if (bts->file < 0)
    goto err_out;

  /* Convert the requested size in bytes to pages (rounding up).  */
  pages = ((size_t) conf->size / PAGE_SIZE
	   + ((conf->size % PAGE_SIZE) == 0 ? 0 : 1));
  /* We need at least one page.  */
  if (pages == 0)
    pages = 1;

  /* The buffer size can be requested in powers of two pages.  Adjust PAGES
     to the next power of two.  */
  for (pg = 0; pages != ((size_t) 1 << pg); ++pg)
    if ((pages & ((size_t) 1 << pg)) != 0)
      pages += ((size_t) 1 << pg);

  /* We try to allocate the requested size.
     If that fails, try to get as much as we can.  */
  for (; pages > 0; pages >>= 1)
    {
      size_t length;
      __u64 data_size;

      data_size = (__u64) pages * PAGE_SIZE;

      /* Don't ask for more than we can represent in the configuration.  */
      if ((__u64) UINT_MAX < data_size)
	continue;

      size = (size_t) data_size;
      length = size + PAGE_SIZE;

      /* Check for overflows.  */
      if ((__u64) length != data_size + PAGE_SIZE)
	continue;

      /* The number of pages we request needs to be a power of two.  */
      header = ((struct perf_event_mmap_page *)
		mmap (NULL, length, PROT_READ, MAP_SHARED, bts->file, 0));
      if (header != MAP_FAILED)
	break;
    }

  if (pages == 0)
    goto err_file;

  data_offset = PAGE_SIZE;

#if defined (PERF_ATTR_SIZE_VER5)
  if (offsetof (struct perf_event_mmap_page, data_size) <= header->size)
    {
      __u64 data_size;

      data_offset = header->data_offset;
      data_size = header->data_size;

      size = (unsigned int) data_size;

      /* Check for overflows.  */
      if ((__u64) size != data_size)
	{
	  munmap ((void *) header, size + PAGE_SIZE);
	  goto err_file;
	}
    }
#endif /* defined (PERF_ATTR_SIZE_VER5) */

  bts->header = header;
  bts->bts.mem = ((const uint8_t *) header) + data_offset;
  bts->bts.size = size;
  bts->bts.data_head = &header->data_head;
  bts->bts.last_head = 0ull;

  tinfo->conf.bts.size = (unsigned int) size;
  return tinfo;

 err_file:
  /* We were not able to allocate any buffer.  */
  close (bts->file);

 err_out:
  xfree (tinfo);
  return NULL;
}

#if defined (PERF_ATTR_SIZE_VER5)

/* Enable branch tracing in Intel Processor Trace format.  */

static struct btrace_target_info *
linux_enable_pt (ptid_t ptid, const struct btrace_config_pt *conf)
{
  struct perf_event_mmap_page *header;
  struct btrace_target_info *tinfo;
  struct btrace_tinfo_pt *pt;
  size_t pages, size;
  int pid, pg, errcode, type;

  if (conf->size == 0)
    return NULL;

  errcode = perf_event_pt_event_type (&type);
  if (errcode != 0)
    return NULL;

  pid = ptid_get_lwp (ptid);
  if (pid == 0)
    pid = ptid_get_pid (ptid);

  tinfo = XCNEW (struct btrace_target_info);
  tinfo->ptid = ptid;

  tinfo->conf.format = BTRACE_FORMAT_PT;
  pt = &tinfo->variant.pt;

  pt->attr.size = sizeof (pt->attr);
  pt->attr.type = type;

  pt->attr.exclude_kernel = 1;
  pt->attr.exclude_hv = 1;
  pt->attr.exclude_idle = 1;

  errno = 0;
  pt->file = syscall (SYS_perf_event_open, &pt->attr, pid, -1, -1, 0);
  if (pt->file < 0)
    goto err;

  /* Allocate the configuration page. */
  header = ((struct perf_event_mmap_page *)
	    mmap (NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		  pt->file, 0));
  if (header == MAP_FAILED)
    goto err_file;

  header->aux_offset = header->data_offset + header->data_size;

  /* Convert the requested size in bytes to pages (rounding up).  */
  pages = ((size_t) conf->size / PAGE_SIZE
	   + ((conf->size % PAGE_SIZE) == 0 ? 0 : 1));
  /* We need at least one page.  */
  if (pages == 0)
    pages = 1;

  /* The buffer size can be requested in powers of two pages.  Adjust PAGES
     to the next power of two.  */
  for (pg = 0; pages != ((size_t) 1 << pg); ++pg)
    if ((pages & ((size_t) 1 << pg)) != 0)
      pages += ((size_t) 1 << pg);

  /* We try to allocate the requested size.
     If that fails, try to get as much as we can.  */
  for (; pages > 0; pages >>= 1)
    {
      size_t length;
      __u64 data_size;

      data_size = (__u64) pages * PAGE_SIZE;

      /* Don't ask for more than we can represent in the configuration.  */
      if ((__u64) UINT_MAX < data_size)
	continue;

      size = (size_t) data_size;

      /* Check for overflows.  */
      if ((__u64) size != data_size)
	continue;

      header->aux_size = data_size;
      length = size;

      pt->pt.mem = ((const uint8_t *)
		    mmap (NULL, length, PROT_READ, MAP_SHARED, pt->file,
			  header->aux_offset));
      if (pt->pt.mem != MAP_FAILED)
	break;
    }

  if (pages == 0)
    goto err_conf;

  pt->header = header;
  pt->pt.size = size;
  pt->pt.data_head = &header->aux_head;

  tinfo->conf.pt.size = (unsigned int) size;
  return tinfo;

 err_conf:
  munmap((void *) header, PAGE_SIZE);

 err_file:
  close (pt->file);

 err:
  xfree (tinfo);
  return NULL;
}

#else /* !defined (PERF_ATTR_SIZE_VER5) */

static struct btrace_target_info *
linux_enable_pt (ptid_t ptid, const struct btrace_config_pt *conf)
{
  errno = EOPNOTSUPP;
  return NULL;
}

#endif /* !defined (PERF_ATTR_SIZE_VER5) */

/* See linux-btrace.h.  */

struct btrace_target_info *
linux_enable_btrace (ptid_t ptid, const struct btrace_config *conf)
{
  struct btrace_target_info *tinfo;

  tinfo = NULL;
  switch (conf->format)
    {
    case BTRACE_FORMAT_NONE:
      break;

    case BTRACE_FORMAT_BTS:
      tinfo = linux_enable_bts (ptid, &conf->bts);
      break;

    case BTRACE_FORMAT_PT:
      tinfo = linux_enable_pt (ptid, &conf->pt);
      break;
    }

  return tinfo;
}

/* Disable BTS tracing.  */

static enum btrace_error
linux_disable_bts (struct btrace_tinfo_bts *tinfo)
{
  munmap((void *) tinfo->header, tinfo->bts.size + PAGE_SIZE);
  close (tinfo->file);

  return BTRACE_ERR_NONE;
}

/* Disable Intel Processor Trace tracing.  */

static enum btrace_error
linux_disable_pt (struct btrace_tinfo_pt *tinfo)
{
  munmap((void *) tinfo->pt.mem, tinfo->pt.size);
  munmap((void *) tinfo->header, PAGE_SIZE);
  close (tinfo->file);

  return BTRACE_ERR_NONE;
}

/* See linux-btrace.h.  */

enum btrace_error
linux_disable_btrace (struct btrace_target_info *tinfo)
{
  enum btrace_error errcode;

  errcode = BTRACE_ERR_NOT_SUPPORTED;
  switch (tinfo->conf.format)
    {
    case BTRACE_FORMAT_NONE:
      break;

    case BTRACE_FORMAT_BTS:
      errcode = linux_disable_bts (&tinfo->variant.bts);
      break;

    case BTRACE_FORMAT_PT:
      errcode = linux_disable_pt (&tinfo->variant.pt);
      break;
    }

  if (errcode == BTRACE_ERR_NONE)
    xfree (tinfo);

  return errcode;
}

/* Read branch trace data in BTS format for the thread given by TINFO into
   BTRACE using the TYPE reading method.  */

static enum btrace_error
linux_read_bts (struct btrace_data_bts *btrace,
		struct btrace_target_info *tinfo,
		enum btrace_read_type type)
{
  struct perf_event_buffer *pevent;
  const uint8_t *begin, *end, *start;
  size_t buffer_size, size;
  __u64 data_head, data_tail;
  unsigned int retries = 5;

  pevent = &tinfo->variant.bts.bts;

  /* For delta reads, we return at least the partial last block containing
     the current PC.  */
  if (type == BTRACE_READ_NEW && !perf_event_new_data (pevent))
    return BTRACE_ERR_NONE;

  buffer_size = pevent->size;
  data_tail = pevent->last_head;

  /* We may need to retry reading the trace.  See below.  */
  while (retries--)
    {
      data_head = *pevent->data_head;

      /* Delete any leftover trace from the previous iteration.  */
      VEC_free (btrace_block_s, btrace->blocks);

      if (type == BTRACE_READ_DELTA)
	{
	  __u64 data_size;

	  /* Determine the number of bytes to read and check for buffer
	     overflows.  */

	  /* Check for data head overflows.  We might be able to recover from
	     those but they are very unlikely and it's not really worth the
	     effort, I think.  */
	  if (data_head < data_tail)
	    return BTRACE_ERR_OVERFLOW;

	  /* If the buffer is smaller than the trace delta, we overflowed.  */
	  data_size = data_head - data_tail;
	  if (buffer_size < data_size)
	    return BTRACE_ERR_OVERFLOW;

	  /* DATA_SIZE <= BUFFER_SIZE and therefore fits into a size_t.  */
	  size = (size_t) data_size;
	}
      else
	{
	  /* Read the entire buffer.  */
	  size = buffer_size;

	  /* Adjust the size if the buffer has not overflowed, yet.  */
	  if (data_head < size)
	    size = (size_t) data_head;
	}

      /* Data_head keeps growing; the buffer itself is circular.  */
      begin = pevent->mem;
      start = begin + data_head % buffer_size;

      if (data_head <= buffer_size)
	end = start;
      else
	end = begin + pevent->size;

      btrace->blocks = perf_event_read_bts (tinfo, begin, end, start, size);

      /* The stopping thread notifies its ptracer before it is scheduled out.
	 On multi-core systems, the debugger might therefore run while the
	 kernel might be writing the last branch trace records.

	 Let's check whether the data head moved while we read the trace.  */
      if (data_head == *pevent->data_head)
	break;
    }

  pevent->last_head = data_head;

  /* Prune the incomplete last block (i.e. the first one of inferior execution)
     if we're not doing a delta read.  There is no way of filling in its zeroed
     BEGIN element.  */
  if (!VEC_empty (btrace_block_s, btrace->blocks)
      && type != BTRACE_READ_DELTA)
    VEC_pop (btrace_block_s, btrace->blocks);

  return BTRACE_ERR_NONE;
}

/* Fill in the Intel Processor Trace configuration information.  */

static void
linux_fill_btrace_pt_config (struct btrace_data_pt_config *conf)
{
  conf->cpu = btrace_this_cpu ();
}

/* Read branch trace data in Intel Processor Trace format for the thread
   given by TINFO into BTRACE using the TYPE reading method.  */

static enum btrace_error
linux_read_pt (struct btrace_data_pt *btrace,
	       struct btrace_target_info *tinfo,
	       enum btrace_read_type type)
{
  struct perf_event_buffer *pt;

  pt = &tinfo->variant.pt.pt;

  linux_fill_btrace_pt_config (&btrace->config);

  switch (type)
    {
    case BTRACE_READ_DELTA:
      /* We don't support delta reads.  The data head (i.e. aux_head) wraps
	 around to stay inside the aux buffer.  */
      return BTRACE_ERR_NOT_SUPPORTED;

    case BTRACE_READ_NEW:
      if (!perf_event_new_data (pt))
	return BTRACE_ERR_NONE;

      /* Fall through.  */
    case BTRACE_READ_ALL:
      perf_event_read_all (pt, &btrace->data, &btrace->size);
      return BTRACE_ERR_NONE;
    }

  internal_error (__FILE__, __LINE__, _("Unkown btrace read type."));
}

/* See linux-btrace.h.  */

enum btrace_error
linux_read_btrace (struct btrace_data *btrace,
		   struct btrace_target_info *tinfo,
		   enum btrace_read_type type)
{
  switch (tinfo->conf.format)
    {
    case BTRACE_FORMAT_NONE:
      return BTRACE_ERR_NOT_SUPPORTED;

    case BTRACE_FORMAT_BTS:
      /* We read btrace in BTS format.  */
      btrace->format = BTRACE_FORMAT_BTS;
      btrace->variant.bts.blocks = NULL;

      return linux_read_bts (&btrace->variant.bts, tinfo, type);

    case BTRACE_FORMAT_PT:
      /* We read btrace in Intel Processor Trace format.  */
      btrace->format = BTRACE_FORMAT_PT;
      btrace->variant.pt.data = NULL;
      btrace->variant.pt.size = 0;

      return linux_read_pt (&btrace->variant.pt, tinfo, type);
    }

  internal_error (__FILE__, __LINE__, _("Unkown branch trace format."));
}

/* See linux-btrace.h.  */

const struct btrace_config *
linux_btrace_conf (const struct btrace_target_info *tinfo)
{
  return &tinfo->conf;
}

#else /* !HAVE_LINUX_PERF_EVENT_H */

/* See linux-btrace.h.  */

int
linux_supports_btrace (struct target_ops *ops, enum btrace_format format)
{
  return 0;
}

/* See linux-btrace.h.  */

struct btrace_target_info *
linux_enable_btrace (ptid_t ptid, const struct btrace_config *conf)
{
  return NULL;
}

/* See linux-btrace.h.  */

enum btrace_error
linux_disable_btrace (struct btrace_target_info *tinfo)
{
  return BTRACE_ERR_NOT_SUPPORTED;
}

/* See linux-btrace.h.  */

enum btrace_error
linux_read_btrace (struct btrace_data *btrace,
		   struct btrace_target_info *tinfo,
		   enum btrace_read_type type)
{
  return BTRACE_ERR_NOT_SUPPORTED;
}

/* See linux-btrace.h.  */

const struct btrace_config *
linux_btrace_conf (const struct btrace_target_info *tinfo)
{
  return NULL;
}

#endif /* !HAVE_LINUX_PERF_EVENT_H */
