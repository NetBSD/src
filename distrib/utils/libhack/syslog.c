#include "namespace.h"
#include <sys/types.h>
#include <sys/syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "extern.h"

#ifdef __weak_alias
__weak_alias(closelog,_closelog)
__weak_alias(openlog,_openlog)
__weak_alias(setlogmask,_setlogmask)
__weak_alias(syslog,_syslog)
__weak_alias(vsyslog,_vsyslog)
__weak_alias(syslogp,_syslogp)
__weak_alias(vsyslogp,_vsyslogp)
#endif

void
openlog(const char *path, int opt, int fac)
{
}

void
closelog(void)
{
}

int
setlogmask(int mask)
{
	return 0xff;
}

void
syslog(int fac, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(fac, fmt, ap);
	va_end(ap);
}

void
vsyslog(int fac, const char *fmt, va_list ap)
{
	(void)vfprintf(stderr, fmt, ap);
	/* Cheap hack to ensure %m causes error message string to be shown */
	if (strstr(fmt, "%m"))
		(void)fprintf(stderr, " (%s)", strerror(errno));
	(void)fprintf(stderr, "\n");
	fflush(stderr);
}

void
syslog_ss(int priority, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(priority, fmt, ap);
	va_end(ap);
}

void
vsyslog_ss(int priority, struct syslog_data *data, const char *fmt, va_list ap)
{
	vsyslog(priority, fmt, ap);
}

void
syslog_r(int priority, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(priority, fmt, ap);
	va_end(ap);
}

void
vsyslog_r(int priority, struct syslog_data *data, const char *fmt, va_list ap)
{
	vsyslog(priority, fmt, ap);
}

void
closelog_r(struct syslog_data *data)
{
}

int
setlogmask_r(int maskpri, struct syslog_data *data)
{
	return 0xff;
}

void
openlog_r(const char *id, int logopt, int facility, struct syslog_data *data)
{
}

void
syslogp_r(int priority, struct syslog_data *data, const char *msgid,
    const char *sdfmt, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(priority, fmt, ap);
	va_end(ap);
}

void
vsyslogp_r(int priority, struct syslog_data *data, const char *msgid,
    const char *sdfmt, const char *fmt, va_list ap)
{
	vsyslog(priority, fmt, ap);
}

void
syslogp_ss(int priority, struct syslog_data *data, const char *msgid,
    const char *sdfmt, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(priority, fmt, ap);
	va_end(ap);
}

void
vsyslogp_ss(int priority, struct syslog_data *data, const char *msgid,
    const char *sdfmt, const char *fmt, va_list ap)
{
	vsyslog(priority, fmt, ap);
}

void
syslogp(int priority, const char *msgid, const char *sdfmt, const char *fmt,
    ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(priority, fmt, ap);
	va_end(ap);
}

void
vsyslogp(int priority, const char *msgid, const char *sdfmt, const char *fmt,
    va_list ap)
{
	vsyslog(priority, fmt, ap);
}
