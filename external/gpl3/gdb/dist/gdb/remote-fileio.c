/* Remote File-I/O communications

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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

/* See the GDB User Guide for details of the GDB remote protocol.  */

#include "defs.h"
#include <string.h>
#include "gdbcmd.h"
#include "remote.h"
#include "gdb/fileio.h"
#include "gdb_wait.h"
#include <sys/stat.h>
#include "exceptions.h"
#include "remote-fileio.h"
#include "event-loop.h"
#include "target.h"
#include "filenames.h"
#include "filestuff.h"

#include <fcntl.h>
#include <sys/time.h>
#ifdef __CYGWIN__
#include <sys/cygwin.h>		/* For cygwin_conv_path.  */
#endif
#include <signal.h>

static struct {
  int *fd_map;
  int fd_map_size;
} remote_fio_data;

#define FIO_FD_INVALID		-1
#define FIO_FD_CONSOLE_IN	-2
#define FIO_FD_CONSOLE_OUT	-3

static int remote_fio_system_call_allowed = 0;

static struct async_signal_handler *sigint_fileio_token;

static int
remote_fileio_init_fd_map (void)
{
  int i;

  if (!remote_fio_data.fd_map)
    {
      remote_fio_data.fd_map = (int *) xmalloc (10 * sizeof (int));
      remote_fio_data.fd_map_size = 10;
      remote_fio_data.fd_map[0] = FIO_FD_CONSOLE_IN;
      remote_fio_data.fd_map[1] = FIO_FD_CONSOLE_OUT;
      remote_fio_data.fd_map[2] = FIO_FD_CONSOLE_OUT;
      for (i = 3; i < 10; ++i)
        remote_fio_data.fd_map[i] = FIO_FD_INVALID;
    }
  return 3;
}

static int
remote_fileio_resize_fd_map (void)
{
  int i = remote_fio_data.fd_map_size;

  if (!remote_fio_data.fd_map)
    return remote_fileio_init_fd_map ();
  remote_fio_data.fd_map_size += 10;
  remote_fio_data.fd_map =
    (int *) xrealloc (remote_fio_data.fd_map,
		      remote_fio_data.fd_map_size * sizeof (int));
  for (; i < remote_fio_data.fd_map_size; i++)
    remote_fio_data.fd_map[i] = FIO_FD_INVALID;
  return remote_fio_data.fd_map_size - 10;
}

static int
remote_fileio_next_free_fd (void)
{
  int i;

  for (i = 0; i < remote_fio_data.fd_map_size; ++i)
    if (remote_fio_data.fd_map[i] == FIO_FD_INVALID)
      return i;
  return remote_fileio_resize_fd_map ();
}

static int
remote_fileio_fd_to_targetfd (int fd)
{
  int target_fd = remote_fileio_next_free_fd ();

  remote_fio_data.fd_map[target_fd] = fd;
  return target_fd;
}

static int
remote_fileio_map_fd (int target_fd)
{
  remote_fileio_init_fd_map ();
  if (target_fd < 0 || target_fd >= remote_fio_data.fd_map_size)
    return FIO_FD_INVALID;
  return remote_fio_data.fd_map[target_fd];
}

static void
remote_fileio_close_target_fd (int target_fd)
{
  remote_fileio_init_fd_map ();
  if (target_fd >= 0 && target_fd < remote_fio_data.fd_map_size)
    remote_fio_data.fd_map[target_fd] = FIO_FD_INVALID;
}

static int
remote_fileio_oflags_to_host (long flags)
{
  int hflags = 0;

  if (flags & FILEIO_O_CREAT)
    hflags |= O_CREAT;
  if (flags & FILEIO_O_EXCL)
    hflags |= O_EXCL;
  if (flags & FILEIO_O_TRUNC)
    hflags |= O_TRUNC;
  if (flags & FILEIO_O_APPEND)
    hflags |= O_APPEND;
  if (flags & FILEIO_O_RDONLY)
    hflags |= O_RDONLY;
  if (flags & FILEIO_O_WRONLY)
    hflags |= O_WRONLY;
  if (flags & FILEIO_O_RDWR)
    hflags |= O_RDWR;
/* On systems supporting binary and text mode, always open files in
   binary mode.  */
#ifdef O_BINARY
  hflags |= O_BINARY;
#endif
  return hflags;
}

static mode_t
remote_fileio_mode_to_host (long mode, int open_call)
{
  mode_t hmode = 0;

  if (!open_call)
    {
      if (mode & FILEIO_S_IFREG)
	hmode |= S_IFREG;
      if (mode & FILEIO_S_IFDIR)
	hmode |= S_IFDIR;
      if (mode & FILEIO_S_IFCHR)
	hmode |= S_IFCHR;
    }
  if (mode & FILEIO_S_IRUSR)
    hmode |= S_IRUSR;
  if (mode & FILEIO_S_IWUSR)
    hmode |= S_IWUSR;
  if (mode & FILEIO_S_IXUSR)
    hmode |= S_IXUSR;
#ifdef S_IRGRP
  if (mode & FILEIO_S_IRGRP)
    hmode |= S_IRGRP;
#endif
#ifdef S_IWGRP
  if (mode & FILEIO_S_IWGRP)
    hmode |= S_IWGRP;
#endif
#ifdef S_IXGRP
  if (mode & FILEIO_S_IXGRP)
    hmode |= S_IXGRP;
#endif
  if (mode & FILEIO_S_IROTH)
    hmode |= S_IROTH;
#ifdef S_IWOTH
  if (mode & FILEIO_S_IWOTH)
    hmode |= S_IWOTH;
#endif
#ifdef S_IXOTH
  if (mode & FILEIO_S_IXOTH)
    hmode |= S_IXOTH;
#endif
  return hmode;
}

