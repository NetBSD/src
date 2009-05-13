/* error.c -- error handler for noninteractive utilities
   Copyright (C) 1990-1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/* David MacKenzie */
/* Brian Berliner added support for CVS */

#include "cvs.h"
#include "vasnprintf.h"

/* Out of memory errors which could not be forwarded to the client are sent to
 * the syslog when it is available.
 */
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
# ifndef LOG_DAEMON   /* for ancient syslogs */
#   define LOG_DAEMON 0
# endif
#endif /* HAVE_SYSLOG_H */



/* If non-zero, error will use the CVS protocol to stdout to report error
 * messages.  This will only be set in the CVS server parent process.
 *
 * Most other code is run via do_cvs_command, which forks off a child
 * process and packages up its stderr in the protocol.
 */
int error_use_protocol; 

#ifndef strerror
extern char *strerror (int);
#endif



/* Print the program name and error message MESSAGE, which is a printf-style
 * format string with optional args, like:
 *
 *   PROGRAM_NAME CVS_CMD_NAME: MESSAGE: ERRNUM
 *
 * or, when STATUS is non-zero:
 *
 *   PROGRAM_NAME [CVS_CMD_NAME aborted]: MESSAGE: ERRNUM
 *
 * CVS_CMD_NAME & ERRMSG may or may not appear in the output (the `:' before
 * ERRMSG will disappear as well when ERRNUM is not present).  ERRMSG
 * represents the system dependent message returned by strerror (ERRNUM), when
 * ERRNUM is non-zero.
 *
 * Exit with status EXIT_FAILURE if STATUS is nonzero.
 *
 * If this function fails to get any memory it might request, it attempts to
 * log a "memory exhausted" message to the syslog, when syslog is available,
 * without any further attempts to allocate memory, before exiting.  See NOTES
 * below for more information on this functions memory allocation.
 *
 * INPUTS
 *   status	When non-zero, exit with EXIT_FAILURE rather than returning.
 *   errnum	When non-zero, interpret as global ERRNO for the purpose of
 *		generating additional error text.
 *   message	A printf style format string.
 *   ...	Variable number of args, as printf.
 *
 * GLOBALS
 *   program_name	The name of this executable, for the output message.
 *   cvs_cmd_name	Output in the error message, when it exists.
 *   errno		Accessed simply to save and restore it before
 *			returning.
 *
 * NOTES
 *   This function goes to fairly great lengths to avoid allocating memory so
 *   that it can relay out-of-memory error messages to the client.  Any error
 *   messages which fit in under 256 characters (after expanding MESSAGE with
 *   ARGS but before adding any ERRNUM text) should not require memory
 *   allocation before they are sent on to cvs_outerr().  Unfortunately,
 *   cvs_outerr() and the buffer functions it uses to send messages to the
 *   client still don't make this same sort of effort, so in local mode
 *   out-of-memory errors will probably get printed properly to stderr but if a
 *   memory outage happens on the server, the admin will need to consult the
 *   syslog to find out what went wrong.
 *
 *   I think this is largely cleaned up to the point where it does the right
 *   thing for the server, whether the normal server_active (child process)
 *   case or the error_use_protocol (parent process) case.  The one exception
 *   is that STATUS nonzero for error_use_protocol probably doesn't work yet;
 *   in that case still need to use the pending_error machinery in server.c.
 *
 *   error() does not molest errno; some code (e.g. Entries_Open) depends
 *   on being able to say something like:
 *
 *      error (0, 0, "foo");
 *      error (0, errno, "bar");
 *
 * RETURNS
 *   Sometimes.  ;)
 */
