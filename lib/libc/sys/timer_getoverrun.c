/*	$NetBSD: timer_getoverrun.c,v 1.2 1998/01/09 03:15:43 perry Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

int
timer_getoverrun(timerid)
	timer_t timerid;
{
	errno = ENOSYS;
	return -1;
}
