/*	$NetBSD: timer_settime.c,v 1.2 1998/01/09 03:15:45 perry Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

int
timer_settime(timerid, flags, value, ovalue)
	timer_t timerid;
	int flags;
	const struct itimerspec *value;
	struct itimerspec *ovalue;
{
	errno = ENOSYS;
	return -1;
}
