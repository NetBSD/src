#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static void wrap(char *str, size_t, const char *, ...);

static void
wrap(char *str, size_t len, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)vsnprintf(str, len, fmt, ap);
	va_end(ap);
}

int
main(int argc, char *argv[])
{
	char b[10];
	size_t len = atoi(argv[1]);
	wrap(b, len, "%s", "012345678910");
	(void)printf("%s\n", b);
	return 0;
}
