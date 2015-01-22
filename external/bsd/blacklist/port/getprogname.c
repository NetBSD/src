#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "port.h"

extern char *__progname;

const char *
getprogname(void)
{
	return __progname;
}
