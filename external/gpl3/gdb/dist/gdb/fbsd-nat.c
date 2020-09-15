/* Native-dependent code for FreeBSD.

   Copyright (C) 2002-2020 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdbsupport/byte-vector.h"
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "regset.h"
#include "gdbarch.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "gdbsupport/gdb_wait.h"
#include "inf-ptrace.h"
#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/ptrace.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#if defined(HAVE_KINFO_GETFILE) || defined(HAVE_KINFO_GETVMMAP)
#include <libutil.h>
#endif
#if !defined(HAVE_KINFO_GETVMMAP)
#include "gdbsupport/filestuff.h"
#endif

#include "elf-bfd.h"
#include "fbsd-nat.h"
#include "fbsd-tdep.h"

#include <list>

/* Return the name of a file that can be opened to get the symbols for
   the child process identified by PID.  */

char *
fbsd_nat_target::pid_to_exec_file (int pid)
{
  ssize_t len;
  static char buf[PATH_MAX];
  char name[PATH_MAX];

#ifdef KERN_PROC_PATHNAME
  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = pid;
  buflen = sizeof buf;
  if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0)
    /* The kern.proc.pathname.<pid> sysctl returns a length of zero
       for processes without an associated executable such as kernel
       processes.  */
    return buflen == 0 ? NULL : buf;
#endif

  xsnprintf (name, PATH_MAX, "/proc/%d/exe", pid);
  len = readlink (name, buf, PATH_MAX - 1);
  if (len != -1)
    {
      buf[len] = '\0';
      return buf;
    }

  return NULL;
}

#ifdef HAVE_KINFO_GETVMMAP
/* Iterate over all the memory regions in the current inferior,
   calling FUNC for each memory region.  DATA is passed as the last
   argument to FUNC.  */

int
fbsd_nat_target::find_memory_regions (find_memory_region_ftype func,
				      void *data)
{
  pid_t pid = inferior_ptid.pid ();
  struct kinfo_vmentry *kve;
  uint64_t size;
  int i, nitems;

  gdb::unique_xmalloc_ptr<struct kinfo_vmentry>
    vmentl (kinfo_getvmmap (pid, &nitems));
  if (vmentl == NULL)
    perror_with_name (_("Couldn't fetch VM map entries."));

  for (i = 0, kve = vmentl.get (); i < nitems; i++, kve++)
    {
      /* Skip unreadable segments and those where MAP_NOCORE has been set.  */
      if (!(kve->kve_protection & KVME_PROT_READ)
	  || kve->kve_flags & KVME_FLAG_NOCOREDUMP)
	continue;

      /* Skip segments with an invalid type.  */
      if (kve->kve_type != KVME_TYPE_DEFAULT
	  && kve->kve_type != KVME_TYPE_VNODE
	  && kve->kve_type != KVME_TYPE_SWAP
	  && kve->kve_type != KVME_TYPE_PHYS)
	continue;

      size = kve->kve_end - kve->kve_start;
      if (info_verbose)
	{
	  fprintf_filtered (gdb_stdout, 
			    "Save segment, %ld bytes at %s (%c%c%c)\n",
			    (long) size,
			    paddress (target_gdbarch (), kve->kve_start),
			    kve->kve_protection & KVME_PROT_READ ? 'r' : '-',
			    kve->kve_protection & KVME_PROT_WRITE ? 'w' : '-',
			    kve->kve_protection & KVME_PROT_EXEC ? 'x' : '-');
	}

      /* Invoke the callback function to create the corefile segment.
	 Pass MODIFIED as true, we do not know the real modification state.  */
      func (kve->kve_start, size, kve->kve_protection & KVME_PROT_READ,
	    kve->kve_protection & KVME_PROT_WRITE,
	    kve->kve_protection & KVME_PROT_EXEC, 1, data);
    }
  return 0;
}
#else
static int
fbsd_read_mapping (FILE *mapfile, unsigned long *start, unsigned long *end,
		   char *protection)
{
  /* FreeBSD 5.1-RELEASE uses a 256-byte buffer.  */
  char buf[256];
  int resident, privateresident;
  unsigned long obj;
  int ret = EOF;

  /* As of FreeBSD 5.0-RELEASE, the layout is described in
     /usr/src/sys/fs/procfs/procfs_map.c.  Somewhere in 5.1-CURRENT a
     new column was added to the procfs map.  Therefore we can't use
     fscanf since we need to support older releases too.  */
  if (fgets (buf, sizeof buf, mapfile) != NULL)
    ret = sscanf (buf, "%lx %lx %d %d %lx %s", start, end,
		  &resident, &privateresident, &obj, protection);

  return (ret != 0 && ret != EOF);
}

/* Iterate over all the memory regions in the current inferior,
   calling FUNC for each memory region.  DATA is passed as the last
   argument to FUNC.  */

int
fbsd_nat_target::find_memory_regions (find_memory_region_ftype func,
				      void *data)
{
  pid_t pid = inferior_ptid.pid ();
  unsigned long start, end, size;
  char protection[4];
  int read, write, exec;

  std::string mapfilename = string_printf ("/proc/%ld/map", (long) pid);
  gdb_file_up mapfile (fopen (mapfilename.c_str (), "r"));
  if (mapfile == NULL)
    error (_("Couldn't open %s."), mapfilename.c_str ());

  if (info_verbose)
    fprintf_filtered (gdb_stdout, 
		      "Reading memory regions from %s\n", mapfilename.c_str ());

  /* Now iterate until end-of-file.  */
  while (fbsd_read_mapping (mapfile.get (), &start, &end, &protection[0]))
    {
      size = end - start;

      read = (strchr (protection, 'r') != 0);
      write = (strchr (protection, 'w') != 0);
      exec = (strchr (protection, 'x') != 0);

      if (info_verbose)
	{
	  fprintf_filtered (gdb_stdout, 
			    "Save segment, %ld bytes at %s (%c%c%c)\n",
			    size, paddress (target_gdbarch (), start),
			    read ? 'r' : '-',
			    write ? 'w' : '-',
			    exec ? 'x' : '-');
	}

      /* Invoke the callback function to create the corefile segment.
	 Pass MODIFIED as true, we do not know the real modification state.  */
      func (start, size, read, write, exec, 1, data);
    }

  return 0;
}
#endif

/* Fetch the command line for a running process.  */

static gdb::unique_xmalloc_ptr<char>
fbsd_fetch_cmdline (pid_t pid)
{
  size_t len;
  int mib[4];

  len = 0;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_ARGS;
  mib[3] = pid;
  if (sysctl (mib, 4, NULL, &len, NULL, 0) == -1)
    return nullptr;

  if (len == 0)
    return nullptr;

  gdb::unique_xmalloc_ptr<char> cmdline ((char *) xmalloc (len));
  if (sysctl (mib, 4, cmdline.get (), &len, NULL, 0) == -1)
    return nullptr;

  /* Join the arguments with spaces to form a single string.  */
  char *cp = cmdline.get ();
  for (size_t i = 0; i < len - 1; i++)
    if (cp[i] == '\0')
      cp[i] = ' ';
  cp[len - 1] = '\0';

  return cmdline;
}

