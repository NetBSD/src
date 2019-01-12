/*	$NetBSD: diag.c,v 1.17 2019/01/12 19:08:24 christos Exp $	*/

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
__RCSID("$NetBSD: diag.c,v 1.17 2019/01/12 19:08:24 christos Exp $");
#endif
#endif

/* System libraries */

#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>

/* Local stuff */

#include "tcpd.h"
#include "expandm.h"

struct tcpd_context tcpd_context;
jmp_buf tcpd_buf;

static void tcpd_diag(int, const char *, const char *, va_list)
    __sysloglike(3,0);

/* tcpd_diag - centralize error reporter */

static void
tcpd_diag(int severity, const char *tag, const char *fmt, va_list ap)
{
    char *buf, *bufx;

    if (vasprintf(&buf, expandm(fmt, NULL, &bufx), ap) == -1)
	buf = __UNCONST(fmt);
    free(bufx);

    /* contruct the tag for the log entry */
    if (tcpd_context.file)
	syslog(severity, "%s: %s, line %d: %s",
	    tag, tcpd_context.file, tcpd_context.line, buf);
    else
	syslog(severity, "%s: %s", tag, buf);

    if (buf != fmt)
	free(buf);
}

/* tcpd_warn - report problem of some sort and proceed */

void
tcpd_warn(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    tcpd_diag(LOG_ERR, "warning", format, ap);
    va_end(ap);
}

/* tcpd_jump - report serious problem and jump */

void
tcpd_jump(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    tcpd_diag(LOG_ERR, "error", format, ap);
    va_end(ap);
    longjmp(tcpd_buf, AC_ERROR);
}
