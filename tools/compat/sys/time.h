/*	$NetBSD: time.h,v 1.1 2010/03/26 07:16:12 cegger Exp $	*/

#if (defined(__APPLE__) && defined(__MACH__)) && !defined(__darwin__)
# define __darwin__     1
#endif

#ifdef __darwin__
#define CLOCK_REALTIME	0	/* XXX Keep this in sync with our sys/time.h */
#endif

#include_next <sys/time.h>
