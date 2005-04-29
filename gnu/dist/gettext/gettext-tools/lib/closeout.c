/* closeout.c - close standard output
   Copyright (C) 1998-2005 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "closeout.h"

#include <stdio.h>
#include <errno.h>

#if 0
#include "unlocked-io.h"
#include "__fpending.h"
#endif
#include "error.h"
#include "fwriteerror.h"
#include "exit.h"
#include "gettext.h"

#define _(msgid) gettext (msgid)

/* Close standard output, exiting with status STATUS on failure.
   If a program writes *anything* to stdout, that program should close
   stdout and make sure that it succeeds before exiting.  Otherwise,
   suppose that you go to the extreme of checking the return status
   of every function that does an explicit write to stdout.  The last
   printf can succeed in writing to the internal stream buffer, and yet
   the fclose(stdout) could still fail (due e.g., to a disk full error)
   when it tries to write out that buffered data.  Thus, you would be
   left with an incomplete output file and the offending program would
   exit successfully.  Even calling fflush is not always sufficient,
   since some file systems (NFS and CODA) buffer written/flushed data
   until an actual close call.

   Besides, it's wasteful to check the return value from every call
   that writes to stdout -- just let the internal stream state record
   the failure.  That's what the ferror test is checking below.

   It's important to detect such failures and exit nonzero because many
   tools (most notably `make' and other build-management systems) depend
   on being able to detect failure in other tools via their exit status.  */

static void
close_stdout_status (int status)
{
  if (fwriteerror (stdout))
    error (status, errno, "%s", _("write error"));
}

/* Close standard output, exiting with status EXIT_FAILURE on failure.  */
void
close_stdout (void)
{
  close_stdout_status (EXIT_FAILURE);
}

/* Note: When exit (...) calls the atexit-registered
              close_stdout (), which calls
              error (status, ...), which calls
              exit (status),
   we have undefined behaviour according to ISO C 99 section 7.20.4.3.(2).
   But in practice there is no problem: The second exit call is executed
   at a moment when the atexit handlers are no longer active.  */
