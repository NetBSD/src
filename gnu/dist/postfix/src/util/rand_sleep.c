/*++
/* NAME
/*	rand_sleep 3
/* SUMMARY
/*	sleep for randomized interval
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	void	rand_sleep(delay, variation)
/*	unsigned delay;
/*	unsigned variation;
/* DESCRIPTION
/*	rand_sleep() blocks the current process for an amount of time
/*	pseudo-randomly chosen from the interval (delay +- variation/2).
/*
/*	Arguments:
/* .IP delay
/*	Time to sleep in microseconds.
/* .IP variation
/*	Variation in microseconds; must not be larger than delay.
/* DIAGNOSTICS
/*	Panic: interface violation. All system call errors are fatal.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <myrand.h>
#include <iostuff.h>

/* rand_sleep - block for random time */

void    rand_sleep(unsigned delay, unsigned variation)
{
    char   *myname = "rand_sleep";
    unsigned usec;

    /*
     * Sanity checks.
     */
    if (delay == 0)
	msg_panic("%s: bad delay %d", myname, delay);
    if (variation > delay)
	msg_panic("%s: bad variation %d", myname, variation);

    /*
     * Use the semi-crappy random number generator.
     */
    usec = (delay - variation / 2) + variation * (double) myrand() / RAND_MAX;
    doze(usec);
}

#ifdef TEST

#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    int     delay;
    int     variation;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc != 3)
	msg_fatal("usage: %s delay variation", argv[0]);
    if ((delay = atoi(argv[1])) <= 0)
	msg_fatal("bad delay: %s", argv[1]);
    if ((variation = atoi(argv[2])) < 0)
	msg_fatal("bad variation: %s", argv[2]);
    rand_sleep(delay * 1000000, variation * 1000000);
    exit(0);
}

#endif
