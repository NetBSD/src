
#include <sys/types.h>
#include <machine/mon.h>

#include "clock.h"

int hz = 1000;

time_t getsecs()
{
	register int ticks = getticks();
	return ((time_t)(ticks / hz));
}

int getticks()
{
	register int ticks;

	ticks = *romVectorPtr->nmiClock;
	return (ticks);
}
