/*  This file is part of the program psim.

    Copyright (C) 1994-1998, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.

    */


#ifndef _EMUL_NETBSD_C_
#define _EMUL_NETBSD_C_


/* Note: this module is called via a table.  There is no benefit in
   making it inline */

#include "emul_generic.h"
#include "emul_netbsd.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/time.h>

#ifdef HAVE_GETRUSAGE
#ifndef HAVE_SYS_RESOURCE_H
#undef HAVE_GETRUSAGE
#endif
#endif

#ifdef HAVE_GETRUSAGE
#include <sys/resource.h>
int getrusage();
#endif

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#ifdef HAVE_UNISTD_H
#undef MAXPATHLEN		/* sys/param.h might define this also */
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#define WITH_NetBSD_HOST (NetBSD >= 199306)
#if WITH_NetBSD_HOST /* here NetBSD as that is what we're emulating */
#include <sys/syscall.h> /* FIXME - should not be including this one */
#include <sys/sysctl.h>
#include <sys/mount.h>
extern int getdirentries(int fd, char *buf, int nbytes, long *basep);

/* NetBSD post 2.0 has the statfs system call (if COMPAT_20), but does
   not have struct statfs.  In this case don't implement fstatfs.
   FIXME: Should implement fstatvfs.  */
#ifndef HAVE_STRUCT_STATFS
#undef HAVE_FSTATFS
#endif

#else

/* If this is not netbsd, don't allow fstatfs or getdirentries at this time */
#undef HAVE_FSTATFS
#undef HAVE_GETDIRENTRIES
#endif

#if (BSD < 199306) /* here BSD as just a bug */
extern int errno;
#endif

#ifndef STATIC_INLINE_EMUL_NETBSD
#define STATIC_INLINE_EMUL_NETBSD STATIC_INLINE
#endif


#if WITH_NetBSD_HOST
#define SYS(X) ASSERT(call == (SYS_##X))
#else
#define SYS(X)
#endif

#if WITH_NetBSD_HOST && (PATH_MAX != 1024)
#error "PATH_MAX not 1024"
#elif !defined(PATH_MAX)
#define PATH_MAX 1024
#endif


/* EMULATION

   NetBSD - Emulation of user programs for NetBSD/PPC

   DESCRIPTION

   */


/* NetBSD's idea of what is needed to implement emulations */

struct _os_emul_data {
  device *vm;
  emul_syscall *syscalls;
};



STATIC_INLINE_EMUL_NETBSD void
write_stat(unsigned_word addr,
	   struct stat buf,
	   cpu *processor,
	   unsigned_word cia)
{
  H2T(buf.st_dev);
  H2T(buf.st_ino);
  H2T(buf.st_mode);
  H2T(buf.st_nlink);
  H2T(buf.st_uid);
  H2T(buf.st_gid);
  H2T(buf.st_size);
  H2T(buf.st_atime);
  /* H2T(buf.st_spare1); */
  H2T(buf.st_mtime);
  /* H2T(buf.st_spare2); */
  H2T(buf.st_ctime);
  /* H2T(buf.st_spare3); */
#ifdef AC_STRUCT_ST_RDEV
  H2T(buf.st_rdev);
#endif
#ifdef AC_STRUCT_ST_BLKSIZE
  H2T(buf.st_blksize);
#endif
#ifdef AC_STRUCT_ST_BLOCKS
  H2T(buf.st_blocks);
#endif
#if WITH_NetBSD_HOST
  H2T(buf.st_flags);
  H2T(buf.st_gen);
#endif
  emul_write_buffer(&buf, addr, sizeof(buf), processor, cia);
}


#ifdef HAVE_FSTATFS
STATIC_INLINE_EMUL_NETBSD void
write_statfs(unsigned_word addr,
	     struct statfs buf,
	     cpu *processor,
	     unsigned_word cia)
{
  H2T(buf.f_type);
  H2T(buf.f_flags);
  H2T(buf.f_bsize);
  H2T(buf.f_iosize);
  H2T(buf.f_blocks);
  H2T(buf.f_bfree);
  H2T(buf.f_bavail);
  H2T(buf.f_files);
  H2T(buf.f_ffree);
  H2T(buf.f_fsid.val[0]);
  H2T(buf.f_fsid.val[1]);
  H2T(buf.f_owner);
  /* f_spare[4]; */
  /* f_fstypename[MFSNAMELEN]; */
  /* f_mntonname[MNAMELEN]; */
  /* f_mntfromname[MNAMELEN]; */
  emul_write_buffer(&buf, addr, sizeof(buf), processor, cia);
}
#endif


STATIC_INLINE_EMUL_NETBSD void
write_timeval(unsigned_word addr,
	      struct timeval t,
	      cpu *processor,
	      unsigned_word cia)
{
  H2T(t.tv_sec);
  H2T(t.tv_usec);
  emul_write_buffer(&t, addr, sizeof(t), processor, cia);
}

#ifdef HAVE_GETTIMEOFDAY
STATIC_INLINE_EMUL_NETBSD void
write_timezone(unsigned_word addr,
	       struct timezone tz,
	       cpu *processor,
	       unsigned_word cia)
{
  H2T(tz.tz_minuteswest);
  H2T(tz.tz_dsttime);
  emul_write_buffer(&tz, addr, sizeof(tz), processor, cia);
}
#endif

#ifdef HAVE_GETDIRENTRIES
STATIC_INLINE_EMUL_NETBSD void
write_direntries(unsigned_word addr,
		 char *buf,
		 int nbytes,
		 cpu *processor,
		 unsigned_word cia)
{
  while (nbytes > 0) {
    struct dirent *out;
    struct dirent *in = (struct dirent*)buf;
    ASSERT(in->d_reclen <= nbytes);
    out = (struct dirent*)zalloc(in->d_reclen);
    memcpy(out/*dest*/, in/*src*/, in->d_reclen);
    H2T(out->d_fileno);
    H2T(out->d_reclen);
    H2T(out->d_type);
    H2T(out->d_namlen);
    emul_write_buffer(out, addr, in->d_reclen, processor, cia);
    nbytes -= in->d_reclen;
    addr += in->d_reclen;
    buf += in->d_reclen;
    free(out);
  }
}
#endif


#ifdef HAVE_GETRUSAGE
STATIC_INLINE_EMUL_NETBSD void
write_rusage(unsigned_word addr,
	     struct rusage rusage,
	     cpu *processor,
	     unsigned_word cia)
{
  H2T(rusage.ru_utime.tv_sec); /* user time used */
  H2T(rusage.ru_utime.tv_usec);
  H2T(rusage.ru_stime.tv_sec); /* system time used */
  H2T(rusage.ru_stime.tv_usec);
  H2T(rusage.ru_maxrss);          /* integral max resident set size */
  H2T(rusage.ru_ixrss);           /* integral shared text memory size */
  H2T(rusage.ru_idrss);           /* integral unshared data size */
  H2T(rusage.ru_isrss);           /* integral unshared stack size */
  H2T(rusage.ru_minflt);          /* page reclaims */
  H2T(rusage.ru_majflt);          /* page faults */
  H2T(rusage.ru_nswap);           /* swaps */
  H2T(rusage.ru_inblock);         /* block input operations */
  H2T(rusage.ru_oublock);         /* block output operations */
  H2T(rusage.ru_msgsnd);          /* messages sent */
  H2T(rusage.ru_msgrcv);          /* messages received */
  H2T(rusage.ru_nsignals);        /* signals received */
  H2T(rusage.ru_nvcsw);           /* voluntary context switches */
  H2T(rusage.ru_nivcsw);          /* involuntary context switches */
  emul_write_buffer(&rusage, addr, sizeof(rusage), processor, cia);
}
#endif


