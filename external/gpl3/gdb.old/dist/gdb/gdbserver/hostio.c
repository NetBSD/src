/* Host file transfer support for gdbserver.
   Copyright (C) 2007-2015 Free Software Foundation, Inc.

   Contributed by CodeSourcery.

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

#include "server.h"
#include "gdb/fileio.h"
#include "hostio.h"

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

extern int remote_debug;

struct fd_list
{
  int fd;
  struct fd_list *next;
};

static struct fd_list *open_fds;

static int
safe_fromhex (char a, int *nibble)
{
  if (a >= '0' && a <= '9')
    *nibble = a - '0';
  else if (a >= 'a' && a <= 'f')
    *nibble = a - 'a' + 10;
  else if (a >= 'A' && a <= 'F')
    *nibble = a - 'A' + 10;
  else
    return -1;

  return 0;
}

/* Filenames are hex encoded, so the maximum we can handle is half the
   packet buffer size.  Cap to PATH_MAX, if it is shorter.  */
#if !defined (PATH_MAX) || (PATH_MAX > (PBUFSIZ / 2 + 1))
#  define HOSTIO_PATH_MAX (PBUFSIZ / 2 + 1)
#else
#  define HOSTIO_PATH_MAX PATH_MAX
#endif

static int
require_filename (char **pp, char *filename)
{
  int count;
  char *p;

  p = *pp;
  count = 0;

  while (*p && *p != ',')
    {
      int nib1, nib2;

      /* Don't allow overflow.  */
      if (count >= HOSTIO_PATH_MAX - 1)
	return -1;

      if (safe_fromhex (p[0], &nib1)
	  || safe_fromhex (p[1], &nib2))
	return -1;

      filename[count++] = nib1 * 16 + nib2;
      p += 2;
    }

  filename[count] = '\0';
  *pp = p;
  return 0;
}

static int
require_int (char **pp, int *value)
{
  char *p;
  int count;

  p = *pp;
  *value = 0;
  count = 0;

  while (*p && *p != ',')
    {
      int nib;

      /* Don't allow overflow.  */
      if (count >= 7)
	return -1;

      if (safe_fromhex (p[0], &nib))
	return -1;
      *value = *value * 16 + nib;
      p++;
      count++;
    }

  *pp = p;
  return 0;
}

static int
require_data (char *p, int p_len, char **data, int *data_len)
{
  int input_index, output_index, escaped;

  *data = xmalloc (p_len);

  output_index = 0;
  escaped = 0;
  for (input_index = 0; input_index < p_len; input_index++)
    {
      char b = p[input_index];

      if (escaped)
	{
	  (*data)[output_index++] = b ^ 0x20;
	  escaped = 0;
	}
      else if (b == '}')
	escaped = 1;
      else
	(*data)[output_index++] = b;
    }

  if (escaped)
    {
      free (*data);
      return -1;
    }

  *data_len = output_index;
  return 0;
}

static int
require_comma (char **pp)
{
  if (**pp == ',')
    {
      (*pp)++;
      return 0;
    }
  else
    return -1;
}

static int
require_end (char *p)
{
  if (*p == '\0')
    return 0;
  else
    return -1;
}

static int
require_valid_fd (int fd)
{
  struct fd_list *fd_ptr;

  for (fd_ptr = open_fds; fd_ptr != NULL; fd_ptr = fd_ptr->next)
    if (fd_ptr->fd == fd)
      return 0;

  return -1;
}

/* Fill in own_buf with the last hostio error packet, however it
   suitable for the target.  */
static void
hostio_error (char *own_buf)
{
  the_target->hostio_last_error (own_buf);
}

static void
hostio_packet_error (char *own_buf)
{
  sprintf (own_buf, "F-1,%x", FILEIO_EINVAL);
}

static void
hostio_reply (char *own_buf, int result)
{
  sprintf (own_buf, "F%x", result);
}

