/*	$NetBSD: timer_getoverrun.c,v 1.4 2000/07/14 07:36:32 kleink Exp $	*/

#include <errno.h>
#include <signal.h>
#include <time.h>

/* ARGSUSED */
int
timer_getoverrun(timerid)
	timer_t timerid;
{

	errno = ENOSYS;
	return -1;
}