/* File descriptors 0, 1, and 2 should not be closed.  fd_closed[]
   tracks whether these descriptors have been closed in do_close()
   below.  */

static int fd_closed[3];

/* Check for some occurrences of bad file descriptors.  We only check
   whether fd 0, 1, or 2 are "closed".  By "closed" we mean that these
   descriptors aren't actually closed, but are considered to be closed
   by this layer.

   Other checks are performed by the underlying OS call.  */

static int
fdbad (int fd)
{
  if (fd >=0 && fd <= 2 && fd_closed[fd])
    {
      errno = EBADF;
      return -1;
    }
  return 0;
}

static void
do_exit(os_emul_data *emul,
	unsigned call,
	const int arg0,
	cpu *processor,
	unsigned_word cia)
{
  int status = (int)cpu_registers(processor)->gpr[arg0];
  SYS(exit);
  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d)\n", status);

  cpu_halt(processor, cia, was_exited, status);
}


static void
do_read(os_emul_data *emul,
	unsigned call,
	const int arg0,
	cpu *processor,
	unsigned_word cia)
{
  void *scratch_buffer;
  int d = (int)cpu_registers(processor)->gpr[arg0];
  unsigned_word buf = cpu_registers(processor)->gpr[arg0+1];
  int nbytes = cpu_registers(processor)->gpr[arg0+2];
  int status;
  SYS(read);

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, 0x%lx, %d", d, (long)buf, nbytes);

  /* get a tempoary bufer */
  scratch_buffer = zalloc(nbytes);

  /* check if buffer exists by reading it */
  emul_read_buffer(scratch_buffer, buf, nbytes, processor, cia);

  /* read */
#if 0
  if (d == 0) {
    status = fread (scratch_buffer, 1, nbytes, stdin);
    if (status == 0 && ferror (stdin))
      status = -1;
  }
#endif
  status = fdbad (d);
  if (status == 0)
    status = read (d, scratch_buffer, nbytes);

  emul_write_status(processor, status, errno);
  if (status > 0)
    emul_write_buffer(scratch_buffer, buf, status, processor, cia);

  free(scratch_buffer);
}


static void
do_write(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  void *scratch_buffer = NULL;
  int d = (int)cpu_registers(processor)->gpr[arg0];
  unsigned_word buf = cpu_registers(processor)->gpr[arg0+1];
  int nbytes = cpu_registers(processor)->gpr[arg0+2];
  int status;
  SYS(write);

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, 0x%lx, %d", d, (long)buf, nbytes);

  /* get a tempoary bufer */
  scratch_buffer = zalloc(nbytes); /* FIXME - nbytes == 0 */

  /* copy in */
  emul_read_buffer(scratch_buffer, buf, nbytes,
		   processor, cia);

  /* write */
  status = fdbad (d);
  if (status == 0)
    status = write(d, scratch_buffer, nbytes);

  emul_write_status(processor, status, errno);
  free(scratch_buffer);

  flush_stdoutput();
}


static void
do_open(os_emul_data *emul,
	unsigned call,
	const int arg0,
	cpu *processor,
	unsigned_word cia)
{
  unsigned_word path_addr = cpu_registers(processor)->gpr[arg0];
  char path_buf[PATH_MAX];
  char *path = emul_read_string(path_buf, path_addr, PATH_MAX, processor, cia);
  int flags = (int)cpu_registers(processor)->gpr[arg0+1];
  int mode = (int)cpu_registers(processor)->gpr[arg0+2];
  int hostflags;
  int status;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("0x%lx [%s], 0x%x, 0x%x", (long)path_addr, path, flags, mode);

  SYS(open);

  /* Do some translation on 'flags' to match it to the host's version.  */
  /* These flag values were taken from the NetBSD 1.4 header files.  */
  if ((flags & 3) == 0)
    hostflags = O_RDONLY;
  else if ((flags & 3) == 1)
    hostflags = O_WRONLY;
  else
    hostflags = O_RDWR;
  if (flags & 0x00000008)
    hostflags |= O_APPEND;
  if (flags & 0x00000200)
    hostflags |= O_CREAT;
  if (flags & 0x00000400)
    hostflags |= O_TRUNC;
  if (flags & 0x00000800)
    hostflags |= O_EXCL;

  /* Can't combine these statements, cuz open sets errno. */
  status = open(path, hostflags, mode);
  emul_write_status(processor, status, errno);
}


static void
do_close(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  int d = (int)cpu_registers(processor)->gpr[arg0];
  int status;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d", d);

  SYS(close);

  status = fdbad (d);
  if (status == 0)
    {
      /* Do not close stdin, stdout, or stderr. GDB may still need access to
	 these descriptors.  */
      if (d == 0 || d == 1 || d == 2)
	{
	  fd_closed[d] = 1;
	  status = 0;
	}
      else
	status = close(d);
    }

  emul_write_status(processor, status, errno);
}


static void
do_break(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  /* just pass this onto the `vm' device */
  unsigned_word new_break = cpu_registers(processor)->gpr[arg0];
  int status;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("0x%lx", (long)cpu_registers(processor)->gpr[arg0]);

  SYS(break);
  status = device_ioctl(emul->vm,
			processor,
			cia,
			device_ioctl_break,
			new_break); /*ioctl-data*/
  emul_write_status(processor, 0, status);
}


#ifndef HAVE_GETPID
#define do_getpid 0
#else
static void
do_getpid(os_emul_data *emul,
	  unsigned call,
	  const int arg0,
	  cpu *processor,
	  unsigned_word cia)
{
  SYS(getpid);
  emul_write_status(processor, (int)getpid(), 0);
}
#endif

#ifndef HAVE_GETUID
#define do_getuid 0
#else
static void
do_getuid(os_emul_data *emul,
	  unsigned call,
	  const int arg0,
	  cpu *processor,
	  unsigned_word cia)
{
  SYS(getuid);
  emul_write_status(processor, (int)getuid(), 0);
}
#endif

#ifndef HAVE_GETEUID
#define do_geteuid 0
#else
static void
do_geteuid(os_emul_data *emul,
	   unsigned call,
	   const int arg0,
	   cpu *processor,
	   unsigned_word cia)
{
  SYS(geteuid);
  emul_write_status(processor, (int)geteuid(), 0);
}
#endif

#ifndef HAVE_KILL
#define do_kill 0
#else
static void
do_kill(os_emul_data *emul,
	unsigned call,
	const int arg0,
	cpu *processor,
	unsigned_word cia)
{
  pid_t pid = cpu_registers(processor)->gpr[arg0];
  int sig = cpu_registers(processor)->gpr[arg0+1];

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, %d", (int)pid, sig);

  SYS(kill);
  printf_filtered("SYS_kill at 0x%lx - more to this than just being killed\n",
		  (long)cia);
  cpu_halt(processor, cia, was_signalled, sig);
}
#endif

#ifndef HAVE_DUP
#define do_dup 0
#else
static void
do_dup(os_emul_data *emul,
       unsigned call,
       const int arg0,
       cpu *processor,
       unsigned_word cia)
{
  int oldd = cpu_registers(processor)->gpr[arg0];
  int status = (fdbad (oldd) < 0) ? -1 : dup(oldd);
  int err = errno;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d", oldd);

  SYS(dup);
  emul_write_status(processor, status, err);
}
#endif

#ifndef HAVE_GETEGID
#define do_getegid 0
#else
static void
do_getegid(os_emul_data *emul,
	   unsigned call,
	   const int arg0,
	   cpu *processor,
	   unsigned_word cia)
{
  SYS(getegid);
  emul_write_status(processor, (int)getegid(), 0);
}
#endif