static int
hostio_reply_with_data (char *own_buf, char *buffer, int len,
			int *new_packet_len)
{
  int input_index, output_index, out_maxlen;

  sprintf (own_buf, "F%x;", len);
  output_index = strlen (own_buf);

  out_maxlen = PBUFSIZ;

  for (input_index = 0; input_index < len; input_index++)
    {
      char b = buffer[input_index];

      if (b == '$' || b == '#' || b == '}' || b == '*')
	{
	  /* These must be escaped.  */
	  if (output_index + 2 > out_maxlen)
	    break;
	  own_buf[output_index++] = '}';
	  own_buf[output_index++] = b ^ 0x20;
	}
      else
	{
	  if (output_index + 1 > out_maxlen)
	    break;
	  own_buf[output_index++] = b;
	}
    }

  *new_packet_len = output_index;
  return input_index;
}

static int
fileio_open_flags_to_host (int fileio_open_flags, int *open_flags_p)
{
  int open_flags = 0;

  if (fileio_open_flags & ~FILEIO_O_SUPPORTED)
    return -1;

  if (fileio_open_flags & FILEIO_O_CREAT)
    open_flags |= O_CREAT;
  if (fileio_open_flags & FILEIO_O_EXCL)
    open_flags |= O_EXCL;
  if (fileio_open_flags & FILEIO_O_TRUNC)
    open_flags |= O_TRUNC;
  if (fileio_open_flags & FILEIO_O_APPEND)
    open_flags |= O_APPEND;
  if (fileio_open_flags & FILEIO_O_RDONLY)
    open_flags |= O_RDONLY;
  if (fileio_open_flags & FILEIO_O_WRONLY)
    open_flags |= O_WRONLY;
  if (fileio_open_flags & FILEIO_O_RDWR)
    open_flags |= O_RDWR;
/* On systems supporting binary and text mode, always open files in
   binary mode. */
#ifdef O_BINARY
  open_flags |= O_BINARY;
#endif

  *open_flags_p = open_flags;
  return 0;
}

static void
handle_open (char *own_buf)
{
  char filename[HOSTIO_PATH_MAX];
  char *p;
  int fileio_flags, mode, flags, fd;
  struct fd_list *new_fd;

  p = own_buf + strlen ("vFile:open:");

  if (require_filename (&p, filename)
      || require_comma (&p)
      || require_int (&p, &fileio_flags)
      || require_comma (&p)
      || require_int (&p, &mode)
      || require_end (p)
      || fileio_open_flags_to_host (fileio_flags, &flags))
    {
      hostio_packet_error (own_buf);
      return;
    }

  /* We do not need to convert MODE, since the fileio protocol
     uses the standard values.  */
  fd = open (filename, flags, mode);

  if (fd == -1)
    {
      hostio_error (own_buf);
      return;
    }

  /* Record the new file descriptor.  */
  new_fd = xmalloc (sizeof (struct fd_list));
  new_fd->fd = fd;
  new_fd->next = open_fds;
  open_fds = new_fd;

  hostio_reply (own_buf, fd);
}

static void
handle_pread (char *own_buf, int *new_packet_len)
{
  int fd, ret, len, offset, bytes_sent;
  char *p, *data;

  p = own_buf + strlen ("vFile:pread:");

  if (require_int (&p, &fd)
      || require_comma (&p)
      || require_valid_fd (fd)
      || require_int (&p, &len)
      || require_comma (&p)
      || require_int (&p, &offset)
      || require_end (p))
    {
      hostio_packet_error (own_buf);
      return;
    }

  data = xmalloc (len);
#ifdef HAVE_PREAD
  ret = pread (fd, data, len, offset);
#else
  ret = -1;
#endif
  /* If we have no pread or it failed for this file, use lseek/read.  */
  if (ret == -1)
    {
      ret = lseek (fd, offset, SEEK_SET);
      if (ret != -1)
	ret = read (fd, data, len);
    }

  if (ret == -1)
    {
      hostio_error (own_buf);
      free (data);
      return;
    }

  bytes_sent = hostio_reply_with_data (own_buf, data, ret, new_packet_len);

  /* If we were using read, and the data did not all fit in the reply,
     we would have to back up using lseek here.  With pread it does
     not matter.  But we still have a problem; the return value in the
     packet might be wrong, so we must fix it.  This time it will
     definitely fit.  */
  if (bytes_sent < ret)
    bytes_sent = hostio_reply_with_data (own_buf, data, bytes_sent,
					 new_packet_len);

  free (data);
}