/* Fetch the external variant of the kernel's internal process
   structure for the process PID into KP.  */

static bool
fbsd_fetch_kinfo_proc (pid_t pid, struct kinfo_proc *kp)
{
  size_t len;
  int mib[4];

  len = sizeof *kp;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = pid;
  return (sysctl (mib, 4, kp, &len, NULL, 0) == 0);
}

/* Implement the "info_proc" target_ops method.  */

bool
fbsd_nat_target::info_proc (const char *args, enum info_proc_what what)
{
#ifdef HAVE_KINFO_GETFILE
  gdb::unique_xmalloc_ptr<struct kinfo_file> fdtbl;
  int nfd = 0;
#endif
  struct kinfo_proc kp;
  pid_t pid;
  bool do_cmdline = false;
  bool do_cwd = false;
  bool do_exe = false;
#ifdef HAVE_KINFO_GETFILE
  bool do_files = false;
#endif
#ifdef HAVE_KINFO_GETVMMAP
  bool do_mappings = false;
#endif
  bool do_status = false;

  switch (what)
    {
    case IP_MINIMAL:
      do_cmdline = true;
      do_cwd = true;
      do_exe = true;
      break;
#ifdef HAVE_KINFO_GETVMMAP
    case IP_MAPPINGS:
      do_mappings = true;
      break;
#endif
    case IP_STATUS:
    case IP_STAT:
      do_status = true;
      break;
    case IP_CMDLINE:
      do_cmdline = true;
      break;
    case IP_EXE:
      do_exe = true;
      break;
    case IP_CWD:
      do_cwd = true;
      break;
#ifdef HAVE_KINFO_GETFILE
    case IP_FILES:
      do_files = true;
      break;
#endif
    case IP_ALL:
      do_cmdline = true;
      do_cwd = true;
      do_exe = true;
#ifdef HAVE_KINFO_GETFILE
      do_files = true;
#endif
#ifdef HAVE_KINFO_GETVMMAP
      do_mappings = true;
#endif
      do_status = true;
      break;
    default:
      error (_("Not supported on this target."));
    }

  gdb_argv built_argv (args);
  if (built_argv.count () == 0)
    {
      pid = inferior_ptid.pid ();
      if (pid == 0)
	error (_("No current process: you must name one."));
    }
  else if (built_argv.count () == 1 && isdigit (built_argv[0][0]))
    pid = strtol (built_argv[0], NULL, 10);
  else
    error (_("Invalid arguments."));

  printf_filtered (_("process %d\n"), pid);
#ifdef HAVE_KINFO_GETFILE
  if (do_cwd || do_exe || do_files)
    fdtbl.reset (kinfo_getfile (pid, &nfd));
#endif

  if (do_cmdline)
    {
      gdb::unique_xmalloc_ptr<char> cmdline = fbsd_fetch_cmdline (pid);
      if (cmdline != nullptr)
	printf_filtered ("cmdline = '%s'\n", cmdline.get ());
      else
	warning (_("unable to fetch command line"));
    }
  if (do_cwd)
    {
      const char *cwd = NULL;
#ifdef HAVE_KINFO_GETFILE
      struct kinfo_file *kf = fdtbl.get ();
      for (int i = 0; i < nfd; i++, kf++)
	{
	  if (kf->kf_type == KF_TYPE_VNODE && kf->kf_fd == KF_FD_TYPE_CWD)
	    {
	      cwd = kf->kf_path;
	      break;
	    }
	}
#endif
      if (cwd != NULL)
	printf_filtered ("cwd = '%s'\n", cwd);
      else
	warning (_("unable to fetch current working directory"));
    }
  if (do_exe)
    {
      const char *exe = NULL;
#ifdef HAVE_KINFO_GETFILE
      struct kinfo_file *kf = fdtbl.get ();
      for (int i = 0; i < nfd; i++, kf++)
	{
	  if (kf->kf_type == KF_TYPE_VNODE && kf->kf_fd == KF_FD_TYPE_TEXT)
	    {
	      exe = kf->kf_path;
	      break;
	    }
	}
#endif
      if (exe == NULL)
	exe = pid_to_exec_file (pid);
      if (exe != NULL)
	printf_filtered ("exe = '%s'\n", exe);
      else
	warning (_("unable to fetch executable path name"));
    }
#ifdef HAVE_KINFO_GETFILE
  if (do_files)
    {
      struct kinfo_file *kf = fdtbl.get ();

      if (nfd > 0)
	{
	  fbsd_info_proc_files_header ();
	  for (int i = 0; i < nfd; i++, kf++)
	    fbsd_info_proc_files_entry (kf->kf_type, kf->kf_fd, kf->kf_flags,
					kf->kf_offset, kf->kf_vnode_type,
					kf->kf_sock_domain, kf->kf_sock_type,
					kf->kf_sock_protocol, &kf->kf_sa_local,
					&kf->kf_sa_peer, kf->kf_path);
	}
      else
	warning (_("unable to fetch list of open files"));
    }
#endif
#ifdef HAVE_KINFO_GETVMMAP
  if (do_mappings)
    {
      int nvment;
      gdb::unique_xmalloc_ptr<struct kinfo_vmentry>
	vmentl (kinfo_getvmmap (pid, &nvment));

      if (vmentl != nullptr)
	{
	  int addr_bit = TARGET_CHAR_BIT * sizeof (void *);
	  fbsd_info_proc_mappings_header (addr_bit);

	  struct kinfo_vmentry *kve = vmentl.get ();
	  for (int i = 0; i < nvment; i++, kve++)
	    fbsd_info_proc_mappings_entry (addr_bit, kve->kve_start,
					   kve->kve_end, kve->kve_offset,
					   kve->kve_flags, kve->kve_protection,
					   kve->kve_path);
	}
      else
	warning (_("unable to fetch virtual memory map"));
    }
#endif
  if (do_status)
    {
      if (!fbsd_fetch_kinfo_proc (pid, &kp))
	warning (_("Failed to fetch process information"));
      else
	{
	  const char *state;
	  int pgtok;

	  printf_filtered ("Name: %s\n", kp.ki_comm);
	  switch (kp.ki_stat)
	    {
	    case SIDL:
	      state = "I (idle)";
	      break;
	    case SRUN:
	      state = "R (running)";
	      break;
	    case SSTOP:
	      state = "T (stopped)";
	      break;
	    case SZOMB:
	      state = "Z (zombie)";
	      break;
	    case SSLEEP:
	      state = "S (sleeping)";
	      break;
	    case SWAIT:
	      state = "W (interrupt wait)";
	      break;
	    case SLOCK:
	      state = "L (blocked on lock)";
	      break;
	    default:
	      state = "? (unknown)";
	      break;
	    }
	  printf_filtered ("State: %s\n", state);
	  printf_filtered ("Parent process: %d\n", kp.ki_ppid);
	  printf_filtered ("Process group: %d\n", kp.ki_pgid);
	  printf_filtered ("Session id: %d\n", kp.ki_sid);
	  printf_filtered ("TTY: %ju\n", (uintmax_t) kp.ki_tdev);
	  printf_filtered ("TTY owner process group: %d\n", kp.ki_tpgid);
	  printf_filtered ("User IDs (real, effective, saved): %d %d %d\n",
			   kp.ki_ruid, kp.ki_uid, kp.ki_svuid);
	  printf_filtered ("Group IDs (real, effective, saved): %d %d %d\n",
			   kp.ki_rgid, kp.ki_groups[0], kp.ki_svgid);
	  printf_filtered ("Groups: ");
	  for (int i = 0; i < kp.ki_ngroups; i++)
	    printf_filtered ("%d ", kp.ki_groups[i]);
	  printf_filtered ("\n");
	  printf_filtered ("Minor faults (no memory page): %ld\n",
			   kp.ki_rusage.ru_minflt);
	  printf_filtered ("Minor faults, children: %ld\n",
			   kp.ki_rusage_ch.ru_minflt);
	  printf_filtered ("Major faults (memory page faults): %ld\n",
			   kp.ki_rusage.ru_majflt);
	  printf_filtered ("Major faults, children: %ld\n",
			   kp.ki_rusage_ch.ru_majflt);
	  printf_filtered ("utime: %jd.%06ld\n",
			   (intmax_t) kp.ki_rusage.ru_utime.tv_sec,
			   kp.ki_rusage.ru_utime.tv_usec);
	  printf_filtered ("stime: %jd.%06ld\n",
			   (intmax_t) kp.ki_rusage.ru_stime.tv_sec,
			   kp.ki_rusage.ru_stime.tv_usec);
	  printf_filtered ("utime, children: %jd.%06ld\n",
			   (intmax_t) kp.ki_rusage_ch.ru_utime.tv_sec,
			   kp.ki_rusage_ch.ru_utime.tv_usec);
	  printf_filtered ("stime, children: %jd.%06ld\n",
			   (intmax_t) kp.ki_rusage_ch.ru_stime.tv_sec,
			   kp.ki_rusage_ch.ru_stime.tv_usec);
	  printf_filtered ("'nice' value: %d\n", kp.ki_nice);
	  printf_filtered ("Start time: %jd.%06ld\n", kp.ki_start.tv_sec,
			   kp.ki_start.tv_usec);
	  pgtok = getpagesize () / 1024;
	  printf_filtered ("Virtual memory size: %ju kB\n",
			   (uintmax_t) kp.ki_size / 1024);
	  printf_filtered ("Data size: %ju kB\n",
			   (uintmax_t) kp.ki_dsize * pgtok);
	  printf_filtered ("Stack size: %ju kB\n",
			   (uintmax_t) kp.ki_ssize * pgtok);
	  printf_filtered ("Text size: %ju kB\n",
			   (uintmax_t) kp.ki_tsize * pgtok);
	  printf_filtered ("Resident set size: %ju kB\n",
			   (uintmax_t) kp.ki_rssize * pgtok);
	  printf_filtered ("Maximum RSS: %ju kB\n",
			   (uintmax_t) kp.ki_rusage.ru_maxrss);
	  printf_filtered ("Pending Signals: ");
	  for (int i = 0; i < _SIG_WORDS; i++)
	    printf_filtered ("%08x ", kp.ki_siglist.__bits[i]);
	  printf_filtered ("\n");
	  printf_filtered ("Ignored Signals: ");
	  for (int i = 0; i < _SIG_WORDS; i++)
	    printf_filtered ("%08x ", kp.ki_sigignore.__bits[i]);
	  printf_filtered ("\n");
	  printf_filtered ("Caught Signals: ");
	  for (int i = 0; i < _SIG_WORDS; i++)
	    printf_filtered ("%08x ", kp.ki_sigcatch.__bits[i]);
	  printf_filtered ("\n");
	}
    }

  return true;
}

