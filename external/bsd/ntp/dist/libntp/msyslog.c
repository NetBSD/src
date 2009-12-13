/*	$NetBSD: msyslog.c,v 1.1.1.1 2009/12/13 16:55:04 kardel Exp $	*/

/*
 * msyslog - either send a message to the terminal or print it on
 *	     the standard output.
 *
 * Converted to use varargs, much better ... jks
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdio.h>

#include "ntp_types.h"
#include "ntp_string.h"
#include "ntp_syslog.h"
#include "ntp_stdlib.h"

#ifdef SYS_WINNT
# include <stdarg.h>
# include "..\ports\winnt\libntp\messages.h"
#endif

int syslogit = 1;

FILE *syslog_file = NULL;

u_long ntp_syslogmask =  ~ (u_long) 0;

#ifdef SYS_WINNT
static char separator = '\\';
#else
static char separator = '/';
#endif /* SYS_WINNT */
extern	char *progname;

/* Declare the local functions */
void	addto_syslog	(int, char *);
void	format_errmsg   (char *, int, const char *, int);


/*
 * This routine adds the contents of a buffer to the log
 */
void
addto_syslog(int level, char * buf)
{
	char *prog;
	FILE *out_file = syslog_file;

#if !defined(VMS) && !defined (SYS_VXWORKS)
	if (syslogit)
	    syslog(level, "%s", buf);
	else
#endif /* VMS  && SYS_VXWORKS*/
	{
		out_file = syslog_file ? syslog_file: level <= LOG_ERR ? stderr : stdout;
		/* syslog() provides the timestamp, so if we're not using
		   syslog, we must provide it. */
		prog = strrchr(progname, separator);
		if (prog == NULL)
		    prog = progname;
		else
		    prog++;
		(void) fprintf(out_file, "%s ", humanlogtime ());
		(void) fprintf(out_file, "%s[%d]: %s", prog, (int)getpid(), buf);
		fflush (out_file);
	}
#if DEBUG
	if (debug && out_file != stdout && out_file != stderr)
		printf("addto_syslog: %s\n", buf);
#endif
}
void
format_errmsg(char *nfmt, int lennfmt, const char *fmt, int errval)
{
	register char c;
	register char *n;
	register const char *f;
	size_t len;
	char *err;

	n = nfmt;
	f = fmt;
	while ((c = *f++) != '\0' && n < (nfmt + lennfmt - 2)) {
		if (c != '%') {
			*n++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*n++ = '%';
			*n++ = c;
			continue;
		}
		err = strerror(errval);
		len = strlen(err);

		/* Make sure we have enough space for the error message */
		if ((n + len) < (nfmt + lennfmt - 2)) {
			memcpy(n, err, len);
			n += len;
		}
	}
#if !defined(VMS)
	if (!syslogit)
#endif /* VMS */
	    *n++ = '\n';
	*n = '\0';
}

#if defined(__STDC__) || defined(HAVE_STDARG_H)
void msyslog(int level, const char *fmt, ...)
#else /* defined(__STDC__) || defined(HAVE_STDARG_H) */
     /*VARARGS*/
     void msyslog(va_alist)
     va_dcl
#endif /* defined(__STDC__) || defined(HAVE_STDARG_H) */
{
#if defined(__STDC__) || defined(HAVE_STDARG_H)
#else
	int level;
	const char *fmt;
#endif
	va_list ap;
	char buf[1025], nfmt[256];
	int errval;

	/*
	 * Save the error value as soon as possible
	 */
	errval = errno;

#ifdef SYS_WINNT
	errval = GetLastError();
	if (NO_ERROR == errval)
		errval = errno;
#endif /* SYS_WINNT */

#if defined(__STDC__) || defined(HAVE_STDARG_H)
	va_start(ap, fmt);
#else
	va_start(ap);

	level = va_arg(ap, int);
	fmt = va_arg(ap, char *);
#endif
	format_errmsg(nfmt, sizeof(nfmt), fmt, errval);

	vsnprintf(buf, sizeof(buf), nfmt, ap);
	addto_syslog(level, buf);
	va_end(ap);
}