#ifndef HAVE_GETGID
#define do_getgid 0
#else
static void
do_getgid(os_emul_data *emul,
	  unsigned call,
	  const int arg0,
	  cpu *processor,
	  unsigned_word cia)
{
  SYS(getgid);
  emul_write_status(processor, (int)getgid(), 0);
}
#endif

#ifndef HAVE_SIGPROCMASK
#define do_sigprocmask 0
#else
static void
do_sigprocmask(os_emul_data *emul,
	       unsigned call,
	       const int arg0,
	       cpu *processor,
	       unsigned_word cia)
{
  natural_word how = cpu_registers(processor)->gpr[arg0];
  unsigned_word set = cpu_registers(processor)->gpr[arg0+1];
  unsigned_word oset = cpu_registers(processor)->gpr[arg0+2];
#ifdef SYS_sigprocmask
  SYS(sigprocmask);
#endif

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%ld, 0x%ld, 0x%ld", (long)how, (long)set, (long)oset);

  emul_write_status(processor, 0, 0);
  cpu_registers(processor)->gpr[4] = set;
}
#endif

#ifndef HAVE_IOCTL
#define do_ioctl 0
#else
static void
do_ioctl(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  int d = cpu_registers(processor)->gpr[arg0];
  unsigned request = cpu_registers(processor)->gpr[arg0+1];
  unsigned_word argp_addr = cpu_registers(processor)->gpr[arg0+2];

#if !WITH_NetBSD_HOST
  cpu_registers(processor)->gpr[arg0] = 0; /* just succeed */
#else
  unsigned dir = request & IOC_DIRMASK;
  int status;
  SYS(ioctl);
  /* what we haven't done */
  if (dir & IOC_IN /* write into the io device */
      || dir & IOC_OUT
      || !(dir & IOC_VOID))
    error("do_ioctl() read or write of parameter not implemented\n");
  status = fdbad (d);
  if (status == 0)
    status = ioctl(d, request, NULL);
  emul_write_status(processor, status, errno);
#endif

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, 0x%x, 0x%lx", d, request, (long)argp_addr);
}
#endif

#ifndef HAVE_UMASK
#define do_umask 0
#else
static void
do_umask(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  int mask = cpu_registers(processor)->gpr[arg0];

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("0%o", mask);

  SYS(umask);
  emul_write_status(processor, umask(mask), 0);
}
#endif

#ifndef HAVE_DUP2
#define do_dup2 0
#else
static void
do_dup2(os_emul_data *emul,
	unsigned call,
	const int arg0,
	cpu *processor,
	unsigned_word cia)
{
  int oldd = cpu_registers(processor)->gpr[arg0];
  int newd = cpu_registers(processor)->gpr[arg0+1];
  int status = (fdbad (oldd) < 0) ? -1 : dup2(oldd, newd);
  int err = errno;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, %d", oldd, newd);

  SYS(dup2);
  emul_write_status(processor, status, err);
}
#endif

#ifndef HAVE_FCNTL
#define do_fcntl 0
#else
static void
do_fcntl(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  int fd = cpu_registers(processor)->gpr[arg0];
  int cmd = cpu_registers(processor)->gpr[arg0+1];
  int arg = cpu_registers(processor)->gpr[arg0+2];
  int status;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, %d, %d", fd, cmd, arg);

  SYS(fcntl);
  status = fdbad (fd);
  if (status == 0)
    status = fcntl(fd, cmd, arg);
  emul_write_status(processor, status, errno);
}
#endif

#ifndef HAVE_GETTIMEOFDAY
#define do_gettimeofday 0
#else
static void
do_gettimeofday(os_emul_data *emul,
		unsigned call,
		const int arg0,
		cpu *processor,
		unsigned_word cia)
{
  unsigned_word t_addr = cpu_registers(processor)->gpr[arg0];
  unsigned_word tz_addr = cpu_registers(processor)->gpr[arg0+1];
  struct timeval t;
  struct timezone tz;
  int status = gettimeofday((t_addr != 0 ? &t : NULL),
			    (tz_addr != 0 ? &tz : NULL));
  int err = errno;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("0x%lx, 0x%lx", (long)t_addr, (long)tz_addr);

  SYS(__gettimeofday50);
  emul_write_status(processor, status, err);
  if (status == 0) {
    if (t_addr != 0)
      write_timeval(t_addr, t, processor, cia);
    if (tz_addr != 0)
      write_timezone(tz_addr, tz, processor, cia);
  }
}
#endif

#ifndef HAVE_GETRUSAGE
#define do_getrusage 0
#else
static void
do_getrusage(os_emul_data *emul,
	     unsigned call,
	     const int arg0,
	     cpu *processor,
	     unsigned_word cia)
{
  int who = cpu_registers(processor)->gpr[arg0];
  unsigned_word rusage_addr = cpu_registers(processor)->gpr[arg0+1];
  struct rusage rusage;
  int status = getrusage(who, (rusage_addr != 0 ? &rusage : NULL));
  int err = errno;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, 0x%lx", who, (long)rusage_addr);

  SYS(__getrusage50);
  emul_write_status(processor, status, err);
  if (status == 0) {
    if (rusage_addr != 0)
      write_rusage(rusage_addr, rusage, processor, cia);
  }
}
#endif


#ifndef HAVE_FSTATFS
#define do_fstatfs 0
#else
static void
do_fstatfs(os_emul_data *emul,
	   unsigned call,
	   const int arg0,
	   cpu *processor,
	   unsigned_word cia)
{
  int fd = cpu_registers(processor)->gpr[arg0];
  unsigned_word buf_addr = cpu_registers(processor)->gpr[arg0+1];
  struct statfs buf;
  int status;

  if (WITH_TRACE && ppc_trace[trace_os_emul])
    printf_filtered ("%d, 0x%lx", fd, (long)buf_addr);

  SYS(fstatfs);
  status = fdbad (fd);
  if (status == 0)
    status = fstatfs(fd, (buf_addr == 0 ? NULL : &buf));
  emul_write_status(processor, status, errno);
  if (status == 0) {
    if (buf_addr != 0)
      write_statfs(buf_addr, buf, processor, cia);
  }
}
#endif

#ifndef HAVE_STAT
#define do_stat 0
#else
static void
do_stat(os_emul_data *emul,
	unsigned call,
	const int arg0,
	cpu *processor,
	unsigned_word cia)
{
  char path_buf[PATH_MAX];
  unsigned_word path_addr = cpu_registers(processor)->gpr[arg0];
  unsigned_word stat_buf_addr = cpu_registers(processor)->gpr[arg0+1];
  char *path = emul_read_string(path_buf, path_addr, PATH_MAX, processor, cia);
  struct stat buf;
  int status;
#ifdef SYS_stat
  SYS(stat);
#endif
  status = stat(path, &buf);
  emul_write_status(processor, status, errno);
  if (status == 0)
    write_stat(stat_buf_addr, buf, processor, cia);
}
#endif

#ifndef HAVE_FSTAT
#define do_fstat 0
#else
static void
do_fstat(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  int fd = cpu_registers(processor)->gpr[arg0];
  unsigned_word stat_buf_addr = cpu_registers(processor)->gpr[arg0+1];
  struct stat buf;
  int status;
#ifdef SYS_fstat
  SYS(fstat);
#endif
  /* Can't combine these statements, cuz fstat sets errno. */
  status = fdbad (fd);
  if (status == 0)
    status = fstat(fd, &buf);
  emul_write_status(processor, status, errno);
  write_stat(stat_buf_addr, buf, processor, cia);
}
#endif

