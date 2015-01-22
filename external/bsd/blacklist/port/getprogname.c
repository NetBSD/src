#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern char *__progname;

const char *
getprogname(void)
{
	return __progname;
}

void setprogname(char *p) {
	__progname = p;
}
