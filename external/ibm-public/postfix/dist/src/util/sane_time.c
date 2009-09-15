/*	$NetBSD: sane_time.c,v 1.1.1.1.2.2 2009/09/15 06:04:03 snj Exp $	*/

/*++
/* NAME
/*	sane_time 3
/* SUMMARY
/*	time(2) with backward time jump protection.
/* SYNOPSIS
/*	#include <sane_time.h>
/*
/*	time_t sane_time(void)
/*
/* DESCRIPTION
/*	This module provides time(2) like call for applications
/*	which need monotonically increasing time function rather
/*	than the real exact time. It eliminates the need for various
/*	workarounds all over the application which would handle
/*	potential problems if time suddenly jumps backward.
/*	Instead we choose to deal with this problem inside this
/*	module and let the application focus on its own tasks.
/*
/*	sane_time() returns the current timestamp as obtained from
/*	time(2) call, at least most of the time. In case this routine
/*	detects that time has jumped backward, it keeps returning
/*	whatever timestamp it returned before, until this timestamp
/*	and the time(2) timestamp become synchronized again.
/*	Additionally, the returned timestamp is slowly increased to
/*	prevent the faked clock from freezing for too long.
/* SEE ALSO
/*	time(2) get current time
/* DIAGNOSTICS
/*	Warning message is logged if backward time jump is detected.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Patrik Rak
/*	Modra 6
/*	155 00, Prague, Czech Republic
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>

/* Application-specific. */

#include "sane_time.h"

/*
 * How many times shall we slow down the real clock when recovering from
 * time jump.
 */
#define SLEW_FACTOR 2

/* sane_time - get current time, protected against time warping */

time_t  sane_time(void)
{
    time_t  now;
    static time_t last_time,
            last_real;
    int     delta;
    static int fraction;
    static int warned;

    now = time((time_t *) 0);

    if ((delta = now - last_time) < 0 && last_time != 0) {
	if ((delta = now - last_real) < 0) {
	    msg_warn("%sbackward time jump detected -- slewing clock",
		     warned++ ? "another " : "");
	} else {
	    delta += fraction;
	    last_time += delta / SLEW_FACTOR;
	    fraction = delta % SLEW_FACTOR;
	}
    } else {
	if (warned) {
	    warned = 0;
	    msg_warn("backward time jump recovered -- back to normality");
	    fraction = 0;
	}
	last_time = now;
    }
    last_real = now;

    return (last_time);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Repeatedly print current system time and
  * time returned by sane_time(). Meanwhile, try stepping your system clock
  * back and forth to see what happens.
  */

#include <stdlib.h>
#include <msg_vstream.h>
#include <iostuff.h>			/* doze() */

int     main(int argc, char **argv)
{
    int     delay = 1000000;
    time_t  now;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    if (argc == 2 && (delay = atol(argv[1]) * 1000) > 0)
	 /* void */ ;
    else if (argc != 1)
	msg_fatal("usage: %s [delay in ms (default 1 second)]", argv[0]);

    for (;;) {
	now = time((time_t *) 0);
	vstream_printf("real: %s", ctime(&now));
	now = sane_time();
	vstream_printf("fake: %s\n", ctime(&now));
	vstream_fflush(VSTREAM_OUT);
	doze(delay);
    }
}

#endif