#ifndef HAVE_LSTAT
#define do_lstat 0
#else
static void
do_lstat(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  char path_buf[PATH_MAX];
  unsigned_word path_addr = cpu_registers(processor)->gpr[arg0];
  char *path = emul_read_string(path_buf, path_addr, PATH_MAX, processor, cia);
  unsigned_word stat_buf_addr = cpu_registers(processor)->gpr[arg0+1];
  struct stat buf;
  int status;
#ifdef SYS_lstat
  SYS(lstat);
#endif
  /* Can't combine these statements, cuz lstat sets errno. */
  status = lstat(path, &buf);
  emul_write_status(processor, status, errno);
  write_stat(stat_buf_addr, buf, processor, cia);
}
#endif

#ifndef HAVE_GETDIRENTRIES
#define do_getdirentries 0
#else
static void
do_getdirentries(os_emul_data *emul,
		 unsigned call,
		 const int arg0,
		 cpu *processor,
		 unsigned_word cia)
{
  int fd = cpu_registers(processor)->gpr[arg0];
  unsigned_word buf_addr = cpu_registers(processor)->gpr[arg0+1];
  char *buf;
  int nbytes = cpu_registers(processor)->gpr[arg0+2];
  unsigned_word basep_addr = cpu_registers(processor)->gpr[arg0+3];
  long basep;
  int status;
#ifdef SYS_getdirentries
  SYS(getdirentries);
#endif
  if (buf_addr != 0 && nbytes >= 0)
    buf = zalloc(nbytes);
  else
    buf = NULL;
  status = getdirentries(fd,
			 (buf_addr == 0 ? NULL : buf),
			 nbytes,
			 (basep_addr == 0 ? NULL : &basep));
  emul_write_status(processor, status, errno);
  if (basep_addr != 0)
    emul_write_word(basep_addr, basep, processor, cia);
  if (status > 0)
    write_direntries(buf_addr, buf, status, processor, cia);
  if (buf != NULL)
    free(buf);
}
#endif


static void
do___syscall(os_emul_data *emul,
	     unsigned call,
	     const int arg0,
	     cpu *processor,
	     unsigned_word cia)
{
  SYS(__syscall);
  emul_do_system_call(emul,
		      emul->syscalls,
		      cpu_registers(processor)->gpr[arg0],
		      arg0 + 1,
		      processor,
		      cia);
}

#ifndef HAVE_LSEEK
#define do_lseek 0
#else
static void
do_lseek(os_emul_data *emul,
	 unsigned call,
	 const int arg0,
	 cpu *processor,
	 unsigned_word cia)
{
  int fildes = cpu_registers(processor)->gpr[arg0];
  off_t offset = emul_read_gpr64(processor, arg0+2);
  int whence = cpu_registers(processor)->gpr[arg0+4];
  off_t status;
  SYS(lseek);
  status = fdbad (fildes);
  if (status == 0)
    status = lseek(fildes, offset, whence);
  if (status == -1)
    emul_write_status(processor, -1, errno);
  else {
    emul_write_status(processor, 0, 0); /* success */
    emul_write_gpr64(processor, 3, status);
  }
}
#endif

static void
do___sysctl(os_emul_data *emul,
	    unsigned call,
	    const int arg0,
	    cpu *processor,
	    unsigned_word cia)
{
  /* call the arguments by their real name */
  unsigned_word name = cpu_registers(processor)->gpr[arg0];
  natural_word namelen = cpu_registers(processor)->gpr[arg0+1];
  unsigned_word oldp = cpu_registers(processor)->gpr[arg0+2];
  unsigned_word oldlenp = cpu_registers(processor)->gpr[arg0+3];
  natural_word oldlen;
  natural_word mib;
  natural_word int_val;
  SYS(__sysctl);

  /* pluck out the management information base id */
  if (namelen < 1)
    error("system_call()SYS___sysctl bad name[0]\n");
  mib = vm_data_map_read_word(cpu_data_map(processor),
			      name,
			      processor,
			      cia);
  name += sizeof(mib);

  /* see what to do with it ... */
  switch ((int)mib) {
  case 6/*CTL_HW*/:
#if WITH_NetBSD_HOST && (CTL_HW != 6)
#  error "CTL_HW"
#endif
    if (namelen < 2)
      error("system_call()SYS___sysctl - CTL_HW - bad name[1]\n");
    mib = vm_data_map_read_word(cpu_data_map(processor),
				name,
				processor,
				cia);
    name += sizeof(mib);
    switch ((int)mib) {
    case 7/*HW_PAGESIZE*/:
#if WITH_NetBSD_HOST && (HW_PAGESIZE != 7)
#  error "HW_PAGESIZE"
#endif
      oldlen = vm_data_map_read_word(cpu_data_map(processor),
				     oldlenp,
				     processor,
				     cia);
      if (sizeof(natural_word) > oldlen)
	error("system_call()sysctl - CTL_HW.HW_PAGESIZE - to small\n");
      int_val = 8192;
      oldlen = sizeof(int_val);
      emul_write_word(oldp, int_val, processor, cia);
      emul_write_word(oldlenp, oldlen, processor, cia);
      break;
    default:
      error("sysctl() CTL_HW.%d unknown\n", mib);
      break;
    }
    break;
  default:
    error("sysctl() name[0]=%d unknown\n", (int)mib);
    break;
  }
  emul_write_status(processor, 0, 0); /* always succeed */
}