static LONGEST
remote_fileio_mode_to_target (mode_t mode)
{
  mode_t tmode = 0;

  if (S_ISREG(mode))
    tmode |= FILEIO_S_IFREG;
  if (S_ISDIR(mode))
    tmode |= FILEIO_S_IFDIR;
  if (S_ISCHR(mode))
    tmode |= FILEIO_S_IFCHR;
  if (mode & S_IRUSR)
    tmode |= FILEIO_S_IRUSR;
  if (mode & S_IWUSR)
    tmode |= FILEIO_S_IWUSR;
  if (mode & S_IXUSR)
    tmode |= FILEIO_S_IXUSR;
#ifdef S_IRGRP
  if (mode & S_IRGRP)
    tmode |= FILEIO_S_IRGRP;
#endif
#ifdef S_IWRGRP
  if (mode & S_IWGRP)
    tmode |= FILEIO_S_IWGRP;
#endif
#ifdef S_IXGRP
  if (mode & S_IXGRP)
    tmode |= FILEIO_S_IXGRP;
#endif
  if (mode & S_IROTH)
    tmode |= FILEIO_S_IROTH;
#ifdef S_IWOTH
  if (mode & S_IWOTH)
    tmode |= FILEIO_S_IWOTH;
#endif
#ifdef S_IXOTH
  if (mode & S_IXOTH)
    tmode |= FILEIO_S_IXOTH;
#endif
  return tmode;
}

static int
remote_fileio_errno_to_target (int error)
{
  switch (error)
    {
      case EPERM:
        return FILEIO_EPERM;
      case ENOENT:
        return FILEIO_ENOENT;
      case EINTR:
        return FILEIO_EINTR;
      case EIO:
        return FILEIO_EIO;
      case EBADF:
        return FILEIO_EBADF;
      case EACCES:
        return FILEIO_EACCES;
      case EFAULT:
        return FILEIO_EFAULT;
      case EBUSY:
        return FILEIO_EBUSY;
      case EEXIST:
        return FILEIO_EEXIST;
      case ENODEV:
        return FILEIO_ENODEV;
      case ENOTDIR:
        return FILEIO_ENOTDIR;
      case EISDIR:
        return FILEIO_EISDIR;
      case EINVAL:
        return FILEIO_EINVAL;
      case ENFILE:
        return FILEIO_ENFILE;
      case EMFILE:
        return FILEIO_EMFILE;
      case EFBIG:
        return FILEIO_EFBIG;
      case ENOSPC:
        return FILEIO_ENOSPC;
      case ESPIPE:
        return FILEIO_ESPIPE;
      case EROFS:
        return FILEIO_EROFS;
      case ENOSYS:
        return FILEIO_ENOSYS;
      case ENAMETOOLONG:
        return FILEIO_ENAMETOOLONG;
    }
  return FILEIO_EUNKNOWN;
}

static int
remote_fileio_seek_flag_to_host (long num, int *flag)
{
  if (!flag)
    return 0;
  switch (num)
    {
      case FILEIO_SEEK_SET:
        *flag = SEEK_SET;
	break;
      case FILEIO_SEEK_CUR:
        *flag =  SEEK_CUR;
	break;
      case FILEIO_SEEK_END:
        *flag =  SEEK_END;
	break;
      default:
        return -1;
    }
  return 0;
}

static int
remote_fileio_extract_long (char **buf, LONGEST *retlong)
{
  char *c;
  int sign = 1;

  if (!buf || !*buf || !**buf || !retlong)
    return -1;
  c = strchr (*buf, ',');
  if (c)
    *c++ = '\0';
  else
    c = strchr (*buf, '\0');
  while (strchr ("+-", **buf))
    {
      if (**buf == '-')
	sign = -sign;
      ++*buf;
    }
  for (*retlong = 0; **buf; ++*buf)
    {
      *retlong <<= 4;
      if (**buf >= '0' && **buf <= '9')
        *retlong += **buf - '0';
      else if (**buf >= 'a' && **buf <= 'f')
        *retlong += **buf - 'a' + 10;
      else if (**buf >= 'A' && **buf <= 'F')
        *retlong += **buf - 'A' + 10;
      else
        return -1;
    }
  *retlong *= sign;
  *buf = c;
  return 0;
}

