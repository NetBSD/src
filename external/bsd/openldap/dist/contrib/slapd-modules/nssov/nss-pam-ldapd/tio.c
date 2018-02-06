/*	$NetBSD: tio.c,v 1.1.1.4 2018/02/06 01:53:06 christos Exp $	*/

/*
   tio.c - timed io functions
   This file is part of the nss-pam-ldapd library.

   Copyright (C) 2007-2014 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include <sys/cdefs.h>
__RCSID("$NetBSD: tio.c,v 1.1.1.4 2018/02/06 01:53:06 christos Exp $");

#include "portable.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <poll.h>
#include <time.h>

#include "tio.h"

/* for platforms that don't have ETIME use ETIMEDOUT */
#ifndef ETIME
#define ETIME ETIMEDOUT
#endif /* ETIME */

/* structure that holds a buffer
   the buffer contains the data that is between the application and the
   file descriptor that is used for efficient transfer
   the buffer is built up as follows:
   |.....********......|
         ^start        ^size
         ^--len--^           */
struct tio_buffer {
  uint8_t *buffer;
  size_t size;      /* the size of the buffer */
  size_t maxsize;   /* the maximum size of the buffer */
  size_t start;     /* the start of the data (before start is unused) */
  size_t len;       /* size of the data (from the start) */
};

/* structure that holds all the state for files */
struct tio_fileinfo {
  int fd;
  struct tio_buffer readbuffer;
  struct tio_buffer writebuffer;
  int readtimeout;
  int writetimeout;
  int read_resettable; /* whether the tio_reset() function can be called */
#ifdef DEBUG_TIO_STATS
  /* this is used to collect statistics on the use of the streams
     and can be used to tune the buffer sizes */
  size_t byteswritten;
  size_t bytesread;
#endif /* DEBUG_TIO_STATS */
};

/* some older versions of Solaris don't provide CLOCK_MONOTONIC but do have
   a CLOCK_HIGHRES that has the same properties we need */
#ifndef CLOCK_MONOTONIC
#ifdef CLOCK_HIGHRES
#define CLOCK_MONOTONIC CLOCK_HIGHRES
#endif /* CLOCK_HIGHRES */
#endif /* not CLOCK_MONOTONIC */

/* update the timeout to the value that is remaining before the deadline
   returns the number of milliseconds before the deadline (or a negative
   value of the deadline has expired) */
static inline int tio_time_remaining(struct timespec *deadline, int timeout)
{
  struct timespec tv;
  /* if this is the first call, set the deadline and return the full time */
  if ((deadline->tv_sec == 0) && (deadline->tv_nsec == 0))
  {
    if (clock_gettime(CLOCK_MONOTONIC, deadline) == 0)
    {
      deadline->tv_sec += timeout / 1000;
      deadline->tv_nsec += (timeout % 1000) * 1000000;
    }
    return timeout;
  }
  /* get the current time (fall back to full time on error) */
  if (clock_gettime(CLOCK_MONOTONIC, &tv))
    return timeout;
  /* calculate time remaining in milliseconds */
  return (deadline->tv_sec - tv.tv_sec) * 1000 +
         (deadline->tv_nsec - tv.tv_nsec) / 1000000;
}

/* open a new TFILE based on the file descriptor */
TFILE *tio_fdopen(int fd, int readtimeout, int writetimeout,
                  size_t initreadsize, size_t maxreadsize,
                  size_t initwritesize, size_t maxwritesize)
{
  struct tio_fileinfo *fp;
  fp = (struct tio_fileinfo *)malloc(sizeof(struct tio_fileinfo));
  if (fp == NULL)
    return NULL;
  fp->fd = fd;
  /* initialize read buffer */
  fp->readbuffer.buffer = (uint8_t *)malloc(initreadsize);
  if (fp->readbuffer.buffer == NULL)
  {
    free(fp);
    return NULL;
  }
  fp->readbuffer.size = initreadsize;
  fp->readbuffer.maxsize = maxreadsize;
  fp->readbuffer.start = 0;
  fp->readbuffer.len = 0;
  /* initialize write buffer */
  fp->writebuffer.buffer = (uint8_t *)malloc(initwritesize);
  if (fp->writebuffer.buffer == NULL)
  {
    free(fp->readbuffer.buffer);
    free(fp);
    return NULL;
  }
  fp->writebuffer.size = initwritesize;
  fp->writebuffer.maxsize = maxwritesize;
  fp->writebuffer.start = 0;
  fp->writebuffer.len = 0;
  /* initialize other attributes */
  fp->readtimeout = readtimeout;
  fp->writetimeout = writetimeout;
  fp->read_resettable = 0;
#ifdef DEBUG_TIO_STATS
  fp->byteswritten = 0;
  fp->bytesread = 0;
#endif /* DEBUG_TIO_STATS */
  return fp;
}

