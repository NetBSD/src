/*	$NetBSD: timer_settime.c,v 1.3 1998/11/15 17:23:00 christos Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

/* ARGSUSED */
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