static int
remote_fileio_extract_int (char **buf, long *retint)
{
  int ret;
  LONGEST retlong;

  if (!retint)
    return -1;
  ret = remote_fileio_extract_long (buf, &retlong);
  if (!ret)
    *retint = (long) retlong;
  return ret;
}

static int
remote_fileio_extract_ptr_w_len (char **buf, CORE_ADDR *ptrval, int *length)
{
  char *c;
  LONGEST retlong;

  if (!buf || !*buf || !**buf || !ptrval || !length)
    return -1;
  c = strchr (*buf, '/');
  if (!c)
    return -1;
  *c++ = '\0';
  if (remote_fileio_extract_long (buf, &retlong))
    return -1;
  *ptrval = (CORE_ADDR) retlong;
  *buf = c;
  if (remote_fileio_extract_long (buf, &retlong))
    return -1;
  *length = (int) retlong;
  return 0;
}

/* Convert to big endian.  */
static void
remote_fileio_to_be (LONGEST num, char *buf, int bytes)
{
  int i;

  for (i = 0; i < bytes; ++i)
    buf[i] = (num >> (8 * (bytes - i - 1))) & 0xff;
}

static void
remote_fileio_to_fio_uint (long num, fio_uint_t fnum)
{
  remote_fileio_to_be ((LONGEST) num, (char *) fnum, 4);
}

static void
remote_fileio_to_fio_mode (mode_t num, fio_mode_t fnum)
{
  remote_fileio_to_be (remote_fileio_mode_to_target(num), (char *) fnum, 4);
}

static void
remote_fileio_to_fio_time (time_t num, fio_time_t fnum)
{
  remote_fileio_to_be ((LONGEST) num, (char *) fnum, 4);
}

static void
remote_fileio_to_fio_long (LONGEST num, fio_long_t fnum)
{
  remote_fileio_to_be (num, (char *) fnum, 8);
}

static void
remote_fileio_to_fio_ulong (LONGEST num, fio_ulong_t fnum)
{
  remote_fileio_to_be (num, (char *) fnum, 8);
}

static void
remote_fileio_to_fio_stat (struct stat *st, struct fio_stat *fst)
{
  LONGEST blksize;

  /* `st_dev' is set in the calling function.  */
  remote_fileio_to_fio_uint ((long) st->st_ino, fst->fst_ino);
  remote_fileio_to_fio_mode (st->st_mode, fst->fst_mode);
  remote_fileio_to_fio_uint ((long) st->st_nlink, fst->fst_nlink);
  remote_fileio_to_fio_uint ((long) st->st_uid, fst->fst_uid);
  remote_fileio_to_fio_uint ((long) st->st_gid, fst->fst_gid);
  remote_fileio_to_fio_uint ((long) st->st_rdev, fst->fst_rdev);
  remote_fileio_to_fio_ulong ((LONGEST) st->st_size, fst->fst_size);
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
  blksize = st->st_blksize;
#else
  blksize = 512;
#endif
  remote_fileio_to_fio_ulong (blksize, fst->fst_blksize);
#if HAVE_STRUCT_STAT_ST_BLOCKS
  remote_fileio_to_fio_ulong ((LONGEST) st->st_blocks, fst->fst_blocks);
#else
  /* FIXME: This is correct for DJGPP, but other systems that don't
     have st_blocks, if any, might prefer 512 instead of st_blksize.
     (eliz, 30-12-2003)  */
  remote_fileio_to_fio_ulong (((LONGEST) st->st_size + blksize - 1)
			      / blksize,
			      fst->fst_blocks);
#endif
  remote_fileio_to_fio_time (st->st_atime, fst->fst_atime);
  remote_fileio_to_fio_time (st->st_mtime, fst->fst_mtime);
  remote_fileio_to_fio_time (st->st_ctime, fst->fst_ctime);
}

static void
remote_fileio_to_fio_timeval (struct timeval *tv, struct fio_timeval *ftv)
{
  remote_fileio_to_fio_time (tv->tv_sec, ftv->ftv_sec);
  remote_fileio_to_fio_long (tv->tv_usec, ftv->ftv_usec);
}

static int remote_fio_ctrl_c_flag = 0;
static int remote_fio_no_longjmp = 0;

#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
static struct sigaction remote_fio_sa;
static struct sigaction remote_fio_osa;
#else
static void (*remote_fio_ofunc)(int);
#endif

static void
remote_fileio_sig_init (void)
{
#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
  remote_fio_sa.sa_handler = SIG_IGN;
  sigemptyset (&remote_fio_sa.sa_mask);
  remote_fio_sa.sa_flags = 0;
  sigaction (SIGINT, &remote_fio_sa, &remote_fio_osa);
#else
  remote_fio_ofunc = signal (SIGINT, SIG_IGN);
#endif
}

static void
remote_fileio_sig_set (void (*sigint_func)(int))
{
#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
  remote_fio_sa.sa_handler = sigint_func;
  sigemptyset (&remote_fio_sa.sa_mask);
  remote_fio_sa.sa_flags = 0;
  sigaction (SIGINT, &remote_fio_sa, NULL);
#else
  signal (SIGINT, sigint_func);
#endif
}