/* wait for any activity on the specified file descriptor using
   the specified deadline */
static int tio_wait(int fd, short events, int timeout,
                    struct timespec *deadline)
{
  int t;
  struct pollfd fds[1];
  int rv;
  while (1)
  {
    fds[0].fd = fd;
    fds[0].events = events;
    /* figure out the time we need to wait */
    if ((t = tio_time_remaining(deadline, timeout)) < 0)
    {
      errno = ETIME;
      return -1;
    }
    /* sanitiy check for moving clock */
    if (t > timeout)
      t = timeout;
    /* wait for activity */
    rv = poll(fds, 1, t);
    if (rv > 0)
      return 0; /* we have activity */
    else if (rv == 0)
    {
      /* no file descriptors were available within the specified time */
      errno = ETIME;
      return -1;
    }
    else if ((errno != EINTR) && (errno != EAGAIN))
      /* some error ocurred */
      return -1;
    /* we just try again on EINTR or EAGAIN */
  }
}

/* do a read on the file descriptor, returning the data in the buffer
   if no data was read in the specified time an error is returned */
int tio_read(TFILE *fp, void *buf, size_t count)
{
  struct timespec deadline = {0, 0};
  int rv;
  uint8_t *tmp;
  size_t newsz;
  size_t len;
  /* have a more convenient storage type for the buffer */
  uint8_t *ptr = (uint8_t *)buf;
  /* loop until we have returned all the needed data */
  while (1)
  {
    /* check if we have enough data in the buffer */
    if (fp->readbuffer.len >= count)
    {
      if (count > 0)
      {
        if (ptr != NULL)
          memcpy(ptr, fp->readbuffer.buffer + fp->readbuffer.start, count);
        /* adjust buffer position */
        fp->readbuffer.start += count;
        fp->readbuffer.len -= count;
      }
      return 0;
    }
    /* empty what we have and continue from there */
    if (fp->readbuffer.len > 0)
    {
      if (ptr != NULL)
      {
        memcpy(ptr, fp->readbuffer.buffer + fp->readbuffer.start,
               fp->readbuffer.len);
        ptr += fp->readbuffer.len;
      }
      count -= fp->readbuffer.len;
      fp->readbuffer.start += fp->readbuffer.len;
      fp->readbuffer.len = 0;
    }
    /* after this point until the read fp->readbuffer.len is 0 */
    if (!fp->read_resettable)
    {
      /* the stream is not resettable, re-use the buffer */
      fp->readbuffer.start = 0;
    }
    else if (fp->readbuffer.start >= (fp->readbuffer.size - 4))
    {
      /* buffer is running empty, try to grow buffer */
      if (fp->readbuffer.size < fp->readbuffer.maxsize)
      {
        newsz = fp->readbuffer.size * 2;
        if (newsz > fp->readbuffer.maxsize)
          newsz = fp->readbuffer.maxsize;
        tmp = realloc(fp->readbuffer.buffer, newsz);
        if (tmp != NULL)
        {
          fp->readbuffer.buffer = tmp;
          fp->readbuffer.size = newsz;
        }
      }
      /* if buffer still does not contain enough room, clear resettable */
      if (fp->readbuffer.start >= (fp->readbuffer.size - 4))
      {
        fp->readbuffer.start = 0;
        fp->read_resettable = 0;
      }
    }
    /* wait until we have input */
    if (tio_wait(fp->fd, POLLIN, fp->readtimeout, &deadline))
      return -1;
    /* read the input in the buffer */
    len = fp->readbuffer.size - fp->readbuffer.start;
#ifdef SSIZE_MAX
    if (len > SSIZE_MAX)
      len = SSIZE_MAX;
#endif /* SSIZE_MAX */
    rv = read(fp->fd, fp->readbuffer.buffer + fp->readbuffer.start, len);
    /* check for errors */
    if (rv == 0)
    {
      errno = ECONNRESET;
      return -1;
    }
    else if ((rv < 0) && (errno != EINTR) && (errno != EAGAIN))
      return -1;        /* something went wrong with the read */
    else if (rv > 0)
      fp->readbuffer.len = rv;  /* skip the read part in the buffer */
#ifdef DEBUG_TIO_STATS
    fp->bytesread += rv;
#endif /* DEBUG_TIO_STATS */
  }
}

