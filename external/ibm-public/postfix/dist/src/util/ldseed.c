/*	$NetBSD: ldseed.c,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	ldseed 3
/* SUMMARY
/*	seed for non-cryptographic applications
/* SYNOPSIS
/*	#include <ldseed.h>
/*
/*	void	ldseed(
/*	void	*dst,
/*	size_t	len)
/* DESCRIPTION
/*	ldseed() preferably extracts pseudo-random bits from
/*	/dev/urandom, a non-blocking device that is available on
/*	modern systems.
/*
/*	On systems where /dev/urandom is unavailable or does not
/*	immediately return the requested amount of randomness,
/*	ldseed() falls back to a combination of wallclock time,
/*	the time since boot, and the process ID.
/* BUGS
/*	With Linux "the O_NONBLOCK flag has no effect when opening
/*	/dev/urandom", but reads "can incur an appreciable delay
/*	when requesting large amounts of data". Apparently, "large"
/*	means more than 256 bytes.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library
  */
#include <sys_defs.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>			/* CHAR_BIT */

 /*
  * Utility library.
  */
#include <iostuff.h>
#include <msg.h>
#include <ldseed.h>

 /*
  * Different systems have different names for non-wallclock time.
  */
#ifdef CLOCK_UPTIME
#define NON_WALLTIME_CLOCK      CLOCK_UPTIME
#elif defined(CLOCK_BOOTTIME)
#define NON_WALLTIME_CLOCK      CLOCK_BOOTTIME
#elif defined(CLOCK_MONOTONIC)
#define NON_WALLTIME_CLOCK      CLOCK_MONOTONIC
#elif defined(CLOCK_HIGHRES)
#define NON_WALLTIME_CLOCK      CLOCK_HIGHRES
#endif

/* ldseed - best-effort, low-dependency seed */

void    ldseed(void *dst, size_t len)
{
    int     count;
    int     fd;
    int     n;
    time_t  fallback = 0;

    /*
     * Medium-quality seed.
     */
    if ((fd = open("/dev/urandom", O_RDONLY)) > 0) {
	non_blocking(fd, NON_BLOCKING);
	count = read(fd, dst, len);
	(void) close(fd);
	if (count == len)
	    return;
    }

    /*
     * Low-quality seed. Based on 1) the time since boot (good when an
     * attacker knows the program start time but not the system boot time),
     * and 2) absolute time (good when an attacker does not know the program
     * start time). Assumes a system with better than microsecond resolution,
     * and a network stack that does not leak the time since boot, for
     * example, through TCP or ICMP timestamps. With those caveats, this seed
     * is good for 20-30 bits of randomness.
     */
#ifdef NON_WALLTIME_CLOCK
    {
	struct timespec ts;

	if (clock_gettime(NON_WALLTIME_CLOCK, &ts) != 0)
	    msg_fatal("clock_gettime() failed: %m");
	fallback += ts.tv_sec ^ ts.tv_nsec;
    }
#elif defined(USE_GETHRTIME)
    fallback += gethrtime();
#endif

#ifdef CLOCK_REALTIME
    {
	struct timespec ts;

	if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
	    msg_fatal("clock_gettime() failed: %m");
	fallback += ts.tv_sec ^ ts.tv_nsec;
    }
#else
    {
	struct timeval tv;

	if (GETTIMEOFDAY(&tv) != 0)
	    msg_fatal("gettimeofday() failed: %m");
	fallback += tv.tv_sec + tv.tv_usec;
    }
#endif
    fallback += getpid();

    /*
     * Copy the least significant bytes first, because those are the most
     * volatile.
     */
    for (n = 0; n < sizeof(fallback) && n < len; n++) {
	*(char *) dst++ ^= (fallback & 0xff);
	fallback >>= CHAR_BIT;
    }
    return;
}