static void
remote_fileio_sig_exit (void)
{
#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
  sigaction (SIGINT, &remote_fio_osa, NULL);
#else
  signal (SIGINT, remote_fio_ofunc);
#endif
}

static void
async_remote_fileio_interrupt (gdb_client_data arg)
{
  quit ();
}

static void
remote_fileio_ctrl_c_signal_handler (int signo)
{
  remote_fileio_sig_set (SIG_IGN);
  remote_fio_ctrl_c_flag = 1;
  if (!remote_fio_no_longjmp)
    gdb_call_async_signal_handler (sigint_fileio_token, 1);
  remote_fileio_sig_set (remote_fileio_ctrl_c_signal_handler);
}

static void
remote_fileio_reply (int retcode, int error)
{
  char buf[32];

  remote_fileio_sig_set (SIG_IGN);
  strcpy (buf, "F");
  if (retcode < 0)
    {
      strcat (buf, "-");
      retcode = -retcode;
    }
  sprintf (buf + strlen (buf), "%x", retcode);
  if (error || remote_fio_ctrl_c_flag)
    {
      if (error && remote_fio_ctrl_c_flag)
        error = FILEIO_EINTR;
      if (error < 0)
        {
	  strcat (buf, "-");
	  error = -error;
	}
      sprintf (buf + strlen (buf), ",%x", error);
      if (remote_fio_ctrl_c_flag)
        strcat (buf, ",C");
    }
  remote_fileio_sig_set (remote_fileio_ctrl_c_signal_handler);
  putpkt (buf);
}

static void
remote_fileio_ioerror (void)
{
  remote_fileio_reply (-1, FILEIO_EIO);
}

static void
remote_fileio_badfd (void)
{
  remote_fileio_reply (-1, FILEIO_EBADF);
}

static void
remote_fileio_return_errno (int retcode)
{
  remote_fileio_reply (retcode, retcode < 0
		       ? remote_fileio_errno_to_target (errno) : 0);
}

static void
remote_fileio_return_success (int retcode)
{
  remote_fileio_reply (retcode, 0);
}

static void
remote_fileio_func_open (char *buf)
{
  CORE_ADDR ptrval;
  int length;
  long num;
  int flags, fd;
  mode_t mode;
  char *pathname;
  struct stat st;

  /* 1. Parameter: Ptr to pathname / length incl. trailing zero.  */
  if (remote_fileio_extract_ptr_w_len (&buf, &ptrval, &length))
    {
      remote_fileio_ioerror ();
      return;
    }
  /* 2. Parameter: open flags */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror ();
      return;
    }
  flags = remote_fileio_oflags_to_host (num);
  /* 3. Parameter: open mode */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror ();
      return;
    }
  mode = remote_fileio_mode_to_host (num, 1);

  /* Request pathname.  */
  pathname = alloca (length);
  if (target_read_memory (ptrval, (gdb_byte *) pathname, length) != 0)
    {
      remote_fileio_ioerror ();
      return;
    }

  /* Check if pathname exists and is not a regular file or directory.  If so,
     return an appropriate error code.  Same for trying to open directories
     for writing.  */
  if (!stat (pathname, &st))
    {
      if (!S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode))
	{
	  remote_fileio_reply (-1, FILEIO_ENODEV);
	  return;
	}
      if (S_ISDIR (st.st_mode)
	  && ((flags & O_WRONLY) == O_WRONLY || (flags & O_RDWR) == O_RDWR))
	{
	  remote_fileio_reply (-1, FILEIO_EISDIR);
	  return;
	}
    }

  remote_fio_no_longjmp = 1;
  fd = gdb_open_cloexec (pathname, flags, mode);
  if (fd < 0)
    {
      remote_fileio_return_errno (-1);
      return;
    }

  fd = remote_fileio_fd_to_targetfd (fd);
  remote_fileio_return_success (fd);
}