/* Read and discard the specified number of bytes from the stream. */
int tio_skip(TFILE *fp, size_t count)
{
  return tio_read(fp, NULL, count);
}

/* Read all available data from the stream and empty the read buffer. */
int tio_skipall(TFILE *fp, int timeout)
{
  struct timespec deadline = {0, 0};
  int rv;
  size_t len;
  /* clear the read buffer */
  fp->readbuffer.start = 0;
  fp->readbuffer.len = 0;
  fp->read_resettable = 0;
  /* read until we can't read no more */
  len = fp->readbuffer.size;
#ifdef SSIZE_MAX
  if (len > SSIZE_MAX)
    len = SSIZE_MAX;
#endif /* SSIZE_MAX */
  while (1)
  {
    /* wait until we have input */
    if (tio_wait(fp->fd, POLLIN, timeout, &deadline))
      return -1;
    /* read data from the stream */
    rv = read(fp->fd, fp->readbuffer.buffer, len);
    if (rv == 0)
      return 0; /* end-of-file */
    if ((rv < 0) && (errno == EWOULDBLOCK))
      return 0; /* we've ready everything we can without blocking */
    if ((rv < 0) && (errno != EINTR) && (errno != EAGAIN))
      return -1; /* something went wrong with the read */
  }
}

/* the caller has assured us that we can write to the file descriptor
   and we give it a shot */
static int tio_writebuf(TFILE *fp)
{
  int rv;
  /* write the buffer */
#ifdef MSG_NOSIGNAL
  rv = send(fp->fd, fp->writebuffer.buffer + fp->writebuffer.start,
            fp->writebuffer.len, MSG_NOSIGNAL);
#else /* not MSG_NOSIGNAL */
  /* on platforms that cannot use send() with masked signals, we change the
     signal mask and change it back after the write (note that there is a
     race condition here) */
  struct sigaction act, oldact;
  /* set up sigaction */
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_sigaction = NULL;
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  /* ignore SIGPIPE */
  if (sigaction(SIGPIPE, &act, &oldact) != 0)
    return -1; /* error setting signal handler */
  /* write the buffer */
  rv = write(fp->fd, fp->writebuffer.buffer + fp->writebuffer.start,
             fp->writebuffer.len);
  /* restore the old handler for SIGPIPE */
  if (sigaction(SIGPIPE, &oldact, NULL) != 0)
    return -1; /* error restoring signal handler */
#endif
  /* check for errors */
  if ((rv == 0) || ((rv < 0) && (errno != EINTR) && (errno != EAGAIN)))
    return -1; /* something went wrong with the write */
  /* skip the written part in the buffer */
  if (rv > 0)
  {
    fp->writebuffer.start += rv;
    fp->writebuffer.len -= rv;
#ifdef DEBUG_TIO_STATS
    fp->byteswritten += rv;
#endif /* DEBUG_TIO_STATS */
    /* reset start if len is 0 */
    if (fp->writebuffer.len == 0)
      fp->writebuffer.start = 0;
    /* move contents of the buffer to the front if it will save enough room */
    if (fp->writebuffer.start >= (fp->writebuffer.size / 4))
    {
      memmove(fp->writebuffer.buffer,
              fp->writebuffer.buffer + fp->writebuffer.start,
              fp->writebuffer.len);
      fp->writebuffer.start = 0;
    }
  }
  return 0;
}

/* write all the data in the buffer to the stream */
int tio_flush(TFILE *fp)
{
  struct timespec deadline = {0, 0};
  /* loop until we have written our buffer */
  while (fp->writebuffer.len > 0)
  {
    /* wait until we can write */
    if (tio_wait(fp->fd, POLLOUT, fp->writetimeout, &deadline))
      return -1;
    /* write one block */
    if (tio_writebuf(fp))
      return -1;
  }
  return 0;
}

