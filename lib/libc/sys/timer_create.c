/*	$NetBSD: timer_create.c,v 1.3 1998/11/15 17:23:00 christos Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

/* ARGSUSED */
int
timer_create(clock_id, evp, timerid)
	clockid_t clock_id;
	struct sigevent *evp;
	timer_t *timerid;
{
	errno = ENOSYS;
	return -1;
}