static void
remote_fileio_func_close (char *buf)
{
  long num;
  int fd;

  /* Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror ();
      return;
    }
  fd = remote_fileio_map_fd ((int) num);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd ();
      return;
    }

  remote_fio_no_longjmp = 1;
  if (fd != FIO_FD_CONSOLE_IN && fd != FIO_FD_CONSOLE_OUT && close (fd))
    remote_fileio_return_errno (-1);
  remote_fileio_close_target_fd ((int) num);
  remote_fileio_return_success (0);
}

static void
remote_fileio_func_read (char *buf)
{
  long target_fd, num;
  LONGEST lnum;
  CORE_ADDR ptrval;
  int fd, ret;
  gdb_byte *buffer;
  size_t length;
  off_t old_offset, new_offset;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror ();
      return;
    }
  fd = remote_fileio_map_fd ((int) target_fd);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd ();
      return;
    }
  /* 2. Parameter: buffer pointer */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror ();
      return;
    }
  ptrval = (CORE_ADDR) lnum;
  /* 3. Parameter: buffer length */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror ();
      return;
    }
  length = (size_t) num;

  switch (fd)
    {
      case FIO_FD_CONSOLE_OUT:
	remote_fileio_badfd ();
	return;
      case FIO_FD_CONSOLE_IN:
	{
	  static char *remaining_buf = NULL;
	  static int remaining_length = 0;

	  buffer = (gdb_byte *) xmalloc (16384);
	  if (remaining_buf)
	    {
	      remote_fio_no_longjmp = 1;
	      if (remaining_length > length)
		{
		  memcpy (buffer, remaining_buf, length);
		  memmove (remaining_buf, remaining_buf + length,
			   remaining_length - length);
		  remaining_length -= length;
		  ret = length;
		}
	      else
		{
		  memcpy (buffer, remaining_buf, remaining_length);
		  xfree (remaining_buf);
		  remaining_buf = NULL;
		  ret = remaining_length;
		}
	    }
	  else
	    {
	      /* Windows (at least XP and Server 2003) has difficulty
		 with large reads from consoles.  If a handle is
		 backed by a real console device, overly large reads
		 from the handle will fail and set errno == ENOMEM.
		 On a Windows Server 2003 system where I tested,
		 reading 26608 bytes from the console was OK, but
		 anything above 26609 bytes would fail.  The limit has
		 been observed to vary on different systems.  So, we
		 limit this read to something smaller than that - by a
		 safe margin, in case the limit depends on system
		 resources or version.  */
	      ret = ui_file_read (gdb_stdtargin, (char *) buffer, 16383);
	      remote_fio_no_longjmp = 1;
	      if (ret > 0 && (size_t)ret > length)
		{
		  remaining_buf = (char *) xmalloc (ret - length);
		  remaining_length = ret - length;
		  memcpy (remaining_buf, buffer + length, remaining_length);
		  ret = length;
		}
	    }
	}
	break;
      default:
	buffer = (gdb_byte *) xmalloc (length);
	/* POSIX defines EINTR behaviour of read in a weird way.  It's allowed
	   for read() to return -1 even if "some" bytes have been read.  It
	   has been corrected in SUSv2 but that doesn't help us much...
	   Therefore a complete solution must check how many bytes have been
	   read on EINTR to return a more reliable value to the target */
	old_offset = lseek (fd, 0, SEEK_CUR);
	remote_fio_no_longjmp = 1;
	ret = read (fd, buffer, length);
	if (ret < 0 && errno == EINTR)
	  {
	    new_offset = lseek (fd, 0, SEEK_CUR);
	    /* If some data has been read, return the number of bytes read.
	       The Ctrl-C flag is set in remote_fileio_reply() anyway.  */
	    if (old_offset != new_offset)
	      ret = new_offset - old_offset;
	  }
	break;
    }

  if (ret > 0)
    {
      errno = target_write_memory (ptrval, buffer, ret);
      if (errno != 0)
	ret = -1;
    }

  if (ret < 0)
    remote_fileio_return_errno (-1);
  else
    remote_fileio_return_success (ret);

  xfree (buffer);
}

static void
remote_fileio_func_write (char *buf)
{
  long target_fd, num;
  LONGEST lnum;
  CORE_ADDR ptrval;
  int fd, ret;
  gdb_byte *buffer;
  size_t length;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror ();
      return;
    }
  fd = remote_fileio_map_fd ((int) target_fd);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd ();
      return;
    }
  /* 2. Parameter: buffer pointer */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror ();
      return;
    }
  ptrval = (CORE_ADDR) lnum;
  /* 3. Parameter: buffer length */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror ();
      return;
    }
  length = (size_t) num;
    
  buffer = (gdb_byte *) xmalloc (length);
  if (target_read_memory (ptrval, buffer, length) != 0)
    {
      xfree (buffer);
      remote_fileio_ioerror ();
      return;
    }

  remote_fio_no_longjmp = 1;
  switch (fd)
    {
      case FIO_FD_CONSOLE_IN:
	remote_fileio_badfd ();
	xfree (buffer);
	return;
      case FIO_FD_CONSOLE_OUT:
	ui_file_write (target_fd == 1 ? gdb_stdtarg : gdb_stdtargerr,
		       (char *) buffer, length);
	gdb_flush (target_fd == 1 ? gdb_stdtarg : gdb_stdtargerr);
	ret = length;
	break;
      default:
	ret = write (fd, buffer, length);
	if (ret < 0 && errno == EACCES)
	  errno = EBADF; /* Cygwin returns EACCESS when writing to a
			    R/O file.  */
	break;
    }

  if (ret < 0)
    remote_fileio_return_errno (-1);
  else
    remote_fileio_return_success (ret);

  xfree (buffer);
}

static void
remote_fileio_func_lseek (char *buf)
{
  long num;
  LONGEST lnum;
  int fd, flag;
  off_t offset, ret;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror ();
      return;
    }
  fd = remote_fileio_map_fd ((int) num);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd ();
      return;
    }
  else if (fd == FIO_FD_CONSOLE_IN || fd == FIO_FD_CONSOLE_OUT)
    {
      remote_fileio_reply (-1, FILEIO_ESPIPE);
      return;
    }

  /* 2. Parameter: offset */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror ();
      return;
    }
  offset = (off_t) lnum;
  /* 3. Parameter: flag */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror ();
      return;
    }
  if (remote_fileio_seek_flag_to_host (num, &flag))
    {
      remote_fileio_reply (-1, FILEIO_EINVAL);
      return;
    }
  
  remote_fio_no_longjmp = 1;
  ret = lseek (fd, offset, flag);

  if (ret == (off_t) -1)
    remote_fileio_return_errno (-1);
  else
    remote_fileio_return_success (ret);
}

