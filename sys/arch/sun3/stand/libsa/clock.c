/*	$NetBSD: clock.c,v 1.4 1998/02/05 04:57:06 gwr Exp $	*/


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
