/*	$NetBSD: timer_gettime.c,v 1.2 1998/01/09 03:15:44 perry Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

int
timer_gettime(timerid, value)
	timer_t timerid;
	struct itimerspec *value;
{
	errno = ENOSYS;
	return -1;
}