static emul_syscall_descriptor netbsd_descriptors[] = {
  /* 0 */ { 0, "syscall" },
  /* 1 */ { do_exit, "exit" },
  /* 2 */ { 0, "fork" },
  /* 3 */ { do_read, "read" },
  /* 4 */ { do_write, "write" },
  /* 5 */ { do_open, "open" },
  /* 6 */ { do_close, "close" },
  { 0, }, /* 7 is old wait4 */
  { 0, }, /* 8 is old creat */
  /* 9 */ { 0, "link" },
  /* 10 */ { 0, "unlink" },
  { 0, }, /* 11 is obsolete execv */
  /* 12 */ { 0, "chdir" },
  /* 13 */ { 0, "fchdir" },
  { 0, }, /* 14 is old mknod */
  /* 15 */ { 0, "chmod" },
  /* 16 */ { 0, "chown" },
  /* 17 */ { do_break, "break" },
  { 0, }, /* 18 is old getfsstat */
  { 0, }, /* 19 is old lseek */
  /* 20 */ { do_getpid, "getpid" },
  { 0, }, /* 21 is old mount */
  /* 22 */ { 0, "unmount" },
  /* 23 */ { 0, "setuid" },
  /* 24 */ { do_getuid, "getuid" },
  /* 25 */ { do_geteuid, "geteuid" },
  /* 26 */ { 0, "ptrace" },
  /* 27 */ { 0, "recvmsg" },
  /* 28 */ { 0, "sendmsg" },
  /* 29 */ { 0, "recvfrom" },
  /* 30 */ { 0, "accept" },
  /* 31 */ { 0, "getpeername" },
  /* 32 */ { 0, "getsockname" },
  /* 33 */ { 0, "access" },
  /* 34 */ { 0, "chflags" },
  /* 35 */ { 0, "fchflags" },
  /* 36 */ { 0, "sync" },
  /* 37 */ { do_kill, "kill" },
  { 0, }, /* 38 is old stat */
  /* 39 */ { 0, "getppid" },
  { 0, }, /* 40 is old lstat */
  /* 41 */ { do_dup, "dup" },
  /* 42 */ { 0, "pipe" },
  /* 43 */ { do_getegid, "getegid" },
  /* 44 */ { 0, "profil" },
  /* 45 */ { 0, "ktrace" },
  { 0, }, /* 46 is old sigaction */
  /* 47 */ { do_getgid, "getgid" },
  { 0, }, /* 48 is old sigprocmask */
  /* 49 */ { 0, "getlogin" },
  /* 50 */ { 0, "setlogin" },
  /* 51 */ { 0, "acct" },
  { 0, }, /* 52 is old sigpending */
  { 0, }, /* 53 is old sigaltstack */
  /* 54 */ { do_ioctl, "ioctl" },
  { 0, }, /* 55 is old reboot */
  /* 56 */ { 0, "revoke" },
  /* 57 */ { 0, "symlink" },
  /* 58 */ { 0, "readlink" },
  /* 59 */ { 0, "execve" },
  /* 60 */ { do_umask, "umask" },
  /* 61 */ { 0, "chroot" },
  { 0, }, /* 62 is old fstat */
  { 0, }, /* 63 is old getkerninfo */
  { 0, }, /* 64 is old getpagesize */
  { 0, }, /* 65 is old msync */
  /* 66 */ { 0, "vfork" },
  { 0, }, /* 67 is obsolete vread */
  { 0, }, /* 68 is obsolete vwrite */
  /* 69 */ { 0, "sbrk" },
  { 0, }, /* 70 is obsolete sstk */
  { 0, }, /* 71 is old mmap */
  /* 72 */ { 0, "vadvise" },
  /* 73 */ { 0, "munmap" },
  /* 74 */ { 0, "mprotect" },
  /* 75 */ { 0, "madvise" },
  { 0, }, /* 76 is obsolete vhangup */
  { 0, }, /* 77 is obsolete vlimit */
  /* 78 */ { 0, "mincore" },
  /* 79 */ { 0, "getgroups" },
  /* 80 */ { 0, "setgroups" },
  /* 81 */ { 0, "getpgrp" },
  /* 82 */ { 0, "setpgid" },
  { 0, }, /* 83 is old setitimer */
  { 0, }, /* 84 is old wait */
  { 0, }, /* 85 is old swapon */
  { 0, }, /* 86 is old getitimer */
  { 0, }, /* 87 is old gethostname */
  { 0, }, /* 88 is old sethostname */
  { 0, }, /* 89 is old getdtablesize */
  { do_dup2, "dup2" },
  { 0, }, /* 91 */
  /* 92 */ { do_fcntl, "fcntl" },
  { 0, }, /* 93 is old select */
  { 0, }, /* 94 */
  /* 95 */ { 0, "fsync" },
  /* 96 */ { 0, "setpriority" },
  { 0, }, /* 97 is old socket */
  { 0, }, /* 98 is old connect */
  { 0, }, /* 99 is old accept */
  /* 100 */ { 0, "getpriority" },
  { 0, }, /* 101 is old send */
  { 0, }, /* 102 is old recv */
  { 0, }, /* 103 is old sigreturn */
  /* 104 */ { 0, "bind" },
  /* 105 */ { 0, "setsockopt" },
  /* 106 */ { 0, "listen" },
  { 0, }, /* 107 is obsolete vtimes */
  { 0, }, /* 108 is old sigvec */
  { 0, }, /* 109 is old sigblock */
  { 0, }, /* 110 is old sigsetmask */
  { 0, }, /* 111 is old sigsuspend */
  { 0, }, /* 112 is old sigstack */
  { 0, }, /* 113 is old recvmsg */
  { 0, }, /* 114 is old sendmsg */
  /* - is obsolete vtrace */ { 0, "vtrace	115" },
  { 0, }, /* 116 is old gettimeofday */
  { 0, }, /* 117 is old getrusage */
  /* 118 */ { 0, "getsockopt" },
  /* - is obsolete resuba */ { 0, "resuba	119" },
  /* 120 */ { 0, "readv" },
  /* 121 */ { 0, "writev" },
  { 0, }, /* 122 is old settimeofday */
  /* 123 */ { 0, "fchown" },
  /* 124 */ { 0, "fchmod" },
  { 0, }, /* 125 is old recvfrom */
  { 0, }, /* 126 is old setreuid */
  { 0, }, /* 127 is old setregid */
  /* 126 */ { 0, "setreuid" },
  /* 127 */ { 0, "setregid" },
  /* 128 */ { 0, "rename" },
  { 0, }, /* 129 is old truncate */
  { 0, }, /* 130 is old ftruncate */
  /* 131 */ { 0, "flock" },
  /* 132 */ { 0, "mkfifo" },
  /* 133 */ { 0, "sendto" },
  /* 134 */ { 0, "shutdown" },
  /* 135 */ { 0, "socketpair" },
  /* 136 */ { 0, "mkdir" },
  /* 137 */ { 0, "rmdir" },
  { 0, }, /* 138 is old utimes */
  { 0, }, /* 139 is obsolete 4.2 sigreturn */
  { 0, }, /* 140 is old adjtime */
  { 0, }, /* 141 is old getpeername */
  { 0, }, /* 142 is old gethostid */
  { 0, }, /* 143 is old sethostid */
  { 0, }, /* 144 is old getrlimit */
  { 0, }, /* 145 is old setrlimit */
  { 0, }, /* 146 is old killpg */
  /* 147 */ { 0, "setsid" },
  /* 148 */ { 0, "quotactl" },
  { 0, }, /* 149 is old quota */
  { 0, }, /* 150 is old getsockname */
  { 0, }, /* 151 */
  { 0, }, /* 152 */
  { 0, }, /* 153 */
  { 0, }, /* 154 */
  /* 155 */ { 0, "nfssvc" },
  { 0, }, /* 156 is old getdirentries */
  { 0, }, /* 157 is old statfs */
  { 0, }, /* 158 is old fstatfs */
  { 0, }, /* 159 */
  { 0, }, /* 160 */
  { 0, }, /* 161 is old getfh */
  { 0, }, /* 162 is old getdomainname */
  { 0, }, /* 163 is old setdomainname */
  { 0, }, /* 164 is old uname */
  /* 165 */ { 0, "sysarch" },
  { 0, }, /* 166 */
  { 0, }, /* 167 */
  { 0, }, /* 168 */
  { 0, }, /* 169 is old semsys */
  { 0, }, /* 170 is old msgsys */
  { 0, }, /* 171 is old shmsys */
  { 0, }, /* 172 */
  /* 173 */ { 0, "pread" },
  /* 174 */ { 0, "pwrite" },
  { 0, }, /* 175 is old ntp_gettime */
  /* 176 */ { 0, "ntp_adjtime" },
  { 0, }, /* 177 */
  { 0, }, /* 178 */
  { 0, }, /* 179 */
  { 0, }, /* 180 */
  /* 181 */ { 0, "setgid" },
  /* 182 */ { 0, "setegid" },
  /* 183 */ { 0, "seteuid" },
  /* 184 */ { 0, "lfs_bmapv" },
  /* 185 */ { 0, "lfs_markv" },
  /* 186 */ { 0, "lfs_segclean" },
  /* 187 */ { 0, "lfs_segwait" },
  { 0, }, /* 188 is old stat" */
  { 0, }, /* 189 is old fstat */
  { 0, }, /* 190 is old lstat */
  /* 191 */ { 0, "pathconf" },
  /* 192 */ { 0, "fpathconf" },
  { 0, }, /* 193 */
  /* 194 */ { 0, "getrlimit" },
  /* 195 */ { 0, "setrlimit" },
  { 0, }, /* 196 is old getdirentries */
  /* 197 */ { 0, "mmap" },
  /* 198 */ { do___syscall, "__syscall" },
  /* 199 */ { do_lseek, "lseek" },
  /* 200 */ { 0, "truncate" },
  /* 201 */ { 0, "ftruncate" },
  /* 202 */ { do___sysctl, "__sysctl" },
  /* 203 */ { 0, "mlock" },
  /* 204 */ { 0, "munlock" },
  /* 205 */ { 0, "undelete" },
  { 0, }, /* 206 is old futimes */
  /* 207 */ { 0, "getpgid" },
  /* 208 */ { 0, "reboot" },
  /* 209 */ { 0, "poll" },
  { 0, }, /* 210 */
  { 0, }, /* 211 */
  { 0, }, /* 212 */
  { 0, }, /* 213 */
  { 0, }, /* 214 */
  { 0, }, /* 215 */
  { 0, }, /* 216 */
  { 0, }, /* 217 */
  { 0, }, /* 218 */
  { 0, }, /* 219 */
  { 0, }, /* 220 is old semctl */
  /* 221 */ { 0, "semget" },
  /* 222 */ { 0, "semop" },
  /* 223 */ { 0, "semconfig" },
  { 0, }, /* 224 is old msgctl */
  /* 225 */ { 0, "msgget" },
  /* 226 */ { 0, "msgsnd" },
  /* 227 */ { 0, "msgrcv" },
  /* 228 */ { 0, "shmat" },
  { 0, }, /* 229 is old shmctl */
  /* 230 */ { 0, "shmdt" },
  /* 231 */ { 0, "shmget" },
  { 0, }, /* 232 is old clock_gettime */
  { 0, }, /* 233 is old clock_settime */
  { 0, }, /* 234 is old clock_getres */
  /* 235 */ { 0, "timer_create" },
  /* 236 */ { 0, "timer_delete" },
  { 0, }, /* 237 is old timer_settime */
  { 0, }, /* 238 is old timer_gettime */
  /* 239 */ { 0, "timer_getoverrun" },
  { 0, }, /* 240 is old nanosleep */
  /* 241 */ { 0, "fdatasync" },
  /* 242 */ { 0, "mlockall" },
  /* 243 */ { 0, "munlockall" },
  { 0, }, /* 244 is old sigtimedwait */
  { 0, }, /* 245 */
  /* 246 */ { 0, "modctl" },
  /* 247 */ { 0, "_ksem_init" },
  /* 248 */ { 0, "_ksem_open" },
  /* 249 */ { 0, "_ksem_unlink" },
  /* 250 */ { 0, "_ksem_close" },
  /* 251 */ { 0, "_ksem_post" },
  /* 252 */ { 0, "_ksem_wait" },
  /* 253 */ { 0, "_ksem_trywait" },
  /* 254 */ { 0, "_ksem_getvalue" },
  /* 255 */ { 0, "_ksem_destroy" },
  /* 256 */ { 0, "_ksem_timedwait" },
  /* 257 */ { 0, "mq_open" },
  /* 258 */ { 0, "mq_close" },
  /* 259 */ { 0, "mq_unlink" },
  /* 260 */ { 0, "mq_getattr" },
  /* 261 */ { 0, "mq_setattr" },
  /* 262 */ { 0, "mq_notify" },
  /* 263 */ { 0, "mq_send" },
  /* 264 */ { 0, "mq_receive" },
  { 0, }, /* 265 is old mq_timedsend */
  { 0, }, /* 266 is old mq_timedrecive */
  { 0, }, /* 267 */
  { 0, }, /* 268 */
  { 0, }, /* 269 */
  /* 270 */ { 0, "__posix_rename" },
  /* 271 */ { 0, "swapctl" },
  { 0, }, /* 272 is old getdents */
  /* 273 */ { 0, "minherit" },
  /* 274 */ { 0, "lchmod" },
  /* 275 */ { 0, "lchown" },
  { 0, }, /* 276 is old lutimes */
  /* 277 */ { 0, "__msync13" },
  { 0, }, /* 278 is old stat */
  { 0, }, /* 279 is old fstat */
  { 0, }, /* 280 is old lstat */
  /* 281 */ { 0, "__sigaltstack13" },
  /* 282 */ { 0, "__vfork14" },
  /* 283 */ { 0, "__posix_chown" },
  /* 284 */ { 0, "__posix_fchown" },
  /* 285 */ { 0, "__posix_lchown" },
  /* 286 */ { 0, "getsid" },
  /* 287 */ { 0, "__clone" },
  /* 288 */ { 0, "fktrace" },
  /* 289 */ { 0, "preadv" },
  /* 290 */ { 0, "pwritev" },
  { 0, }, /* 291 is old sigaction */
  /* 292 */ { 0, "__sigpending14" },
  /* 293 */ { do_sigprocmask, "__sigprocmask14" },
  /* 294 */ { 0, "__sigsuspend14" },
  /* 295 */ { 0, "__sigreturn14" },
  /* 296 */ { 0, "__getcwd" },
  /* 297 */ { 0, "fchroot" },
  { 0, }, /* 298 is old fhopen */
  { 0, }, /* 299 is old fhstat */
  { 0, }, /* 300 is old fhstatfs */
  { 0, }, /* 301 is old semctl */
  { 0, }, /* 302 is old msgctl */
  { 0, }, /* 303 is old shmctl */
  /* 304 */ { 0, "lchflags" },
  /* 305 */ { 0, "issetugid" },
  /* 306 */ { 0, "utrace" },
  /* 307 */ { 0, "getcontext" },
  /* 308 */ { 0, "setcontext" },
  /* 309 */ { 0, "_lwp_create" },
  /* 310 */ { 0, "_lwp_exit" },
  /* 311 */ { 0, "_lwp_self" },
  /* 312 */ { 0, "_lwp_wait" },
  /* 313 */ { 0, "_lwp_suspend" },
  /* 314 */ { 0, "_lwp_continue" },
  /* 315 */ { 0, "_lwp_wakeup" },
  /* 316 */ { 0, "_lwp_getprivate" },
  /* 317 */ { 0, "_lwp_setprivate" },
  /* 318 */ { 0, "_lwp_kill" },
  /* 319 */ { 0, "_lwp_detach" },
  { 0, }, /* 320 is old _lwp_park */
  /* 321 */ { 0, "_lwp_unpark" },
  /* 322 */ { 0, "_lwp_unpark_all" },
  /* 323 */ { 0, "_lwp_setname" },
  /* 324 */ { 0, "_lwp_getname" },
  /* 325 */ { 0, "_lwp_ctl" },
  { 0, }, /* 326 */
  { 0, }, /* 327 */
  { 0, }, /* 328 */
  { 0, }, /* 329 */
  /* 330 */ { 0, "sa_register" },
  /* 331 */ { 0, "sa_stacks" },
  /* 332 */ { 0, "sa_enable" },
  /* 333 */ { 0, "sa_setconcurrency" },
  /* 334 */ { 0, "sa_yield" },
  /* 335 */ { 0, "sa_preempt" },
  { 0, }, /* 336 */
  { 0, }, /* 337 */
  { 0, }, /* 338 */
  { 0, }, /* 339 */
  /* 340 */ { 0, "__sigaction_sigtramp" },
  /* 341 */ { 0, "pmc_get_info" },
  /* 342 */ { 0, "pmc_control" },
  /* 343 */ { 0, "rasctl" },
  /* 344 */ { 0, "kqueue" },
  { 0, }, /* 345 is old kevent */
  /* 346 */ { 0, "_sched_setparam" },
  /* 347 */ { 0, "_sched_getparam" },
  /* 348 */ { 0, "_sched_setaffinity" },
  /* 349 */ { 0, "_sched_getaffinity" },
  /* 350 */ { 0, "sched_yield" },
  { 0, }, /* 351 */
  { 0, }, /* 352 */
  { 0, }, /* 353 */
  /* 354 */ { 0, "fsync_range" },
  /* 355 */ { 0, "uuidgen" },
  /* 356 */ { 0, "getvfsstat" },
  /* 357 */ { 0, "statvfs1" },
  /* 358 */ { 0, "fstatvfs1" },
  { 0, }, /* 359 is old fhstatvfs1 */
  /* 360 */ { 0, "extattrctl" },
  /* 361 */ { 0, "extattr_set_file" },
  /* 362 */ { 0, "extattr_get_file" },
  /* 363 */ { 0, "extattr_delete_file" },
  /* 364 */ { 0, "extattr_set_fd" },
  /* 365 */ { 0, "extattr_get_fd" },
  /* 366 */ { 0, "extattr_delete_fd" },
  /* 367 */ { 0, "extattr_set_link" },
  /* 368 */ { 0, "extattr_get_link" },
  /* 369 */ { 0, "extattr_delete_link" },
  /* 370 */ { 0, "extattr_list_fd" },
  /* 371 */ { 0, "extattr_list_file" },
  /* 372 */ { 0, "extattr_list_link" },
  { 0, }, /* 373 is old pselect */
  { 0, }, /* 374 is old pollts */
  /* 375 */ { 0, "setxattr" },
  /* 376 */ { 0, "lsetxattr" },
  /* 377 */ { 0, "fsetxattr" },
  /* 378 */ { 0, "getxattr" },
  /* 379 */ { 0, "lgetxattr" },
  /* 380 */ { 0, "fgetxattr" },
  /* 381 */ { 0, "listxattr" },
  /* 382 */ { 0, "llistxattr" },
  /* 383 */ { 0, "flistxattr" },
  /* 384 */ { 0, "removexattr" },
  /* 385 */ { 0, "lremovexattr" },
  /* 386 */ { 0, "fremovexattr" },
  { 0, }, /* 387 is old stat */
  { 0, }, /* 388 is old fstat */
  { 0, }, /* 389 is old lstat */
  /* 390 */ { do_getdirentries, "__getdents30" },
  { 0, }, /* 391 is old posix_fadvise */
  { 0, }, /* 392 is old fhstat */
  { 0, }, /* 393 is old ntp_gettime */
  /* 394 */ { 0, "__socket30" },
  /* 395 */ { 0, "__getfh30" },
  /* 396 */ { 0, "__fhopen40" },
  /* 397 */ { 0, "__fhstatvfs140" },
  { 0, }, /* 398 is old fhstat */
  /* 399 */ { 0, "aio_cancel" },
  /* 400 */ { 0, "aio_error" },
  /* 401 */ { 0, "aio_fsync" },
  /* 402 */ { 0, "aio_read" },
  /* 403 */ { 0, "aio_return" },
  { 0, }, /* 404 is old aio_suspend */
  /* 405 */ { 0, "aio_write" },
  /* 406 */ { 0, "lio_listio" },
  { 0, }, /* 407 */
  { 0, }, /* 408 */
  { 0, }, /* 409 */
  /* 410 */ { 0, "__mount50" },
  /* 411 */ { 0, "mremap" },
  /* 412 */ { 0, "pset_create" },
  /* 413 */ { 0, "pset_destroy" },
  /* 414 */ { 0, "pset_assign" },
  /* 415 */ { 0, "_pset_bind" },
  /* 416 */ { 0, "__posix_fadvise50" },
  /* 417 */ { 0, "__select50" },
  /* 418 */ { do_gettimeofday, "__gettimeofday50" },
  /* 419 */ { 0, "__settimeofday50" },
  /* 420 */ { 0, "__utimes50" },
  /* 421 */ { 0, "__adjtime50" },
  /* 422 */ { 0, "__lfs_segwait50" },
  /* 423 */ { 0, "__futimes50" },
  /* 424 */ { 0, "__lutimes50" },
  /* 425 */ { 0, "__setitimer50" },
  /* 426 */ { 0, "__getitimer50" },
  /* 427 */ { 0, "__clock_gettime50" },
  /* 428 */ { 0, "__clock_settime50" },
  /* 429 */ { 0, "__clock_getres50" },
  /* 430 */ { 0, "__nanosleep50" },
  /* 431 */ { 0, "____sigtimedwait50" },
  /* 432 */ { 0, "__mq_timedsend50" },
  /* 433 */ { 0, "__mq_timedreceive50" },
  /* 434 */ { 0, "____lwp_park50" },
  /* 435 */ { 0, "__kevent50" },
  /* 436 */ { 0, "__pselect50" },
  /* 437 */ { 0, "__pollts50" },
  /* 438 */ { 0, "__aio_suspend50" },
  /* 439 */ { do_stat, "__stat50" },
  /* 440 */ { do_fstat, "__fstat50" },
  /* 441 */ { do_lstat, "__lstat50" },
  /* 442 */ { 0, "____semctl50" },
  /* 443 */ { 0, "__shmctl50" },
  /* 444 */ { 0, "__msgctl50" },
  /* 445 */ { do_getrusage, "__getrusage50" },
  /* 446 */ { 0, "__timer_settime50" },
  /* 447 */ { 0, "__timer_gettime50" },
  /* 448 */ { 0, "__ntp_gettime50" },
  /* 449 */ { 0, "__wait450" },
  /* 450 */ { 0, "__mknod50" },
  /* 451 */ { 0, "__fhstat50" },
  { 0, }, /* 452 is obsolete 5.99 __quotactl50 */
  /* 453 */ { 0, "pipe2" },
  /* 454 */ { 0, "dup3" },
  /* 455 */ { 0, "kqueue1" },
  /* 456 */ { 0, "paccept" },
  /* 457 */ { 0, "linkat" },
  /* 458 */ { 0, "renameat" },
  /* 459 */ { 0, "mkfifoat" },
  /* 460 */ { 0, "mknodat" },
  /* 461 */ { 0, "mkdirat" },
  /* 462 */ { 0, "faccessat" },
  /* 463 */ { 0, "fchmodat" },
  /* 464 */ { 0, "fchownat" },
  /* 465 */ { 0, "fexecve" },
  /* 466 */ { 0, "fstatat" },
  /* 467 */ { 0, "utimensat" },
  /* 468 */ { 0, "openat" },
  /* 469 */ { 0, "readlinkat" },
  /* 470 */ { 0, "symlinkat" },
  /* 471 */ { 0, "unlinkat" },
  /* 472 */ { 0, "futimens" },
  /* 473 */ { 0, "__quotactl" },
};

