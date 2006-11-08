#include <stdio.h>
#include <stdarg.h>

static void wrap(char *str, const char *, ...);

static void
wrap(char *str, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)vsprintf(str, fmt, ap);
	va_end(ap);
}

int
main(int argc, char *argv[])
{
	char b[10];
	wrap(b, "%s", argv[1]);
	(void)printf("%s\n", b);
	return 0;
}
