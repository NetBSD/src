/*	$NetBSD: fakelog.c,v 1.5 2002/07/06 21:46:59 wiz Exp $	*/

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
__RCSID("$NetBSD: fakelog.c,v 1.5 2002/07/06 21:46:59 wiz Exp $");
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
