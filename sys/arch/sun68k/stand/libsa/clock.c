/*	$NetBSD: clock.c,v 1.1 2001/06/14 12:57:14 fredette Exp $	*/


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