static void
remote_fileio_func_rename (char *buf)
{
  CORE_ADDR old_ptr, new_ptr;
  int old_len, new_len;
  char *oldpath, *newpath;
  int ret, of, nf;
  struct stat ost, nst;

  /* 1. Parameter: Ptr to oldpath / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &old_ptr, &old_len))
    {
      remote_fileio_ioerror ();
      return;
    }
  
  /* 2. Parameter: Ptr to newpath / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &new_ptr, &new_len))
    {
      remote_fileio_ioerror ();
      return;
    }
  
  /* Request oldpath using 'm' packet */
  oldpath = alloca (old_len);
  if (target_read_memory (old_ptr, (gdb_byte *) oldpath, old_len) != 0)
    {
      remote_fileio_ioerror ();
      return;
    }
  
  /* Request newpath using 'm' packet */
  newpath = alloca (new_len);
  if (target_read_memory (new_ptr, (gdb_byte *) newpath, new_len) != 0)
    {
      remote_fileio_ioerror ();
      return;
    }
  
  /* Only operate on regular files and directories.  */
  of = stat (oldpath, &ost);
  nf = stat (newpath, &nst);
  if ((!of && !S_ISREG (ost.st_mode) && !S_ISDIR (ost.st_mode))
      || (!nf && !S_ISREG (nst.st_mode) && !S_ISDIR (nst.st_mode)))
    {
      remote_fileio_reply (-1, FILEIO_EACCES);
      return;
    }

  remote_fio_no_longjmp = 1;
  ret = rename (oldpath, newpath);

  if (ret == -1)
    {
      /* Special case: newpath is a non-empty directory.  Some systems
         return ENOTEMPTY, some return EEXIST.  We coerce that to be
	 always EEXIST.  */
      if (errno == ENOTEMPTY)
        errno = EEXIST;
#ifdef __CYGWIN__
      /* Workaround some Cygwin problems with correct errnos.  */
      if (errno == EACCES)
        {
	  if (!of && !nf && S_ISDIR (nst.st_mode))
	    {
	      if (S_ISREG (ost.st_mode))
		errno = EISDIR;
	      else
		{
		  char oldfullpath[PATH_MAX];
		  char newfullpath[PATH_MAX];
		  int len;

		  cygwin_conv_path (CCP_WIN_A_TO_POSIX, oldpath, oldfullpath,
				    PATH_MAX);
		  cygwin_conv_path (CCP_WIN_A_TO_POSIX, newpath, newfullpath,
				    PATH_MAX);
		  len = strlen (oldfullpath);
		  if (IS_DIR_SEPARATOR (newfullpath[len])
		      && !filename_ncmp (oldfullpath, newfullpath, len))
		    errno = EINVAL;
		  else
		    errno = EEXIST;
		}
	    }
	}
#endif

      remote_fileio_return_errno (-1);
    }
  else
    remote_fileio_return_success (ret);
}

static void
remote_fileio_func_unlink (char *buf)
{
  CORE_ADDR ptrval;
  int length;
  char *pathname;
  int ret;
  struct stat st;

  /* Parameter: Ptr to pathname / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &ptrval, &length))
    {
      remote_fileio_ioerror ();
      return;
    }
  /* Request pathname using 'm' packet */
  pathname = alloca (length);
  if (target_read_memory (ptrval, (gdb_byte *) pathname, length) != 0)
    {
      remote_fileio_ioerror ();
      return;
    }

  /* Only operate on regular files (and directories, which allows to return
     the correct return code).  */
  if (!stat (pathname, &st) && !S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode))
    {
      remote_fileio_reply (-1, FILEIO_ENODEV);
      return;
    }

  remote_fio_no_longjmp = 1;
  ret = unlink (pathname);

  if (ret == -1)
    remote_fileio_return_errno (-1);
  else
    remote_fileio_return_success (ret);
}