static void
handle_pwrite (char *own_buf, int packet_len)
{
  int fd, ret, len, offset;
  char *p, *data;

  p = own_buf + strlen ("vFile:pwrite:");

  if (require_int (&p, &fd)
      || require_comma (&p)
      || require_valid_fd (fd)
      || require_int (&p, &offset)
      || require_comma (&p)
      || require_data (p, packet_len - (p - own_buf), &data, &len))
    {
      hostio_packet_error (own_buf);
      return;
    }

#ifdef HAVE_PWRITE
  ret = pwrite (fd, data, len, offset);
#else
  ret = -1;
#endif
  /* If we have no pwrite or it failed for this file, use lseek/write.  */
  if (ret == -1)
    {
      ret = lseek (fd, offset, SEEK_SET);
      if (ret != -1)
	ret = write (fd, data, len);
    }

  if (ret == -1)
    {
      hostio_error (own_buf);
      free (data);
      return;
    }

  hostio_reply (own_buf, ret);
  free (data);
}

static void
handle_close (char *own_buf)
{
  int fd, ret;
  char *p;
  struct fd_list **open_fd_p, *old_fd;

  p = own_buf + strlen ("vFile:close:");

  if (require_int (&p, &fd)
      || require_valid_fd (fd)
      || require_end (p))
    {
      hostio_packet_error (own_buf);
      return;
    }

  ret = close (fd);

  if (ret == -1)
    {
      hostio_error (own_buf);
      return;
    }

  open_fd_p = &open_fds;
  /* We know that fd is in the list, thanks to require_valid_fd.  */
  while ((*open_fd_p)->fd != fd)
    open_fd_p = &(*open_fd_p)->next;

  old_fd = *open_fd_p;
  *open_fd_p = (*open_fd_p)->next;
  free (old_fd);

  hostio_reply (own_buf, ret);
}

static void
handle_unlink (char *own_buf)
{
  char filename[HOSTIO_PATH_MAX];
  char *p;
  int ret;

  p = own_buf + strlen ("vFile:unlink:");

  if (require_filename (&p, filename)
      || require_end (p))
    {
      hostio_packet_error (own_buf);
      return;
    }

  ret = unlink (filename);

  if (ret == -1)
    {
      hostio_error (own_buf);
      return;
    }

  hostio_reply (own_buf, ret);
}

static void
handle_readlink (char *own_buf, int *new_packet_len)
{
  char filename[HOSTIO_PATH_MAX], linkname[HOSTIO_PATH_MAX];
  char *p;
  int ret, bytes_sent;

  p = own_buf + strlen ("vFile:readlink:");

  if (require_filename (&p, filename)
      || require_end (p))
    {
      hostio_packet_error (own_buf);
      return;
    }

  ret = readlink (filename, linkname, sizeof (linkname) - 1);
  if (ret == -1)
    {
      hostio_error (own_buf);
      return;
    }

  bytes_sent = hostio_reply_with_data (own_buf, linkname, ret, new_packet_len);

  /* If the response does not fit into a single packet, do not attempt
     to return a partial response, but simply fail.  */
  if (bytes_sent < ret)
    sprintf (own_buf, "F-1,%x", FILEIO_ENAMETOOLONG);
}

/* Handle all the 'F' file transfer packets.  */

int
handle_vFile (char *own_buf, int packet_len, int *new_packet_len)
{
  if (strncmp (own_buf, "vFile:open:", 11) == 0)
    handle_open (own_buf);
  else if (strncmp (own_buf, "vFile:pread:", 11) == 0)
    handle_pread (own_buf, new_packet_len);
  else if (strncmp (own_buf, "vFile:pwrite:", 12) == 0)
    handle_pwrite (own_buf, packet_len);
  else if (strncmp (own_buf, "vFile:close:", 12) == 0)
    handle_close (own_buf);
  else if (strncmp (own_buf, "vFile:unlink:", 13) == 0)
    handle_unlink (own_buf);
  else if (strncmp (own_buf, "vFile:readlink:", 15) == 0)
    handle_readlink (own_buf, new_packet_len);
  else
    return 0;

  return 1;
}
