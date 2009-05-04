/*	$NetBSD: clock.c,v 1.3.78.1 2009/05/04 08:12:02 yamt Exp $	*/


#include <sys/types.h>
#include <machine/mon.h>

#include "libsa.h"
#include "net.h"

int hz = 1000;

satime_t 
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