static void
remote_fileio_func_stat (char *buf)
{
  CORE_ADDR statptr, nameptr;
  int ret, namelength;
  char *pathname;
  LONGEST lnum;
  struct stat st;
  struct fio_stat fst;

  /* 1. Parameter: Ptr to pathname / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &nameptr, &namelength))
    {
      remote_fileio_ioerror ();
      return;
    }

  /* 2. Parameter: Ptr to struct stat */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror ();
      return;
    }
  statptr = (CORE_ADDR) lnum;
  
  /* Request pathname using 'm' packet */
  pathname = alloca (namelength);
  if (target_read_memory (nameptr, (gdb_byte *) pathname, namelength) != 0)
    {
      remote_fileio_ioerror ();
      return;
    }

  remote_fio_no_longjmp = 1;
  ret = stat (pathname, &st);

  if (ret == -1)
    {
      remote_fileio_return_errno (-1);
      return;
    }
  /* Only operate on regular files and directories.  */
  if (!ret && !S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode))
    {
      remote_fileio_reply (-1, FILEIO_EACCES);
      return;
    }
  if (statptr)
    {
      remote_fileio_to_fio_stat (&st, &fst);
      remote_fileio_to_fio_uint (0, fst.fst_dev);

      errno = target_write_memory (statptr, (gdb_byte *) &fst, sizeof fst);
      if (errno != 0)
	{
	  remote_fileio_return_errno (-1);
	  return;
	}
    }
  remote_fileio_return_success (ret);
}

static void
remote_fileio_func_fstat (char *buf)
{
  CORE_ADDR ptrval;
  int fd, ret;
  long target_fd;
  LONGEST lnum;
  struct stat st;
  struct fio_stat fst;
  struct timeval tv;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror ();
      return;
    }
  fd = remote_fileio_map_fd ((int) target_fd);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd ();
      return;
    }
  /* 2. Parameter: Ptr to struct stat */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror ();
      return;
    }
  ptrval = (CORE_ADDR) lnum;

  remote_fio_no_longjmp = 1;
  if (fd == FIO_FD_CONSOLE_IN || fd == FIO_FD_CONSOLE_OUT)
    {
      remote_fileio_to_fio_uint (1, fst.fst_dev);
      memset (&st, 0, sizeof (st));
      st.st_mode = S_IFCHR | (fd == FIO_FD_CONSOLE_IN ? S_IRUSR : S_IWUSR);
      st.st_nlink = 1;
#ifdef HAVE_GETUID
      st.st_uid = getuid ();
#endif
#ifdef HAVE_GETGID
      st.st_gid = getgid ();
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
      st.st_blksize = 512;
#endif
#if HAVE_STRUCT_STAT_ST_BLOCKS
      st.st_blocks = 0;
#endif
      if (!gettimeofday (&tv, NULL))
	st.st_atime = st.st_mtime = st.st_ctime = tv.tv_sec;
      else
        st.st_atime = st.st_mtime = st.st_ctime = (time_t) 0;
      ret = 0;
    }
  else
    ret = fstat (fd, &st);

  if (ret == -1)
    {
      remote_fileio_return_errno (-1);
      return;
    }
  if (ptrval)
    {
      remote_fileio_to_fio_stat (&st, &fst);

      errno = target_write_memory (ptrval, (gdb_byte *) &fst, sizeof fst);
      if (errno != 0)
	{
	  remote_fileio_return_errno (-1);
	  return;
	}
    }
  remote_fileio_return_success (ret);
}

static void
remote_fileio_func_gettimeofday (char *buf)
{
  LONGEST lnum;
  CORE_ADDR ptrval;
  int ret;
  struct timeval tv;
  struct fio_timeval ftv;

  /* 1. Parameter: struct timeval pointer */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror ();
      return;
    }
  ptrval = (CORE_ADDR) lnum;
  /* 2. Parameter: some pointer value...  */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror ();
      return;
    }
  /* ...which has to be NULL.  */
  if (lnum)
    {
      remote_fileio_reply (-1, FILEIO_EINVAL);
      return;
    }

  remote_fio_no_longjmp = 1;
  ret = gettimeofday (&tv, NULL);

  if (ret == -1)
    {
      remote_fileio_return_errno (-1);
      return;
    }

  if (ptrval)
    {
      remote_fileio_to_fio_timeval (&tv, &ftv);

      errno = target_write_memory (ptrval, (gdb_byte *) &ftv, sizeof ftv);
      if (errno != 0)
	{
	  remote_fileio_return_errno (-1);
	  return;
	}
    }
  remote_fileio_return_success (ret);
}

static void
remote_fileio_func_isatty (char *buf)
{
  long target_fd;
  int fd;

  /* Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror ();
      return;
    }
  remote_fio_no_longjmp = 1;
  fd = remote_fileio_map_fd ((int) target_fd);
  remote_fileio_return_success (fd == FIO_FD_CONSOLE_IN ||
  				fd == FIO_FD_CONSOLE_OUT ? 1 : 0);
}

static void
remote_fileio_func_system (char *buf)
{
  CORE_ADDR ptrval;
  int ret, length;
  char *cmdline = NULL;

  /* Parameter: Ptr to commandline / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &ptrval, &length))
    {
      remote_fileio_ioerror ();
      return;
    }

  if (length)
    {
      /* Request commandline using 'm' packet */
      cmdline = alloca (length);
      if (target_read_memory (ptrval, (gdb_byte *) cmdline, length) != 0)
	{
	  remote_fileio_ioerror ();
	  return;
	}
    }
  
  /* Check if system(3) has been explicitely allowed using the
     `set remote system-call-allowed 1' command.  If length is 0,
     indicating a NULL parameter to the system call, return zero to
     indicate a shell is not available.  Otherwise fail with EPERM.  */
  if (!remote_fio_system_call_allowed)
    {
      if (!length)
	remote_fileio_return_success (0);
      else
	remote_fileio_reply (-1, FILEIO_EPERM);
      return;
    }

  remote_fio_no_longjmp = 1;
  ret = system (cmdline);

  if (!length)
    remote_fileio_return_success (ret);
  else if (ret == -1)
    remote_fileio_return_errno (-1);
  else
    remote_fileio_return_success (WEXITSTATUS (ret));
}