/*
 * The current layout of siginfo_t on FreeBSD was adopted in SVN
 * revision 153154 which shipped in FreeBSD versions 7.0 and later.
 * Don't bother supporting the older layout on older kernels.  The
 * older format was also never used in core dump notes.
 */
#if __FreeBSD_version >= 700009
#define USE_SIGINFO
#endif

#ifdef USE_SIGINFO
/* Return the size of siginfo for the current inferior.  */

#ifdef __LP64__
union sigval32 {
  int sival_int;
  uint32_t sival_ptr;
};

/* This structure matches the naming and layout of `siginfo_t' in
   <sys/signal.h>.  In particular, the `si_foo' macros defined in that
   header can be used with both types to copy fields in the `_reason'
   union.  */

struct siginfo32
{
  int si_signo;
  int si_errno;
  int si_code;
  __pid_t si_pid;
  __uid_t si_uid;
  int si_status;
  uint32_t si_addr;
  union sigval32 si_value;
  union
  {
    struct
    {
      int _trapno;
    } _fault;
    struct
    {
      int _timerid;
      int _overrun;
    } _timer;
    struct
    {
      int _mqd;
    } _mesgq;
    struct
    {
      int32_t _band;
    } _poll;
    struct
    {
      int32_t __spare1__;
      int __spare2__[7];
    } __spare__;
  } _reason;
};
#endif

static size_t
fbsd_siginfo_size ()
{
#ifdef __LP64__
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If so, use the 32-bit siginfo size.  */
  if (gdbarch_long_bit (gdbarch) == 32)
    return sizeof (struct siginfo32);
#endif
  return sizeof (siginfo_t);
}

/* Convert a native 64-bit siginfo object to a 32-bit object.  Note
   that FreeBSD doesn't support writing to $_siginfo, so this only
   needs to convert one way.  */

