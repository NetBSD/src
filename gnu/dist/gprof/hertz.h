#ifndef hertz_h
#define hertz_h

#include "gprof.h"

#define	HZ_WRONG 0		/* impossible clock frequency */

/*
 * Discover the tick frequency of the machine if something goes wrong,
 * we return HZ_WRONG, an impossible sampling frequency.
 */

/* FIXME: Checking for MACH here makes no sense when for a cross
   gprof.  */
#ifdef MACH
#include <machine/mach_param.h>
#define hertz() (HZ)
#else
extern int hertz PARAMS ((void));
#endif

#endif /* hertz_h */
