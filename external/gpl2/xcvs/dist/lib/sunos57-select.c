/* Work around the bug in Solaris 7 whereby a fd that is opened on
   /dev/null will cause select/poll to hang when given a NULL timeout.

   Copyright (C) 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* written by Mark D. Baushke */

/*
 * Observed on Solaris 7:
 *   If /dev/null is in the readfds set, it will never be marked as
 *   ready by the OS. In the case of a /dev/null fd being the only fd
 *   in the select set and timeout == NULL, the select will hang.
 *   If /dev/null is in the exceptfds set, it will not be set on
 *   return from select().
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

/* The rpl_select function calls the real select. */
#undef select

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "minmax.h"
#include "xtime.h"

static struct stat devnull;
static int devnull_set = -1;
int
rpl_select (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
            struct timeval *timeout)
{
  int ret = 0;

  /* Argument checking */
  if (nfds < 1 || nfds > FD_SETSIZE)
    {
      errno = EINVAL;
      return -1;
    }

  /* Perform the initial stat on /dev/null */
  if (devnull_set == -1)
    devnull_set = stat ("/dev/null", &devnull);

  if (devnull_set >= 0)
    {
      int fd;
      int maxfd = -1;
      fd_set null_rfds, null_wfds;
      bool altered = false;	/* Whether we have altered the caller's args.
				 */

      FD_ZERO (&null_rfds);
      FD_ZERO (&null_wfds);

      for (fd = 0; fd < nfds; fd++)
	{
	  /* Check the callers bits for interesting fds */
	  bool isread = (readfds && FD_ISSET (fd, readfds));
	  bool isexcept = (exceptfds && FD_ISSET (fd, exceptfds));
	  bool iswrite = (writefds && FD_ISSET (fd, writefds));

	  /* Check interesting fds against /dev/null */
	  if (isread || iswrite || isexcept)
	    {
	      struct stat sb;

	      /* Equivalent to /dev/null ? */
	      if (fstat (fd, &sb) >= 0
		  && sb.st_dev == devnull.st_dev
		  && sb.st_ino == devnull.st_ino
		  && sb.st_mode == devnull.st_mode
		  && sb.st_uid == devnull.st_uid
		  && sb.st_gid == devnull.st_gid
		  && sb.st_size == devnull.st_size
		  && sb.st_blocks == devnull.st_blocks
		  && sb.st_blksize == devnull.st_blksize)
		{
		  /* Save the interesting bits for later use. */
		  if (isread)
		    {
		      FD_SET (fd, &null_rfds);
		      FD_CLR (fd, readfds);
		      altered = true;
		    }
		  if (isexcept)
		    /* Pass exception bits through.
		     *
		     * At the moment, we only know that this bug
		     * exists in Solaris 7 and so this file should
		     * only be compiled on Solaris 7. Since Solaris 7
		     * never returns ready for exceptions on
		     * /dev/null, we probably could assume this too,
		     * but since Solaris 9 is known to always return
		     * ready for exceptions on /dev/null, pass this
		     * through in case any other systems turn out to
		     * do the same. Besides, this will cause the
		     * timeout to be processed as it would have been
		     * otherwise.
		     */
		    maxfd = MAX (maxfd, fd);
		  if (iswrite)
		    {
		      /* We know of no bugs involving selecting /dev/null
		       * writefds, but we also know that /dev/null is always
		       * ready for write.  Therefore, since we have already
		       * performed all the necessary processing, avoid calling
		       * the system select for this case.
		       */
		      FD_SET (fd, &null_wfds);
		      FD_CLR (fd, writefds);
		      altered = true;
		    }
		}
	      else
		/* A non-/dev/null fd is present. */
		maxfd = MAX (maxfd, fd);
	    }
	}

      if (maxfd >= 0)
	{
	  /* we need to call select, one way or another.  */
	  if (altered)
	    {
	      /* We already have some ready bits set, so timeout immediately
	       * if no bits are set.
	       */
	      struct timeval ztime;
	      ztime.tv_sec = 0;
	      ztime.tv_usec = 0;
	      ret = select (maxfd + 1, readfds, writefds, exceptfds, &ztime);
	      if (ret == 0)
		{
		  /* Timeout.  Zero the sets since the system select might
		   * not have.
		   */
		  if (readfds)
		    FD_ZERO (readfds);
		  if (exceptfds)
		    FD_ZERO (exceptfds);
		  if (writefds)
		    FD_ZERO (writefds);
		}
	    }
	  else
	    /* No /dev/null fds.  Call select just as the user specified.  */
	    ret = select (maxfd + 1, readfds, writefds, exceptfds, timeout);
	}

      /*
       * Borrowed from the Solaris 7 man page for select(3c):
       * 
       *   On successful completion, the objects pointed to by the
       *   readfds, writefds, and exceptfds arguments are modified to
       *   indicate which file descriptors are ready for reading,
       *   ready for writing, or have an error condition pending,
       *   respectively. For each file descriptor less than nfds, the
       *   corresponding bit will be set on successful completion if
       *   it was set on input and the associated condition is true
       *   for that file descriptor.
       *  
       *   On failure, the objects pointed to by the readfds,
       *   writefds, and exceptfds arguments are not modified. If the
       *   timeout interval expires without the specified condition
       *   being true for any of the specified file descriptors, the
       *   objects pointed to by the readfs, writefs, and errorfds
       *   arguments have all bits set to 0.
       *  
       *   On successful completion, select() returns the total number
       *   of bits set in the bit masks. Otherwise, -1 is returned,
       *   and errno is set to indicate the error.
       */

      /* Fix up the fd sets for any changes we may have made. */
      if (altered)
	{
	  /* Tell the caller that nothing is blocking the /dev/null fds */
	  for (fd = 0; fd < nfds; fd++)
	    {
	      /* If ret < 0, then we still need to restore the fd sets.  */
	      if (FD_ISSET (fd, &null_rfds))
		{
		  FD_SET (fd, readfds);
		  if (ret >= 0)
		    ret++;
		}
	      if (FD_ISSET (fd, &null_wfds))
		{
		  FD_SET (fd, writefds);
		  if (ret >= 0)
		    ret++;
		}		  
	    }
	}
    }
  else
    ret = select (nfds, readfds, writefds, exceptfds, timeout);

  return ret;
}