static void
fbsd_convert_siginfo (siginfo_t *si)
{
#ifdef __LP64__
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If not, nothing to do.  */
  if (gdbarch_long_bit (gdbarch) != 32)
    return;

  struct siginfo32 si32;

  si32.si_signo = si->si_signo;
  si32.si_errno = si->si_errno;
  si32.si_code = si->si_code;
  si32.si_pid = si->si_pid;
  si32.si_uid = si->si_uid;
  si32.si_status = si->si_status;
  si32.si_addr = (uintptr_t) si->si_addr;

  /* If sival_ptr is being used instead of sival_int on a big-endian
     platform, then sival_int will be zero since it holds the upper
     32-bits of the pointer value.  */
#if _BYTE_ORDER == _BIG_ENDIAN
  if (si->si_value.sival_int == 0)
    si32.si_value.sival_ptr = (uintptr_t) si->si_value.sival_ptr;
  else
    si32.si_value.sival_int = si->si_value.sival_int;
#else
  si32.si_value.sival_int = si->si_value.sival_int;
#endif

  /* Always copy the spare fields and then possibly overwrite them for
     signal-specific or code-specific fields.  */
  si32._reason.__spare__.__spare1__ = si->_reason.__spare__.__spare1__;
  for (int i = 0; i < 7; i++)
    si32._reason.__spare__.__spare2__[i] = si->_reason.__spare__.__spare2__[i];
  switch (si->si_signo) {
  case SIGILL:
  case SIGFPE:
  case SIGSEGV:
  case SIGBUS:
    si32.si_trapno = si->si_trapno;
    break;
  }
  switch (si->si_code) {
  case SI_TIMER:
    si32.si_timerid = si->si_timerid;
    si32.si_overrun = si->si_overrun;
    break;
  case SI_MESGQ:
    si32.si_mqd = si->si_mqd;
    break;
  }

  memcpy(si, &si32, sizeof (si32));
#endif
}
#endif

/* Implement the "xfer_partial" target_ops method.  */

enum target_xfer_status
fbsd_nat_target::xfer_partial (enum target_object object,
			       const char *annex, gdb_byte *readbuf,
			       const gdb_byte *writebuf,
			       ULONGEST offset, ULONGEST len,
			       ULONGEST *xfered_len)
{
  pid_t pid = inferior_ptid.pid ();

  switch (object)
    {
#ifdef USE_SIGINFO
    case TARGET_OBJECT_SIGNAL_INFO:
      {
	struct ptrace_lwpinfo pl;
	size_t siginfo_size;

	/* FreeBSD doesn't support writing to $_siginfo.  */
	if (writebuf != NULL)
	  return TARGET_XFER_E_IO;

	if (inferior_ptid.lwp_p ())
	  pid = inferior_ptid.lwp ();

	siginfo_size = fbsd_siginfo_size ();
	if (offset > siginfo_size)
	  return TARGET_XFER_E_IO;

	if (ptrace (PT_LWPINFO, pid, (PTRACE_TYPE_ARG3) &pl, sizeof (pl)) == -1)
	  return TARGET_XFER_E_IO;

	if (!(pl.pl_flags & PL_FLAG_SI))
	  return TARGET_XFER_E_IO;

	fbsd_convert_siginfo (&pl.pl_siginfo);
	if (offset + len > siginfo_size)
	  len = siginfo_size - offset;

	memcpy (readbuf, ((gdb_byte *) &pl.pl_siginfo) + offset, len);
	*xfered_len = len;
	return TARGET_XFER_OK;
      }
#endif
#ifdef KERN_PROC_AUXV
    case TARGET_OBJECT_AUXV:
      {
	gdb::byte_vector buf_storage;
	gdb_byte *buf;
	size_t buflen;
	int mib[4];

	if (writebuf != NULL)
	  return TARGET_XFER_E_IO;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_AUXV;
	mib[3] = pid;
	if (offset == 0)
	  {
	    buf = readbuf;
	    buflen = len;
	  }
	else
	  {
	    buflen = offset + len;
	    buf_storage.resize (buflen);
	    buf = buf_storage.data ();
	  }
	if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0)
	  {
	    if (offset != 0)
	      {
		if (buflen > offset)
		  {
		    buflen -= offset;
		    memcpy (readbuf, buf + offset, buflen);
		  }
		else
		  buflen = 0;
	      }
	    *xfered_len = buflen;
	    return (buflen == 0) ? TARGET_XFER_EOF : TARGET_XFER_OK;
	  }
	return TARGET_XFER_E_IO;
      }
#endif
#if defined(KERN_PROC_VMMAP) && defined(KERN_PROC_PS_STRINGS)
    case TARGET_OBJECT_FREEBSD_VMMAP:
    case TARGET_OBJECT_FREEBSD_PS_STRINGS:
      {
	gdb::byte_vector buf_storage;
	gdb_byte *buf;
	size_t buflen;
	int mib[4];

	int proc_target;
	uint32_t struct_size;
	switch (object)
	  {
	  case TARGET_OBJECT_FREEBSD_VMMAP:
	    proc_target = KERN_PROC_VMMAP;
	    struct_size = sizeof (struct kinfo_vmentry);
	    break;
	  case TARGET_OBJECT_FREEBSD_PS_STRINGS:
	    proc_target = KERN_PROC_PS_STRINGS;
	    struct_size = sizeof (void *);
	    break;
	  }

	if (writebuf != NULL)
	  return TARGET_XFER_E_IO;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = proc_target;
	mib[3] = pid;

	if (sysctl (mib, 4, NULL, &buflen, NULL, 0) != 0)
	  return TARGET_XFER_E_IO;
	buflen += sizeof (struct_size);

	if (offset >= buflen)
	  {
	    *xfered_len = 0;
	    return TARGET_XFER_EOF;
	  }

	buf_storage.resize (buflen);
	buf = buf_storage.data ();

	memcpy (buf, &struct_size, sizeof (struct_size));
	buflen -= sizeof (struct_size);
	if (sysctl (mib, 4, buf + sizeof (struct_size), &buflen, NULL, 0) != 0)
	  return TARGET_XFER_E_IO;
	buflen += sizeof (struct_size);

	if (buflen - offset < len)
	  len = buflen - offset;
	memcpy (readbuf, buf + offset, len);
	*xfered_len = len;
	return TARGET_XFER_OK;
      }
#endif
    default:
      return inf_ptrace_target::xfer_partial (object, annex,
					      readbuf, writebuf, offset,
					      len, xfered_len);
    }
}

#ifdef PT_LWPINFO
static bool debug_fbsd_lwp;
static bool debug_fbsd_nat;

static void
show_fbsd_lwp_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of FreeBSD lwp module is %s.\n"), value);
}

static void
show_fbsd_nat_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of FreeBSD native target is %s.\n"),
		    value);
}

