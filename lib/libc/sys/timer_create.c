/*	$NetBSD: timer_create.c,v 1.4 2000/07/14 07:36:32 kleink Exp $	*/

#include <errno.h>
#include <signal.h>
#include <time.h>

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
