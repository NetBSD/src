/*	$NetBSD: clock.c,v 1.1.24.1 2005/01/24 08:35:03 skrll Exp $	*/


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