static char *(netbsd_error_names[]) = {
  /* 0 */ "ESUCCESS",
  /* 1 */ "EPERM",
  /* 2 */ "ENOENT",
  /* 3 */ "ESRCH",
  /* 4 */ "EINTR",
  /* 5 */ "EIO",
  /* 6 */ "ENXIO",
  /* 7 */ "E2BIG",
  /* 8 */ "ENOEXEC",
  /* 9 */ "EBADF",
  /* 10 */ "ECHILD",
  /* 11 */ "EDEADLK",
  /* 12 */ "ENOMEM",
  /* 13 */ "EACCES",
  /* 14 */ "EFAULT",
  /* 15 */ "ENOTBLK",
  /* 16 */ "EBUSY",
  /* 17 */ "EEXIST",
  /* 18 */ "EXDEV",
  /* 19 */ "ENODEV",
  /* 20 */ "ENOTDIR",
  /* 21 */ "EISDIR",
  /* 22 */ "EINVAL",
  /* 23 */ "ENFILE",
  /* 24 */ "EMFILE",
  /* 25 */ "ENOTTY",
  /* 26 */ "ETXTBSY",
  /* 27 */ "EFBIG",
  /* 28 */ "ENOSPC",
  /* 29 */ "ESPIPE",
  /* 30 */ "EROFS",
  /* 31 */ "EMLINK",
  /* 32 */ "EPIPE",
  /* 33 */ "EDOM",
  /* 34 */ "ERANGE",
  /* 35 */ "EAGAIN",
  /* 36 */ "EINPROGRESS",
  /* 37 */ "EALREADY",
  /* 38 */ "ENOTSOCK",
  /* 39 */ "EDESTADDRREQ",
  /* 40 */ "EMSGSIZE",
  /* 41 */ "EPROTOTYPE",
  /* 42 */ "ENOPROTOOPT",
  /* 43 */ "EPROTONOSUPPORT",
  /* 44 */ "ESOCKTNOSUPPORT",
  /* 45 */ "EOPNOTSUPP",
  /* 46 */ "EPFNOSUPPORT",
  /* 47 */ "EAFNOSUPPORT",
  /* 48 */ "EADDRINUSE",
  /* 49 */ "EADDRNOTAVAIL",
  /* 50 */ "ENETDOWN",
  /* 51 */ "ENETUNREACH",
  /* 52 */ "ENETRESET",
  /* 53 */ "ECONNABORTED",
  /* 54 */ "ECONNRESET",
  /* 55 */ "ENOBUFS",
  /* 56 */ "EISCONN",
  /* 57 */ "ENOTCONN",
  /* 58 */ "ESHUTDOWN",
  /* 59 */ "ETOOMANYREFS",
  /* 60 */ "ETIMEDOUT",
  /* 61 */ "ECONNREFUSED",
  /* 62 */ "ELOOP",
  /* 63 */ "ENAMETOOLONG",
  /* 64 */ "EHOSTDOWN",
  /* 65 */ "EHOSTUNREACH",
  /* 66 */ "ENOTEMPTY",
  /* 67 */ "EPROCLIM",
  /* 68 */ "EUSERS",
  /* 69 */ "EDQUOT",
  /* 70 */ "ESTALE",
  /* 71 */ "EREMOTE",
  /* 72 */ "EBADRPC",
  /* 73 */ "ERPCMISMATCH",
  /* 74 */ "EPROGUNAVAIL",
  /* 75 */ "EPROGMISMATCH",
  /* 76 */ "EPROCUNAVAIL",
  /* 77 */ "ENOLCK",
  /* 78 */ "ENOSYS",
  /* 79 */ "EFTYPE",
  /* 80 */ "EAUTH",
  /* 81 */ "ENEEDAUTH",
  /* 82 */ "EIDRM",
  /* 83 */ "ENOMSG",
  /* 84 */ "EOVERFLOW",
  /* 85 */ "EILSEQ",
  /* 86 */ "ENOTSUP",
  /* 87 */ "ECANCELED",
  /* 88 */ "EBADMSG",
  /* 89 */ "ENODATA",
  /* 90 */ "ENOSR",
  /* 91 */ "ENOSTR",
  /* 92 */ "ETIME",
  /* 93 */ "ENOATTR",
  /* 94 */ "EMULTIHOP",
  /* 95 */ "ENOLINK",
  /* 96 */ "EPROTO",
  /* 96 */ "ELAST",
};

