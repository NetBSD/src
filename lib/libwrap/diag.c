/*	$NetBSD: diag.c,v 1.7 2001/09/24 17:55:47 atatat Exp $	*/

 /*
  * Routines to report various classes of problems. Each report is decorated
  * with the current context (file name and line number), if available.
  * 
  * tcpd_warn() reports a problem and proceeds.
  * 
  * tcpd_jump() reports a problem and jumps.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#) diag.c 1.1 94/12/28 17:42:20";
#else
__RCSID("$NetBSD: diag.c,v 1.7 2001/09/24 17:55:47 atatat Exp $");
#endif
#endif

/* System libraries */

#include <syslog.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>

/* Local stuff */

#include "tcpd.h"
#include "mystdarg.h"

struct tcpd_context tcpd_context;
jmp_buf tcpd_buf;

static void tcpd_diag __P((int, char *, char *, va_list))
	__attribute__((__format__(__printf__, 3, 0)));

/* tcpd_diag - centralize error reporter */

static void tcpd_diag(severity, tag, format, ap)
int     severity;
char   *tag;
char   *format;
va_list ap;
{
    char    fmt[BUFSIZ];
    char    buf[BUFSIZ];
    int     i, o, oerrno;

    /* save errno in case we need it */
    oerrno = errno;

    /* contruct the tag for the log entry */
    if (tcpd_context.file)
	(void)snprintf(buf, sizeof buf, "%s: %s, line %d: ",
		tag, tcpd_context.file, tcpd_context.line);
    else
	(void)snprintf(buf, sizeof buf, "%s: ", tag);

    /* change % to %% in tag before appending the format */
    for (i = 0, o = 0; buf[i] != '\0'; ) {
	if (buf[i] == '%') {
	    fmt[o] = '%';
	    if (o < sizeof(fmt) - 1)
		o++;
	}
	fmt[o] = buf[i++];
	if (o < sizeof(fmt) - 1)
	    o++;
    }

    /* append format and force null termination */
    fmt[o] = '\0';
    strncat(fmt, format, sizeof(fmt) - o);
    fmt[sizeof(fmt) - 1] = '\0';

    errno = oerrno;
    vsyslog(severity, fmt, ap);
}

/* tcpd_warn - report problem of some sort and proceed */

void    VARARGS(tcpd_warn, char *, format)
{
    va_list ap;

    VASTART(ap, char *, format);
    tcpd_diag(LOG_ERR, "warning", format, ap);
    VAEND(ap);
}

/* tcpd_jump - report serious problem and jump */

void    VARARGS(tcpd_jump, char *, format)
{
    va_list ap;

    VASTART(ap, char *, format);
    tcpd_diag(LOG_ERR, "error", format, ap);
    VAEND(ap);
    longjmp(tcpd_buf, AC_ERROR);
}
