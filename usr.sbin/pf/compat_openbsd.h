/*	$NetBSD: compat_openbsd.h,v 1.1 2004/11/16 05:14:12 yamt Exp $	*/

/*
 * misc OpenBSD compatibility hacks.
 */

#include <sys/cdefs.h>

/*
 * syslog.h
 */

#include <stdarg.h>
#include <syslog.h>

#define	openlog_r(ident, logopt, fac, data)	/* nothing */

struct syslog_data {
#if !defined(__GNUC__)
	/* empty structure is gcc extension */
	char dummy;
#endif
};
#define	SYSLOG_DATA_INIT	{}

static __inline void syslog_r(int pri, struct syslog_data *data,
    const char *fmt, ...) __unused;

static __inline void
syslog_r(int pri, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

/*
 * sys/queue.h
 */

#define	TAILQ_END(h)	NULL
