/*	$NetBSD: timer_gettime.c,v 1.4 2000/07/14 07:36:32 kleink Exp $	*/

#include <errno.h>
#include <signal.h>
#include <time.h>

/* ARGSUSED */
int
timer_gettime(timerid, value)
	timer_t timerid;
	struct itimerspec *value;
{

	errno = ENOSYS;
	return -1;
}