static char *(netbsd_signal_names[]) = {
  /* 0 */ 0,
  /* 1 */ "SIGHUP",
  /* 2 */ "SIGINT",
  /* 3 */ "SIGQUIT",
  /* 4 */ "SIGILL",
  /* 5 */ "SIGTRAP",
  /* 6 */ "SIGABRT",
  /* 7 */ "SIGEMT",
  /* 8 */ "SIGFPE",
  /* 9 */ "SIGKILL",
  /* 10 */ "SIGBUS",
  /* 11 */ "SIGSEGV",
  /* 12 */ "SIGSYS",
  /* 13 */ "SIGPIPE",
  /* 14 */ "SIGALRM",
  /* 15 */ "SIGTERM",
  /* 16 */ "SIGURG",
  /* 17 */ "SIGSTOP",
  /* 18 */ "SIGTSTP",
  /* 19 */ "SIGCONT",
  /* 20 */ "SIGCHLD",
  /* 21 */ "SIGTTIN",
  /* 22 */ "SIGTTOU",
  /* 23 */ "SIGIO",
  /* 24 */ "SIGXCPU",
  /* 25 */ "SIGXFSZ",
  /* 26 */ "SIGVTALRM",
  /* 27 */ "SIGPROF",
  /* 28 */ "SIGWINCH",
  /* 29 */ "SIGINFO",
  /* 30 */ "SIGUSR1",
  /* 31 */ "SIGUSR2",
  /* 32 */ "SIGPWR",
};

