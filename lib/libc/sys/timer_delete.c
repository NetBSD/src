/*	$NetBSD: timer_delete.c,v 1.3 1998/11/15 17:23:00 christos Exp $	*/

#include <signal.h>
#include <time.h>
#include <errno.h>

/* ARGSUSED */
int
timer_delete(timerid)
	timer_t timerid;
{
	errno = ENOSYS;
	return -1;
}
