/*	$NetBSD: clock.c,v 1.3.2.1 1998/01/27 02:35:31 gwr Exp $	*/


#include <sys/types.h>
#include <machine/mon.h>

#include "libsa.h"

int hz = 1000;

long
getsecs()
{
	long ticks;

	ticks = getticks();
	return ((ticks / hz));
}

long
getticks()
{
	long ticks;

	ticks = *romVectorPtr->nmiClock;
	return (ticks);
}
