/* Machine-independent support for SVR4 /proc (process file system)

   Copyright (C) 1999-2017 Free Software Foundation, Inc.

   Written by Michael Snyder at Cygnus Solutions.
   Based on work by Fred Fish, Stu Grossman, Geoff Noer, and others.

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

#ifdef NEW_PROC_API
#define _STRUCTURED_PROC 1
#endif

#include <sys/types.h>
#include <sys/procfs.h>

#include "proc-utils.h"

/* Much of the information used in the /proc interface, particularly
   for printing status information, is kept as tables of structures of
   the following form.  These tables can be used to map numeric values
   to their symbolic names and to a string that describes their
   specific use.  */

struct trans
{
  int value;                    /* The numeric value.  */
  const char *name;                   /* The equivalent symbolic value.  */
  const char *desc;                   /* Short description of value.  */
};

/* Translate values in the pr_why field of a `struct prstatus' or
   `struct lwpstatus'.  */

static struct trans pr_why_table[] =
{
#if defined (PR_REQUESTED)
  /* All platforms.  */
  { PR_REQUESTED, "PR_REQUESTED", 
    "Directed to stop by debugger via P(IO)CSTOP or P(IO)CWSTOP" },
#endif
#if defined (PR_SIGNALLED)
  /* All platforms.  */
  { PR_SIGNALLED, "PR_SIGNALLED", "Receipt of a traced signal" },
#endif
#if defined (PR_SYSENTRY)
  /* All platforms.  */
  { PR_SYSENTRY, "PR_SYSENTRY", "Entry to a traced system call" },
#endif
#if defined (PR_SYSEXIT)
  /* All platforms.  */
  { PR_SYSEXIT, "PR_SYSEXIT", "Exit from a traced system call" },
#endif
#if defined (PR_JOBCONTROL)
  /* All platforms.  */
  { PR_JOBCONTROL, "PR_JOBCONTROL", "Default job control stop signal action" },
#endif
#if defined (PR_FAULTED)
  /* All platforms.  */
  { PR_FAULTED, "PR_FAULTED", "Incurred a traced hardware fault" },
#endif
#if defined (PR_SUSPENDED)
  /* Solaris only.  */
  { PR_SUSPENDED, "PR_SUSPENDED", "Process suspended" },
#endif
#if defined (PR_CHECKPOINT)
  /* Solaris only.  */
  { PR_CHECKPOINT, "PR_CHECKPOINT", "Process stopped at checkpoint" },
#endif
#if defined (PR_FORKSTOP)
  /* OSF/1 only.  */
  { PR_FORKSTOP, "PR_FORKSTOP", "Process stopped at end of fork call" },
#endif
#if defined (PR_TCRSTOP)
  /* OSF/1 only.  */
  { PR_TCRSTOP, "PR_TCRSTOP", "Process stopped on thread creation" },
#endif
#if defined (PR_TTSTOP)
  /* OSF/1 only.  */
  { PR_TTSTOP, "PR_TTSTOP", "Process stopped on thread termination" },
#endif
#if defined (PR_DEAD)
  /* OSF/1 only.  */
  { PR_DEAD, "PR_DEAD", "Process stopped in exit system call" },
#endif
};

/* Pretty-print the pr_why field of a `struct prstatus' or `struct
   lwpstatus'.  */

void
proc_prettyfprint_why (FILE *file, unsigned long why, unsigned long what,
		       int verbose)
{
  int i;

  if (why == 0)
    return;

  for (i = 0; i < ARRAY_SIZE (pr_why_table); i++)
    if (why == pr_why_table[i].value)
      {
	fprintf (file, "%s ", pr_why_table[i].name);
	if (verbose)
	  fprintf (file, ": %s ", pr_why_table[i].desc);

	switch (why) {
#ifdef PR_REQUESTED
	case PR_REQUESTED:
	  break;		/* Nothing more to print.  */
#endif
#ifdef PR_SIGNALLED
	case PR_SIGNALLED:
	  proc_prettyfprint_signal (file, what, verbose);
	  break;
#endif
#ifdef PR_FAULTED
	case PR_FAULTED:
	  proc_prettyfprint_fault (file, what, verbose);
	  break;
#endif
#ifdef PR_SYSENTRY
	case PR_SYSENTRY:
	  fprintf (file, "Entry to ");
	  proc_prettyfprint_syscall (file, what, verbose);
	  break;
#endif
#ifdef PR_SYSEXIT
	case PR_SYSEXIT:
	  fprintf (file, "Exit from ");
	  proc_prettyfprint_syscall (file, what, verbose);
	  break;
#endif
#ifdef PR_JOBCONTROL
	case PR_JOBCONTROL:
	  proc_prettyfprint_signal (file, what, verbose);
	  break;
#endif
#ifdef PR_DEAD
	case PR_DEAD:
	  fprintf (file, "Exit status: %ld\n", what);
	  break;
#endif
	default:
	  fprintf (file, "Unknown why %ld, what %ld\n", why, what);
	  break;
	}
	fprintf (file, "\n");

	return;
      }

  fprintf (file, "Unknown pr_why.\n");
}

void
proc_prettyprint_why (unsigned long why, unsigned long what, int verbose)
{
  proc_prettyfprint_why (stdout, why, what, verbose);
}
