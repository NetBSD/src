/*	$NetBSD: clock.c,v 1.2 2005/01/22 15:36:11 chs Exp $	*/


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
