/*	$NetBSD: timer_gettime.c,v 1.3 1998/11/15 17:23:00 christos Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

/* ARGSUSED */
int
timer_gettime(timerid, value)
	timer_t timerid;
	struct itimerspec *value;
{
	errno = ENOSYS;
	return -1;
}
