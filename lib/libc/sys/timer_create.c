/*	$NetBSD: timer_create.c,v 1.2 1998/01/09 03:15:42 perry Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

int
timer_create(clock_id, evp, timerid)
	clockid_t clock_id;
	struct sigevent *evp;
	timer_t *timerid;
{
	errno = ENOSYS;
	return -1;
}