static emul_syscall emul_netbsd_syscalls = {
  netbsd_descriptors,
  ARRAY_SIZE (netbsd_descriptors),
  netbsd_error_names,
  ARRAY_SIZE (netbsd_error_names),
  netbsd_signal_names,
  ARRAY_SIZE (netbsd_signal_names),
};


/* NetBSD's os_emul interface, most are just passed on to the generic
   syscall stuff */

static os_emul_data *
emul_netbsd_create(device *root,
		   bfd *image,
		   const char *name)
{
  unsigned_word top_of_stack;
  unsigned stack_size;
  int elf_binary;
  os_emul_data *bsd_data;
  device *vm;
  char *filename;

  /* check that this emulation is really for us */
  if (name != NULL && strcmp(name, "netbsd") != 0)
    return NULL;
  if (image == NULL)
    return NULL;


  /* merge any emulation specific entries into the device tree */

  /* establish a few defaults */
  if (image->xvec->flavour == bfd_target_elf_flavour) {
    elf_binary = 1;
    top_of_stack = 0xe0000000;
    stack_size =   0x00100000;
  }
  else {
    elf_binary = 0;
    top_of_stack = 0x20000000;
    stack_size =   0x00100000;
  }

  /* options */
  emul_add_tree_options(root, image, "netbsd",
			(WITH_ENVIRONMENT == USER_ENVIRONMENT
			 ? "user" : "virtual"),
			0 /*oea-interrupt-prefix*/);

  /* virtual memory - handles growth of stack/heap */
  vm = tree_parse(root, "/openprom/vm");
  tree_parse(vm, "./stack-base 0x%lx",
	     (unsigned long)(top_of_stack - stack_size));
  tree_parse(vm, "./nr-bytes 0x%x", stack_size);

  filename = tree_quote_property (bfd_get_filename(image));
  tree_parse(root, "/openprom/vm/map-binary/file-name %s",
	     filename);
  free (filename);

  /* finish the init */
  tree_parse(root, "/openprom/init/register/pc 0x%lx",
	     (unsigned long)bfd_get_start_address(image));
  tree_parse(root, "/openprom/init/register/sp 0x%lx",
	     (unsigned long)top_of_stack);
  tree_parse(root, "/openprom/init/register/msr 0x%x",
	     ((tree_find_boolean_property(root, "/options/little-endian?")
	       ? msr_little_endian_mode
	       : 0)
	      | (tree_find_boolean_property(root, "/openprom/options/floating-point?")
		 ? (msr_floating_point_available
		    | msr_floating_point_exception_mode_0
		    | msr_floating_point_exception_mode_1)
		 : 0)));
  tree_parse(root, "/openprom/init/stack/stack-type %s",
	     (elf_binary ? "ppc-elf" : "ppc-xcoff"));

  /* finally our emulation data */
  bsd_data = ZALLOC(os_emul_data);
  bsd_data->vm = vm;
  bsd_data->syscalls = &emul_netbsd_syscalls;
  return bsd_data;
}

static void
emul_netbsd_init(os_emul_data *emul_data,
		 int nr_cpus)
{
  fd_closed[0] = 0;
  fd_closed[1] = 0;
  fd_closed[2] = 0;
}

static void
emul_netbsd_system_call(cpu *processor,
			unsigned_word cia,
			os_emul_data *emul_data)
{
  emul_do_system_call(emul_data,
		      emul_data->syscalls,
		      cpu_registers(processor)->gpr[0],
		      3, /*r3 contains arg0*/
		      processor,
		      cia);
}

const os_emul emul_netbsd = {
  "netbsd",
  emul_netbsd_create,
  emul_netbsd_init,
  emul_netbsd_system_call,
  0, /*instruction_call*/
  0 /*data*/
};

#endif	/* _EMUL_NETBSD_C_ */
