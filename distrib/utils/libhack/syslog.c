#include <sys/types.h>
#include <sys/syslog.h>
#include <stdio.h>
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
}

__strong_alias(_syslog, syslog)
void
syslog(int fac, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	fflush(stderr);
}

__strong_alias(_vsyslog, vsyslog)
void
vsyslog(int fac, const char *fmt, va_list ap)
{
	(void)vfprintf(stderr, fmt, ap);
	(void)fprintf(stderr, "\n");
	fflush(stderr);
}
