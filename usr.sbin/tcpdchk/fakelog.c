/*	$NetBSD: fakelog.c,v 1.4 2000/12/30 21:45:44 martin Exp $	*/

 /*
  * This module intercepts syslog() library calls and redirects their output
  * to the standard output stream. For interactive testing.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#) fakelog.c 1.3 94/12/28 17:42:21";
#else
__RCSID("$NetBSD: fakelog.c,v 1.4 2000/12/30 21:45:44 martin Exp $");
#endif
#endif

#include <stdio.h>
#include <syslog.h>

#include "mystdarg.h"
#include "percent_m.h"

/* openlog - dummy */

/* ARGSUSED */

void
openlog(name, logopt, facility)
const char   *name;
int     logopt;
int     facility;
{
    /* void */
}

/* vsyslog - format one record */

void
vsyslog(severity, fmt, ap)
int     severity;
const char   *fmt;
_BSD_VA_LIST_ ap;
{
    char    buf[BUFSIZ];

    vprintf(percent_m(buf, fmt), ap);
    printf("\n");
    fflush(stdout);
}

/* syslog - format one record */

/* VARARGS */

void
#ifdef __STDC__
syslog(int severity, const char *fmt, ...)
#else
syslog(va_alist)
	va_dcl
#endif
{
    va_list ap;
#ifndef __STDC__
    int severity;
    char   *fmt;

    va_start(ap);
    severity = va_arg(ap, int);
    fmt = va_arg(ap, char *);
#else
    va_start(ap, fmt);
#endif
    vsyslog(severity, fmt, ap);
    va_end(ap);
}

/* closelog - dummy */

void
closelog()
{
    /* void */
}