/*
  FreeBSD's first thread support was via a "reentrant" version of libc
  (libc_r) that first shipped in 2.2.7.  This library multiplexed all
  of the threads in a process onto a single kernel thread.  This
  library was supported via the bsd-uthread target.

  FreeBSD 5.1 introduced two new threading libraries that made use of
  multiple kernel threads.  The first (libkse) scheduled M user
  threads onto N (<= M) kernel threads (LWPs).  The second (libthr)
  bound each user thread to a dedicated kernel thread.  libkse shipped
  as the default threading library (libpthread).

  FreeBSD 5.3 added a libthread_db to abstract the interface across
  the various thread libraries (libc_r, libkse, and libthr).

  FreeBSD 7.0 switched the default threading library from from libkse
  to libpthread and removed libc_r.

  FreeBSD 8.0 removed libkse and the in-kernel support for it.  The
  only threading library supported by 8.0 and later is libthr which
  ties each user thread directly to an LWP.  To simplify the
  implementation, this target only supports LWP-backed threads using
  ptrace directly rather than libthread_db.

  FreeBSD 11.0 introduced LWP event reporting via PT_LWP_EVENTS.
*/

/* Return true if PTID is still active in the inferior.  */

bool
fbsd_nat_target::thread_alive (ptid_t ptid)
{
  if (ptid.lwp_p ())
    {
      struct ptrace_lwpinfo pl;

      if (ptrace (PT_LWPINFO, ptid.lwp (), (caddr_t) &pl, sizeof pl)
	  == -1)
	return false;
#ifdef PL_FLAG_EXITED
      if (pl.pl_flags & PL_FLAG_EXITED)
	return false;
#endif
    }

  return true;
}

/* Convert PTID to a string.  */

std::string
fbsd_nat_target::pid_to_str (ptid_t ptid)
{
  lwpid_t lwp;

  lwp = ptid.lwp ();
  if (lwp != 0)
    {
      int pid = ptid.pid ();

      return string_printf ("LWP %d of process %d", lwp, pid);
    }

  return normal_pid_to_str (ptid);
}

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME
/* Return the name assigned to a thread by an application.  Returns
   the string in a static buffer.  */

const char *
fbsd_nat_target::thread_name (struct thread_info *thr)
{
  struct ptrace_lwpinfo pl;
  struct kinfo_proc kp;
  int pid = thr->ptid.pid ();
  long lwp = thr->ptid.lwp ();
  static char buf[sizeof pl.pl_tdname + 1];

  /* Note that ptrace_lwpinfo returns the process command in pl_tdname
     if a name has not been set explicitly.  Return a NULL name in
     that case.  */
  if (!fbsd_fetch_kinfo_proc (pid, &kp))
    perror_with_name (_("Failed to fetch process information"));
  if (ptrace (PT_LWPINFO, lwp, (caddr_t) &pl, sizeof pl) == -1)
    perror_with_name (("ptrace"));
  if (strcmp (kp.ki_comm, pl.pl_tdname) == 0)
    return NULL;
  xsnprintf (buf, sizeof buf, "%s", pl.pl_tdname);
  return buf;
}
#endif

/* Enable additional event reporting on new processes.

   To catch fork events, PTRACE_FORK is set on every traced process
   to enable stops on returns from fork or vfork.  Note that both the
   parent and child will always stop, even if system call stops are
   not enabled.

   To catch LWP events, PTRACE_EVENTS is set on every traced process.
   This enables stops on the birth for new LWPs (excluding the "main" LWP)
   and the death of LWPs (excluding the last LWP in a process).  Note
   that unlike fork events, the LWP that creates a new LWP does not
   report an event.  */