/* try a single write of data in the buffer if the file descriptor
   will accept data */
static int tio_flush_nonblock(TFILE *fp)
{
  struct pollfd fds[1];
  int rv;
  /* see if we can write without blocking */
  fds[0].fd = fp->fd;
  fds[0].events = POLLOUT;
  rv = poll(fds, 1, 0);
  /* check if any file descriptors were ready (timeout) or we were
     interrupted */
  if ((rv == 0) || ((rv < 0) && ((errno == EINTR) || (errno == EAGAIN))))
    return 0;
  /* any other errors? */
  if (rv < 0)
    return -1;
  /* so file descriptor will accept writes */
  return tio_writebuf(fp);
}

int tio_write(TFILE *fp, const void *buf, size_t count)
{
  size_t fr;
  uint8_t *tmp;
  size_t newsz;
  const uint8_t *ptr = (const uint8_t *)buf;
  /* keep filling the buffer until we have bufferred everything */
  while (count > 0)
  {
    /* figure out free size in buffer */
    fr = fp->writebuffer.size - (fp->writebuffer.start + fp->writebuffer.len);
    if (count <= fr)
    {
      /* the data fits in the buffer */
      memcpy(fp->writebuffer.buffer + fp->writebuffer.start +
             fp->writebuffer.len, ptr, count);
      fp->writebuffer.len += count;
      return 0;
    }
    else if (fr > 0)
    {
      /* fill the buffer with data that will fit */
      memcpy(fp->writebuffer.buffer + fp->writebuffer.start +
             fp->writebuffer.len, ptr, fr);
      fp->writebuffer.len += fr;
      ptr += fr;
      count -= fr;
    }
    /* try to flush some of the data that is in the buffer */
    if (tio_flush_nonblock(fp))
      return -1;
    /* if we have room now, try again */
    if (fp->writebuffer.size > (fp->writebuffer.start + fp->writebuffer.len))
      continue;
    /* try to grow the buffer */
    if (fp->writebuffer.size < fp->writebuffer.maxsize)
    {
      newsz = fp->writebuffer.size * 2;
      if (newsz > fp->writebuffer.maxsize)
        newsz = fp->writebuffer.maxsize;
      tmp = realloc(fp->writebuffer.buffer, newsz);
      if (tmp != NULL)
      {
        fp->writebuffer.buffer = tmp;
        fp->writebuffer.size = newsz;
        continue; /* try again */
      }
    }
    /* write the buffer to the stream */
    if (tio_flush(fp))
      return -1;
  }
  return 0;
}

int tio_close(TFILE *fp)
{
  int retv;
  /* write any buffered data */
  retv = tio_flush(fp);
#ifdef DEBUG_TIO_STATS
  /* dump statistics to stderr */
  fprintf(stderr, "DEBUG_TIO_STATS READ=%d WRITTEN=%d\n", fp->bytesread,
          fp->byteswritten);
#endif /* DEBUG_TIO_STATS */
  /* close file descriptor */
  if (close(fp->fd))
    retv = -1;
  /* free any allocated buffers */
  memset(fp->readbuffer.buffer, 0, fp->readbuffer.size);
  memset(fp->writebuffer.buffer, 0, fp->writebuffer.size);
  free(fp->readbuffer.buffer);
  free(fp->writebuffer.buffer);
  /* free the tio struct itself */
  free(fp);
  /* return the result of the earlier operations */
  return retv;
}

void tio_mark(TFILE *fp)
{
  /* move any data in the buffer to the start of the buffer */
  if ((fp->readbuffer.start > 0) && (fp->readbuffer.len > 0))
  {
    memmove(fp->readbuffer.buffer,
            fp->readbuffer.buffer + fp->readbuffer.start, fp->readbuffer.len);
    fp->readbuffer.start = 0;
  }
  /* mark the stream as resettable */
  fp->read_resettable = 1;
}

int tio_reset(TFILE *fp)
{
  /* check if the stream is (still) resettable */
  if (!fp->read_resettable)
    return -1;
  /* reset the buffer */
  fp->readbuffer.len += fp->readbuffer.start;
  fp->readbuffer.start = 0;
  return 0;
}
