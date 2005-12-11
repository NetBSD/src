/*	$NetBSD: clock.c,v 1.3 2005/12/11 12:19:29 christos Exp $	*/


#include <sys/types.h>
#include <machine/mon.h>

#include "libsa.h"

int hz = 1000;

long 
getsecs(void)
{
	long ticks;

	ticks = getticks();
	return ((ticks / hz));
}

long 
getticks(void)
{
	long ticks;

	ticks = *romVectorPtr->nmiClock;
	return (ticks);
}