static void
fbsd_enable_proc_events (pid_t pid)
{
#ifdef PT_GET_EVENT_MASK
  int events;

  if (ptrace (PT_GET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3)&events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace"));
  events |= PTRACE_FORK | PTRACE_LWP;
#ifdef PTRACE_VFORK
  events |= PTRACE_VFORK;
#endif
  if (ptrace (PT_SET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3)&events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace"));
#else
#ifdef TDP_RFPPWAIT
  if (ptrace (PT_FOLLOW_FORK, pid, (PTRACE_TYPE_ARG3)0, 1) == -1)
    perror_with_name (("ptrace"));
#endif
#ifdef PT_LWP_EVENTS
  if (ptrace (PT_LWP_EVENTS, pid, (PTRACE_TYPE_ARG3)0, 1) == -1)
    perror_with_name (("ptrace"));
#endif
#endif
}

/* Add threads for any new LWPs in a process.

   When LWP events are used, this function is only used to detect existing
   threads when attaching to a process.  On older systems, this function is
   called to discover new threads each time the thread list is updated.  */

static void
fbsd_add_threads (fbsd_nat_target *target, pid_t pid)
{
  int i, nlwps;

  gdb_assert (!in_thread_list (target, ptid_t (pid)));
  nlwps = ptrace (PT_GETNUMLWPS, pid, NULL, 0);
  if (nlwps == -1)
    perror_with_name (("ptrace"));

  gdb::unique_xmalloc_ptr<lwpid_t[]> lwps (XCNEWVEC (lwpid_t, nlwps));

  nlwps = ptrace (PT_GETLWPLIST, pid, (caddr_t) lwps.get (), nlwps);
  if (nlwps == -1)
    perror_with_name (("ptrace"));

  for (i = 0; i < nlwps; i++)
    {
      ptid_t ptid = ptid_t (pid, lwps[i], 0);

      if (!in_thread_list (target, ptid))
	{
#ifdef PT_LWP_EVENTS
	  struct ptrace_lwpinfo pl;

	  /* Don't add exited threads.  Note that this is only called
	     when attaching to a multi-threaded process.  */
	  if (ptrace (PT_LWPINFO, lwps[i], (caddr_t) &pl, sizeof pl) == -1)
	    perror_with_name (("ptrace"));
	  if (pl.pl_flags & PL_FLAG_EXITED)
	    continue;
#endif
	  if (debug_fbsd_lwp)
	    fprintf_unfiltered (gdb_stdlog,
				"FLWP: adding thread for LWP %u\n",
				lwps[i]);
	  add_thread (target, ptid);
	}
    }
}

/* Implement the "update_thread_list" target_ops method.  */

void
fbsd_nat_target::update_thread_list ()
{
#ifdef PT_LWP_EVENTS
  /* With support for thread events, threads are added/deleted from the
     list as events are reported, so just try deleting exited threads.  */
  delete_exited_threads ();
#else
  prune_threads ();

  fbsd_add_threads (this, inferior_ptid.pid ());
#endif
}

#ifdef TDP_RFPPWAIT
/*
  To catch fork events, PT_FOLLOW_FORK is set on every traced process
  to enable stops on returns from fork or vfork.  Note that both the
  parent and child will always stop, even if system call stops are not
  enabled.

  After a fork, both the child and parent process will stop and report
  an event.  However, there is no guarantee of order.  If the parent
  reports its stop first, then fbsd_wait explicitly waits for the new
  child before returning.  If the child reports its stop first, then
  the event is saved on a list and ignored until the parent's stop is
  reported.  fbsd_wait could have been changed to fetch the parent PID
  of the new child and used that to wait for the parent explicitly.
  However, if two threads in the parent fork at the same time, then
  the wait on the parent might return the "wrong" fork event.

  The initial version of PT_FOLLOW_FORK did not set PL_FLAG_CHILD for
  the new child process.  This flag could be inferred by treating any
  events for an unknown pid as a new child.

  In addition, the initial version of PT_FOLLOW_FORK did not report a
  stop event for the parent process of a vfork until after the child
  process executed a new program or exited.  The kernel was changed to
  defer the wait for exit or exec of the child until after posting the
  stop event shortly after the change to introduce PL_FLAG_CHILD.
  This could be worked around by reporting a vfork event when the
  child event posted and ignoring the subsequent event from the
  parent.

  This implementation requires both of these fixes for simplicity's
  sake.  FreeBSD versions newer than 9.1 contain both fixes.
*/

static std::list<ptid_t> fbsd_pending_children;

/* Record a new child process event that is reported before the
   corresponding fork event in the parent.  */

static void
fbsd_remember_child (ptid_t pid)
{
  fbsd_pending_children.push_front (pid);
}

/* Check for a previously-recorded new child process event for PID.
   If one is found, remove it from the list and return the PTID.  */

static ptid_t
fbsd_is_child_pending (pid_t pid)
{
  for (auto it = fbsd_pending_children.begin ();
       it != fbsd_pending_children.end (); it++)
    if (it->pid () == pid)
      {
	ptid_t ptid = *it;
	fbsd_pending_children.erase (it);
	return ptid;
      }
  return null_ptid;
}

#ifndef PTRACE_VFORK
static std::forward_list<ptid_t> fbsd_pending_vfork_done;

/* Record a pending vfork done event.  */

static void
fbsd_add_vfork_done (ptid_t pid)
{
  fbsd_pending_vfork_done.push_front (pid);
}

/* Check for a pending vfork done event for a specific PID.  */

static int
fbsd_is_vfork_done_pending (pid_t pid)
{
  for (auto it = fbsd_pending_vfork_done.begin ();
       it != fbsd_pending_vfork_done.end (); it++)
    if (it->pid () == pid)
      return 1;
  return 0;
}

/* Check for a pending vfork done event.  If one is found, remove it
   from the list and return the PTID.  */

static ptid_t
fbsd_next_vfork_done (void)
{
  if (!fbsd_pending_vfork_done.empty ())
    {
      ptid_t ptid = fbsd_pending_vfork_done.front ();
      fbsd_pending_vfork_done.pop_front ();
      return ptid;
    }
  return null_ptid;
}
#endif
#endif

/* Implement the "resume" target_ops method.  */

void
fbsd_nat_target::resume (ptid_t ptid, int step, enum gdb_signal signo)
{
#if defined(TDP_RFPPWAIT) && !defined(PTRACE_VFORK)
  pid_t pid;

  /* Don't PT_CONTINUE a process which has a pending vfork done event.  */
  if (minus_one_ptid == ptid)
    pid = inferior_ptid.pid ();
  else
    pid = ptid.pid ();
  if (fbsd_is_vfork_done_pending (pid))
    return;
#endif

  if (debug_fbsd_lwp)
    fprintf_unfiltered (gdb_stdlog,
			"FLWP: fbsd_resume for ptid (%d, %ld, %ld)\n",
			ptid.pid (), ptid.lwp (),
			ptid.tid ());
  if (ptid.lwp_p ())
    {
      /* If ptid is a specific LWP, suspend all other LWPs in the process.  */
      inferior *inf = find_inferior_ptid (this, ptid);

      for (thread_info *tp : inf->non_exited_threads ())
        {
	  int request;

	  if (tp->ptid.lwp () == ptid.lwp ())
	    request = PT_RESUME;
	  else
	    request = PT_SUSPEND;

	  if (ptrace (request, tp->ptid.lwp (), NULL, 0) == -1)
	    perror_with_name (("ptrace"));
	}
    }
  else
    {
      /* If ptid is a wildcard, resume all matching threads (they won't run
	 until the process is continued however).  */
      for (thread_info *tp : all_non_exited_threads (this, ptid))
	if (ptrace (PT_RESUME, tp->ptid.lwp (), NULL, 0) == -1)
	  perror_with_name (("ptrace"));
      ptid = inferior_ptid;
    }

#if __FreeBSD_version < 1200052
  /* When multiple threads within a process wish to report STOPPED
     events from wait(), the kernel picks one thread event as the
     thread event to report.  The chosen thread event is retrieved via
     PT_LWPINFO by passing the process ID as the request pid.  If
     multiple events are pending, then the subsequent wait() after
     resuming a process will report another STOPPED event after
     resuming the process to handle the next thread event and so on.

     A single thread event is cleared as a side effect of resuming the
     process with PT_CONTINUE, PT_STEP, etc.  In older kernels,
     however, the request pid was used to select which thread's event
     was cleared rather than always clearing the event that was just
     reported.  To avoid clearing the event of the wrong LWP, always
     pass the process ID instead of an LWP ID to PT_CONTINUE or
     PT_SYSCALL.

     In the case of stepping, the process ID cannot be used with
     PT_STEP since it would step the thread that reported an event
     which may not be the thread indicated by PTID.  For stepping, use
     PT_SETSTEP to enable stepping on the desired thread before
     resuming the process via PT_CONTINUE instead of using
     PT_STEP.  */
  if (step)
    {
      if (ptrace (PT_SETSTEP, get_ptrace_pid (ptid), NULL, 0) == -1)
	perror_with_name (("ptrace"));
      step = 0;
    }
  ptid = ptid_t (ptid.pid ());
#endif
  inf_ptrace_target::resume (ptid, step, signo);
}

#ifdef USE_SIGTRAP_SIGINFO
/* Handle breakpoint and trace traps reported via SIGTRAP.  If the
   trap was a breakpoint or trace trap that should be reported to the
   core, return true.  */

static bool
fbsd_handle_debug_trap (fbsd_nat_target *target, ptid_t ptid,
			const struct ptrace_lwpinfo &pl)
{

  /* Ignore traps without valid siginfo or for signals other than
     SIGTRAP.

     FreeBSD kernels prior to r341800 can return stale siginfo for at
     least some events, but those events can be identified by
     additional flags set in pl_flags.  True breakpoint and
     single-step traps should not have other flags set in
     pl_flags.  */
  if (pl.pl_flags != PL_FLAG_SI || pl.pl_siginfo.si_signo != SIGTRAP)
    return false;

  /* Trace traps are either a single step or a hardware watchpoint or
     breakpoint.  */
  if (pl.pl_siginfo.si_code == TRAP_TRACE)
    {
      if (debug_fbsd_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "FNAT: trace trap for LWP %ld\n", ptid.lwp ());
      return true;
    }

  if (pl.pl_siginfo.si_code == TRAP_BRKPT)
    {
      /* Fixup PC for the software breakpoint.  */
      struct regcache *regcache = get_thread_regcache (target, ptid);
      struct gdbarch *gdbarch = regcache->arch ();
      int decr_pc = gdbarch_decr_pc_after_break (gdbarch);

      if (debug_fbsd_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "FNAT: sw breakpoint trap for LWP %ld\n",
			    ptid.lwp ());
      if (decr_pc != 0)
	{
	  CORE_ADDR pc;

	  pc = regcache_read_pc (regcache);
	  regcache_write_pc (regcache, pc - decr_pc);
	}
      return true;
    }

  return false;
}
#endif

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

ptid_t
fbsd_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
		       int target_options)
{
  ptid_t wptid;

  while (1)
    {
#ifndef PTRACE_VFORK
      wptid = fbsd_next_vfork_done ();
      if (wptid != null_ptid)
	{
	  ourstatus->kind = TARGET_WAITKIND_VFORK_DONE;
	  return wptid;
	}
#endif
      wptid = inf_ptrace_target::wait (ptid, ourstatus, target_options);
      if (ourstatus->kind == TARGET_WAITKIND_STOPPED)
	{
	  struct ptrace_lwpinfo pl;
	  pid_t pid;
	  int status;

	  pid = wptid.pid ();
	  if (ptrace (PT_LWPINFO, pid, (caddr_t) &pl, sizeof pl) == -1)
	    perror_with_name (("ptrace"));

	  wptid = ptid_t (pid, pl.pl_lwpid, 0);

	  if (debug_fbsd_nat)
	    {
	      fprintf_unfiltered (gdb_stdlog,
				  "FNAT: stop for LWP %u event %d flags %#x\n",
				  pl.pl_lwpid, pl.pl_event, pl.pl_flags);
	      if (pl.pl_flags & PL_FLAG_SI)
		fprintf_unfiltered (gdb_stdlog,
				    "FNAT: si_signo %u si_code %u\n",
				    pl.pl_siginfo.si_signo,
				    pl.pl_siginfo.si_code);
	    }

#ifdef PT_LWP_EVENTS
	  if (pl.pl_flags & PL_FLAG_EXITED)
	    {
	      /* If GDB attaches to a multi-threaded process, exiting
		 threads might be skipped during post_attach that
		 have not yet reported their PL_FLAG_EXITED event.
		 Ignore EXITED events for an unknown LWP.  */
	      thread_info *thr = find_thread_ptid (this, wptid);
	      if (thr != nullptr)
		{
		  if (debug_fbsd_lwp)
		    fprintf_unfiltered (gdb_stdlog,
					"FLWP: deleting thread for LWP %u\n",
					pl.pl_lwpid);
		  if (print_thread_events)
		    printf_unfiltered (_("[%s exited]\n"),
				       target_pid_to_str (wptid).c_str ());
		  delete_thread (thr);
		}
	      if (ptrace (PT_CONTINUE, pid, (caddr_t) 1, 0) == -1)
		perror_with_name (("ptrace"));
	      continue;
	    }
#endif

	  /* Switch to an LWP PTID on the first stop in a new process.
	     This is done after handling PL_FLAG_EXITED to avoid
	     switching to an exited LWP.  It is done before checking
	     PL_FLAG_BORN in case the first stop reported after
	     attaching to an existing process is a PL_FLAG_BORN
	     event.  */
	  if (in_thread_list (this, ptid_t (pid)))
	    {
	      if (debug_fbsd_lwp)
		fprintf_unfiltered (gdb_stdlog,
				    "FLWP: using LWP %u for first thread\n",
				    pl.pl_lwpid);
	      thread_change_ptid (this, ptid_t (pid), wptid);
	    }

#ifdef PT_LWP_EVENTS
	  if (pl.pl_flags & PL_FLAG_BORN)
	    {
	      /* If GDB attaches to a multi-threaded process, newborn
		 threads might be added by fbsd_add_threads that have
		 not yet reported their PL_FLAG_BORN event.  Ignore
		 BORN events for an already-known LWP.  */
	      if (!in_thread_list (this, wptid))
		{
		  if (debug_fbsd_lwp)
		    fprintf_unfiltered (gdb_stdlog,
					"FLWP: adding thread for LWP %u\n",
					pl.pl_lwpid);
		  add_thread (this, wptid);
		}
	      ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
	      return wptid;
	    }
#endif

#ifdef TDP_RFPPWAIT
	  if (pl.pl_flags & PL_FLAG_FORKED)
	    {
#ifndef PTRACE_VFORK
	      struct kinfo_proc kp;
#endif
	      ptid_t child_ptid;
	      pid_t child;

	      child = pl.pl_child_pid;
	      ourstatus->kind = TARGET_WAITKIND_FORKED;
#ifdef PTRACE_VFORK
	      if (pl.pl_flags & PL_FLAG_VFORKED)
		ourstatus->kind = TARGET_WAITKIND_VFORKED;
#endif

	      /* Make sure the other end of the fork is stopped too.  */
	      child_ptid = fbsd_is_child_pending (child);
	      if (child_ptid == null_ptid)
		{
		  pid = waitpid (child, &status, 0);
		  if (pid == -1)
		    perror_with_name (("waitpid"));

		  gdb_assert (pid == child);

		  if (ptrace (PT_LWPINFO, child, (caddr_t)&pl, sizeof pl) == -1)
		    perror_with_name (("ptrace"));

		  gdb_assert (pl.pl_flags & PL_FLAG_CHILD);
		  child_ptid = ptid_t (child, pl.pl_lwpid, 0);
		}

	      /* Enable additional events on the child process.  */
	      fbsd_enable_proc_events (child_ptid.pid ());

#ifndef PTRACE_VFORK
	      /* For vfork, the child process will have the P_PPWAIT
		 flag set.  */
	      if (fbsd_fetch_kinfo_proc (child, &kp))
		{
		  if (kp.ki_flag & P_PPWAIT)
		    ourstatus->kind = TARGET_WAITKIND_VFORKED;
		}
	      else
		warning (_("Failed to fetch process information"));
#endif
	      ourstatus->value.related_pid = child_ptid;

	      return wptid;
	    }

	  if (pl.pl_flags & PL_FLAG_CHILD)
	    {
	      /* Remember that this child forked, but do not report it
		 until the parent reports its corresponding fork
		 event.  */
	      fbsd_remember_child (wptid);
	      continue;
	    }

#ifdef PTRACE_VFORK
	  if (pl.pl_flags & PL_FLAG_VFORK_DONE)
	    {
	      ourstatus->kind = TARGET_WAITKIND_VFORK_DONE;
	      return wptid;
	    }
#endif
#endif

#ifdef PL_FLAG_EXEC
	  if (pl.pl_flags & PL_FLAG_EXEC)
	    {
	      ourstatus->kind = TARGET_WAITKIND_EXECD;
	      ourstatus->value.execd_pathname
		= xstrdup (pid_to_exec_file (pid));
	      return wptid;
	    }
#endif

#ifdef USE_SIGTRAP_SIGINFO
	  if (fbsd_handle_debug_trap (this, wptid, pl))
	    return wptid;
#endif

	  /* Note that PL_FLAG_SCE is set for any event reported while
	     a thread is executing a system call in the kernel.  In
	     particular, signals that interrupt a sleep in a system
	     call will report this flag as part of their event.  Stops
	     explicitly for system call entry and exit always use
	     SIGTRAP, so only treat SIGTRAP events as system call
	     entry/exit events.  */
	  if (pl.pl_flags & (PL_FLAG_SCE | PL_FLAG_SCX)
	      && ourstatus->value.sig == SIGTRAP)
	    {
#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
	      if (catch_syscall_enabled ())
		{
		  if (catching_syscall_number (pl.pl_syscall_code))
		    {
		      if (pl.pl_flags & PL_FLAG_SCE)
			ourstatus->kind = TARGET_WAITKIND_SYSCALL_ENTRY;
		      else
			ourstatus->kind = TARGET_WAITKIND_SYSCALL_RETURN;
		      ourstatus->value.syscall_number = pl.pl_syscall_code;
		      return wptid;
		    }
		}
#endif
	      /* If the core isn't interested in this event, just
		 continue the process explicitly and wait for another
		 event.  Note that PT_SYSCALL is "sticky" on FreeBSD
		 and once system call stops are enabled on a process
		 it stops for all system call entries and exits.  */
	      if (ptrace (PT_CONTINUE, pid, (caddr_t) 1, 0) == -1)
		perror_with_name (("ptrace"));
	      continue;
	    }
	}
      return wptid;
    }
}

#ifdef USE_SIGTRAP_SIGINFO
/* Implement the "stopped_by_sw_breakpoint" target_ops method.  */

bool
fbsd_nat_target::stopped_by_sw_breakpoint ()
{
  struct ptrace_lwpinfo pl;

  if (ptrace (PT_LWPINFO, get_ptrace_pid (inferior_ptid), (caddr_t) &pl,
	      sizeof pl) == -1)
    return false;

  return (pl.pl_flags == PL_FLAG_SI
	  && pl.pl_siginfo.si_signo == SIGTRAP
	  && pl.pl_siginfo.si_code == TRAP_BRKPT);
}

/* Implement the "supports_stopped_by_sw_breakpoint" target_ops
   method.  */

bool
fbsd_nat_target::supports_stopped_by_sw_breakpoint ()
{
  return true;
}
#endif

#ifdef TDP_RFPPWAIT
/* Target hook for follow_fork.  On entry and at return inferior_ptid is
   the ptid of the followed inferior.  */

bool
fbsd_nat_target::follow_fork (bool follow_child, bool detach_fork)
{
  if (!follow_child && detach_fork)
    {
      struct thread_info *tp = inferior_thread ();
      pid_t child_pid = tp->pending_follow.value.related_pid.pid ();

      /* Breakpoints have already been detached from the child by
	 infrun.c.  */

      if (ptrace (PT_DETACH, child_pid, (PTRACE_TYPE_ARG3)1, 0) == -1)
	perror_with_name (("ptrace"));

#ifndef PTRACE_VFORK
      if (tp->pending_follow.kind == TARGET_WAITKIND_VFORKED)
	{
	  /* We can't insert breakpoints until the child process has
	     finished with the shared memory region.  The parent
	     process doesn't wait for the child process to exit or
	     exec until after it has been resumed from the ptrace stop
	     to report the fork.  Once it has been resumed it doesn't
	     stop again before returning to userland, so there is no
	     reliable way to wait on the parent.

	     We can't stay attached to the child to wait for an exec
	     or exit because it may invoke ptrace(PT_TRACE_ME)
	     (e.g. if the parent process is a debugger forking a new
	     child process).

	     In the end, the best we can do is to make sure it runs
	     for a little while.  Hopefully it will be out of range of
	     any breakpoints we reinsert.  Usually this is only the
	     single-step breakpoint at vfork's return point.  */

	  usleep (10000);

	  /* Schedule a fake VFORK_DONE event to report on the next
	     wait.  */
	  fbsd_add_vfork_done (inferior_ptid);
	}
#endif
    }

  return false;
}

int
fbsd_nat_target::insert_fork_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::remove_fork_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::insert_vfork_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::remove_vfork_catchpoint (int pid)
{
  return 0;
}
#endif

/* Implement the "post_startup_inferior" target_ops method.  */

void
fbsd_nat_target::post_startup_inferior (ptid_t pid)
{
  fbsd_enable_proc_events (pid.pid ());
}

/* Implement the "post_attach" target_ops method.  */

void
fbsd_nat_target::post_attach (int pid)
{
  fbsd_enable_proc_events (pid);
  fbsd_add_threads (this, pid);
}

#ifdef PL_FLAG_EXEC
/* If the FreeBSD kernel supports PL_FLAG_EXEC, then traced processes
   will always stop after exec.  */

int
fbsd_nat_target::insert_exec_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::remove_exec_catchpoint (int pid)
{
  return 0;
}
#endif

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
int
fbsd_nat_target::set_syscall_catchpoint (int pid, bool needed,
					 int any_count,
					 gdb::array_view<const int> syscall_counts)
{

  /* Ignore the arguments.  inf-ptrace.c will use PT_SYSCALL which
     will catch all system call entries and exits.  The system calls
     are filtered by GDB rather than the kernel.  */
  return 0;
}
#endif
#endif

bool
fbsd_nat_target::supports_multi_process ()
{
  return true;
}

void _initialize_fbsd_nat ();
void
_initialize_fbsd_nat ()
{
#ifdef PT_LWPINFO
  add_setshow_boolean_cmd ("fbsd-lwp", class_maintenance,
			   &debug_fbsd_lwp, _("\
Set debugging of FreeBSD lwp module."), _("\
Show debugging of FreeBSD lwp module."), _("\
Enables printf debugging output."),
			   NULL,
			   &show_fbsd_lwp_debug,
			   &setdebuglist, &showdebuglist);
  add_setshow_boolean_cmd ("fbsd-nat", class_maintenance,
			   &debug_fbsd_nat, _("\
Set debugging of FreeBSD native target."), _("\
Show debugging of FreeBSD native target."), _("\
Enables printf debugging output."),
			   NULL,
			   &show_fbsd_nat_debug,
			   &setdebuglist, &showdebuglist);
#endif
}