static struct {
  char *name;
  void (*func)(char *);
} remote_fio_func_map[] = {
  { "open", remote_fileio_func_open },
  { "close", remote_fileio_func_close },
  { "read", remote_fileio_func_read },
  { "write", remote_fileio_func_write },
  { "lseek", remote_fileio_func_lseek },
  { "rename", remote_fileio_func_rename },
  { "unlink", remote_fileio_func_unlink },
  { "stat", remote_fileio_func_stat },
  { "fstat", remote_fileio_func_fstat },
  { "gettimeofday", remote_fileio_func_gettimeofday },
  { "isatty", remote_fileio_func_isatty },
  { "system", remote_fileio_func_system },
  { NULL, NULL }
};

static int
do_remote_fileio_request (struct ui_out *uiout, void *buf_arg)
{
  char *buf = buf_arg;
  char *c;
  int idx;

  remote_fileio_sig_set (remote_fileio_ctrl_c_signal_handler);

  c = strchr (++buf, ',');
  if (c)
    *c++ = '\0';
  else
    c = strchr (buf, '\0');
  for (idx = 0; remote_fio_func_map[idx].name; ++idx)
    if (!strcmp (remote_fio_func_map[idx].name, buf))
      break;
  if (!remote_fio_func_map[idx].name)	/* ERROR: No such function.  */
    return RETURN_ERROR;
  remote_fio_func_map[idx].func (c);
  return 0;
}

/* Close any open descriptors, and reinitialize the file mapping.  */

void
remote_fileio_reset (void)
{
  int ix;

  for (ix = 0; ix != remote_fio_data.fd_map_size; ix++)
    {
      int fd = remote_fio_data.fd_map[ix];

      if (fd >= 0)
	close (fd);
    }
  if (remote_fio_data.fd_map)
    {
      xfree (remote_fio_data.fd_map);
      remote_fio_data.fd_map = NULL;
      remote_fio_data.fd_map_size = 0;
    }
}

/* Handle a file I/O request.  BUF points to the packet containing the
   request.  CTRLC_PENDING_P should be nonzero if the target has not
   acknowledged the Ctrl-C sent asynchronously earlier.  */

void
remote_fileio_request (char *buf, int ctrlc_pending_p)
{
  int ex;

  remote_fileio_sig_init ();

  if (ctrlc_pending_p)
    {
      /* If the target hasn't responded to the Ctrl-C sent
	 asynchronously earlier, take this opportunity to send the
	 Ctrl-C synchronously.  */
      remote_fio_ctrl_c_flag = 1;
      remote_fio_no_longjmp = 0;
      remote_fileio_reply (-1, FILEIO_EINTR);
    }
  else
    {
      remote_fio_ctrl_c_flag = 0;
      remote_fio_no_longjmp = 0;

      ex = catch_exceptions (current_uiout,
			     do_remote_fileio_request, (void *)buf,
			     RETURN_MASK_ALL);
      switch (ex)
	{
	case RETURN_ERROR:
	  remote_fileio_reply (-1, FILEIO_ENOSYS);
	  break;
	case RETURN_QUIT:
	  remote_fileio_reply (-1, FILEIO_EINTR);
	  break;
	default:
	  break;
	}
    }

  remote_fileio_sig_exit ();
}

static void
set_system_call_allowed (char *args, int from_tty)
{
  if (args)
    {
      char *arg_end;
      int val = strtoul (args, &arg_end, 10);

      if (*args && *arg_end == '\0')
        {
	  remote_fio_system_call_allowed = !!val;
	  return;
	}
    }
  error (_("Illegal argument for \"set remote system-call-allowed\" command"));
}

static void
show_system_call_allowed (char *args, int from_tty)
{
  if (args)
    error (_("Garbage after \"show remote "
	     "system-call-allowed\" command: `%s'"), args);
  printf_unfiltered ("Calling host system(3) call from target is %sallowed\n",
		     remote_fio_system_call_allowed ? "" : "not ");
}

void
initialize_remote_fileio (struct cmd_list_element *remote_set_cmdlist,
			  struct cmd_list_element *remote_show_cmdlist)
{
  sigint_fileio_token =
    create_async_signal_handler (async_remote_fileio_interrupt, NULL);

  add_cmd ("system-call-allowed", no_class,
	   set_system_call_allowed,
	   _("Set if the host system(3) call is allowed for the target."),
	   &remote_set_cmdlist);
  add_cmd ("system-call-allowed", no_class,
	   show_system_call_allowed,
	   _("Show if the host system(3) call is allowed for the target."),
	   &remote_show_cmdlist);
}