void
error (int status, int errnum, const char *message, ...)
{
    va_list args;
    int save_errno = errno;

    /* Various buffers we attempt to use to generate the error message.  */
    char statbuf[256];
    char *buf;
    size_t length;
    char statbuf2[384];
    char *buf2;
    char statcmdbuf[32];
    char *cmdbuf;
    char *emptybuf = "";

    static const char *last_message = NULL;
    static int last_status;
    static int last_errnum;

    /* Initialize these to avoid a lot of special case error handling.  */
    buf = statbuf;
    buf2 = statbuf2;
    cmdbuf = emptybuf;

    /* Expand the message the user passed us.  */
    length = sizeof (statbuf);
    va_start (args, message);
    buf = vasnprintf (statbuf, &length, message, args);
    va_end (args);
    if (!buf) goto memerror;

    /* Expand the cvs commmand name to <cmd> or [<cmd> aborted].
     *
     * I could squeeze this into the buf2 printf below, but this makes the code
     * easier to read and I don't think error messages are printed often enough
     * to make this a major performance hit.  I think the memory cost is about
     * 40 bytes.
     */
    if (cvs_cmd_name)
    {
	length = sizeof (statcmdbuf);
	cmdbuf = asnprintf (statcmdbuf, &length, " %s%s%s",
			    status ? "[" : "",
			    cvs_cmd_name,
			    status ? " aborted]" : "");
	/* Else cmdbuf still = emptybuf.  */
	if (!cmdbuf) goto memerror;
    }
    /* Else cmdbuf still = emptybuf.  */

    /* Now put it all together.  */
    length = sizeof (statbuf2);
    buf2 = asnprintf (statbuf2, &length, "%s%s: %s%s%s\n",
                      program_name, cmdbuf, buf,
                      errnum ? ": " : "", errnum ? strerror (errnum) : "");
    if (!buf2) goto memerror;

    /* Send the final message to the client or log it.
     *
     * Set this recursion block first since this is the only function called
     * here which can cause error() to be called a second time.
     */
    if (last_message) goto recursion_error;
    last_message = buf2;
    last_status = status;
    last_errnum = errnum;
    cvs_outerr (buf2, length);

    /* Reset our recursion lock.  This needs to be done before the call to
     * exit() to allow the exit handlers to make calls to error().
     */
    last_message = NULL;

    /* Done, if we're exiting.  */
    if (status)
	exit (EXIT_FAILURE);

    /* Free anything we may have allocated.  */
    if (buf != statbuf) free (buf);
    if (buf2 != statbuf2) free (buf2);
    if (cmdbuf != statcmdbuf && cmdbuf != emptybuf) free (cmdbuf);

    /* Restore errno per our charter.  */
    errno = save_errno;

    /* Done.  */
    return;

memerror:
    /* Make one last attempt to log the problem in the syslog since that
     * should not require new memory, then abort.
     *
     * No second attempt is made to send the information to the client - if we
     * got here then that already failed once and this prevents us from
     * entering an infinite loop.
     *
     * FIXME
     *   If the buffer routines can be altered in such a way that a single,
     *   short, statically allocated message could be sent without needing to
     *   allocate new memory, then it would still be safe to call cvs_outerr
     *   with the message here.
     */
#if HAVE_SYSLOG_H
    syslog (LOG_DAEMON | LOG_EMERG, "Memory exhausted.  Aborting.");
#endif /* HAVE_SYSLOG_H */

    goto sidestep_done;

recursion_error:
#if HAVE_SYSLOG_H
    /* Syslog the problem since recursion probably means that we encountered an
     * error while attempting to send the last error message to the client.
     */

    syslog (LOG_DAEMON | LOG_EMERG,
	    "error (%d, %d) called recursively.  Original message was:",
	    last_status, last_errnum);
    syslog (LOG_DAEMON | LOG_EMERG, "%s", last_message);


    syslog (LOG_DAEMON | LOG_EMERG,
            "error (%d, %d) called recursively.  Second message was:",
	    status, errnum);
    syslog (LOG_DAEMON | LOG_EMERG, "%s", buf2);

    syslog (LOG_DAEMON | LOG_EMERG, "Aborting.");
#endif /* HAVE_SYSLOG_H */

sidestep_done:
    /* Reset our recursion lock.  This needs to be done before the call to
     * exit() to allow the exit handlers to make calls to error().
     */
    last_message = NULL;

    exit (EXIT_FAILURE);
}



/* Print the program name and error message MESSAGE, which is a printf-style
   format string with optional args to the file specified by FP.
   If ERRNUM is nonzero, print its corresponding system error message.
   Exit with status EXIT_FAILURE if STATUS is nonzero.  */
/* VARARGS */
void
fperrmsg (FILE *fp, int status, int errnum, char *message, ...)
{
    va_list args;

    fprintf (fp, "%s: ", program_name);
    va_start (args, message);
    vfprintf (fp, message, args);
    va_end (args);
    if (errnum)
	fprintf (fp, ": %s", strerror (errnum));
    putc ('\n', fp);
    fflush (fp);
    if (status)
	exit (EXIT_FAILURE);
}
