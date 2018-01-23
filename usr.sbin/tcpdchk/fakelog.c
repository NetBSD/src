/*	$NetBSD: fakelog.c,v 1.7 2018/01/23 21:06:26 sevan Exp $	*/

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
__RCSID("$NetBSD: fakelog.c,v 1.7 2018/01/23 21:06:26 sevan Exp $");
#endif
#endif

#include <stdio.h>
#include <syslog.h>

#include "mystdarg.h"
#include "percent_m.h"

/* openlog - dummy */

/* ARGSUSED */

void
openlog(const char *name, int logopt, int facility)
{
    /* void */
}

/* vsyslog - format one record */

void
vsyslog(int severity, const char *fmt, va_list ap)
{
    char    buf[BUFSIZ];

    vprintf(percent_m(buf, fmt), ap);
    printf("\n");
    fflush(stdout);
}

/* syslog - format one record */

/* VARARGS */

void
syslog(int severity, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsyslog(severity, fmt, ap);
    va_end(ap);
}

/* closelog - dummy */

void
closelog()
{
    /* void */
}
