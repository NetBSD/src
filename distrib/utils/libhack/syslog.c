#include <sys/types.h>
#include <sys/syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

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

__strong_alias(_syslog, syslog)
void
syslog(int fac, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(fac, fmt, ap);
	va_end(ap);
}

__strong_alias(_vsyslog, vsyslog)
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

void syslog_ss(int, struct syslog_data *, const char *, ...);
__strong_alias(_syslog_ss, syslog_ss)
void
syslog_ss(int priority, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(priority, fmt, ap);
	va_end(ap);
}

void vsyslog_ss(int, struct syslog_data *, const char *, va_list);
__strong_alias(_vsyslog_ss, vsyslog_ss)
void
vsyslog_ss(int priority, struct syslog_data *data, const char *fmt, va_list ap)
{
	vsyslog(priority, fmt, ap);
}

__strong_alias(_syslog_r, syslog_r)
void
syslog_r(int priority, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsyslog(priority, fmt, ap);
	va_end(ap);
}

__strong_alias(_vsyslog_r, vsyslog_r)
void
vsyslog_r(int priority, struct syslog_data *data, const char *fmt, va_list ap)
{
	vsyslog(priority, fmt, ap);
}

__strong_alias(_closelog_r, closelog_r)
void
closelog_r(struct syslog_data *data)
{
}

__strong_alias(_setlogmask_r, setlogmask_r)
int
setlogmask_r(int maskpri, struct syslog_data *data)
{
	return 0xff;
}

__strong_alias(_openlog_r, openlog_r)
void
openlog_r(const char *id, int logopt, int facility, struct syslog_data *data)
{
}
