/*	$NetBSD: timer_settime.c,v 1.4 2000/07/14 07:36:32 kleink Exp $	*/

#include <errno.h>
#include <signal.h>
#include <time.h>

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
