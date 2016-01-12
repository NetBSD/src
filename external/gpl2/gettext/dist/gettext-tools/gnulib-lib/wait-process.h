/* Waiting for a subprocess to finish.
   Copyright (C) 2001-2003, 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _WAIT_PROCESS_H
#define _WAIT_PROCESS_H

/* Get pid_t.  */
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Wait for a subprocess to finish.  Return its exit code.
   If it didn't terminate correctly, exit if exit_on_error is true, otherwise
   return 127.
   Arguments:
   - child is the pid of the subprocess.
   - progname is the name of the program executed by the subprocess, used for
     error messages.
   - If ignore_sigpipe is true, consider a subprocess termination due to
     SIGPIPE as equivalent to a success.  This is suitable for processes whose
     only purpose is to write to standard output.  This flag can be safely set
     to false when the process' standard output is known to go to DEV_NULL.
   - If null_stderr is true, the usual error message to stderr will be omitted.
     This is suitable when the subprocess does not fulfill an important task.
   - slave_process should be set to true if the process has been launched as a
     slave process.
   - If exit_on_error is true, any error will cause the main process to exit
     with an error status.  */
extern int wait_subprocess (pid_t child, const char *progname,
			    bool ignore_sigpipe, bool null_stderr,
			    bool slave_process, bool exit_on_error);

/* Register a subprocess as being a slave process.  This means that the
   subprocess will be terminated when its creator receives a catchable fatal
   signal or exits normally.  Registration ends when wait_subprocess()
   notices that the subprocess has exited.  */
extern void register_slave_subprocess (pid_t child);


#ifdef __cplusplus
}
#endif


#endif /* _WAIT_PROCESS_H */
