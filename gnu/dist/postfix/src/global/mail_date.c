/*++
/* NAME
/*	mail_date 3
/* SUMMARY
/*	return formatted time
/* SYNOPSIS
/*	#include <mail_date.h>
/*
/*	const char *mail_date(when)
/*	time_t	when;
/* DESCRIPTION
/*	mail_date() converts the time specified in \fIwhen\fR to the
/*	form: "Mon, 9 Dec 1996 05:38:26 -0500 (EST)" and returns
/*	a pointer to the result. The result is overwritten upon
/*	each call.
/* DIAGNOSTICS
/*	Panic: the offset from UTC is more than a whole day. Fatal
/*	error: out of memory.
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
#include <time.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include "mail_date.h"

 /*
  * Application-specific.
  */
#define DAY_MIN		(24 * HOUR_MIN)	/* minutes in a day */
#define	HOUR_MIN	60		/* minutes in an hour */
#define MIN_SEC		60		/* seconds in a minute */

/* mail_date - return formatted time */

const char *mail_date(time_t when)
{
    static VSTRING *vp;
    struct tm *lt;
    struct tm gmt;
    int     gmtoff;

    /*
     * As if strftime() isn't expensive enough, we're dynamically adjusting
     * the size for the result, so we won't be surprised by long names etc.
     */
    if (vp == 0)
	vp = vstring_alloc(100);
    else
	VSTRING_RESET(vp);

    /*
     * POSIX does not require that struct tm has a tm_gmtoff field, so we
     * must compute the time offset from UTC by hand.
     * 
     * Starting with the difference in hours/minutes between 24-hour clocks,
     * adjust for differences in years, in yeardays, and in (leap) seconds.
     * 
     * Assume 0..23 hours in a day, 0..59 minutes in an hour. The library spec
     * has changed: we can no longer assume that there are 0..59 seconds in a
     * minute.
     */
    gmt = *gmtime(&when);
    lt = localtime(&when);
    gmtoff = (lt->tm_hour - gmt.tm_hour) * HOUR_MIN + lt->tm_min - gmt.tm_min;
    if (lt->tm_year < gmt.tm_year)
	gmtoff -= DAY_MIN;
    else if (lt->tm_year > gmt.tm_year)
	gmtoff += DAY_MIN;
    else if (lt->tm_yday < gmt.tm_yday)
	gmtoff -= DAY_MIN;
    else if (lt->tm_yday > gmt.tm_yday)
	gmtoff += DAY_MIN;
    if (lt->tm_sec <= gmt.tm_sec - MIN_SEC)
	gmtoff -= 1;
    else if (lt->tm_sec >= gmt.tm_sec + MIN_SEC)
	gmtoff += 1;

    /*
     * First, format the date and wall-clock time. XXX The %e format (day of
     * month, leading zero replaced by blank) isn't in my POSIX book, but
     * many vendors seem to support it.
     */
#ifdef MISSING_STRFTIME_E
#define STRFTIME_FMT "%a, %d %b %Y %H:%M:%S "
#else
#define STRFTIME_FMT "%a, %e %b %Y %H:%M:%S "
#endif

    while (strftime(vstring_end(vp), vstring_avail(vp), STRFTIME_FMT, lt) == 0)
	VSTRING_SPACE(vp, 100);
    VSTRING_SKIP(vp);

    /*
     * Then, add the UTC offset.
     */
    if (gmtoff < -DAY_MIN || gmtoff > DAY_MIN)
	msg_panic("UTC time offset %d is larger than one day", gmtoff);
    vstring_sprintf_append(vp, "%+03d%02d", (int) (gmtoff / HOUR_MIN),
			   (int) (abs(gmtoff) % HOUR_MIN));

    /*
     * Finally, add the time zone name.
     */
    while (strftime(vstring_end(vp), vstring_avail(vp), " (%Z)", lt) == 0)
	VSTRING_SPACE(vp, vstring_avail(vp) + 100);
    VSTRING_SKIP(vp);

    return (vstring_str(vp));
}

#ifdef TEST

#include <vstream.h>

int     main(void)
{
    vstream_printf("%s\n", mail_date(time((time_t *) 0)));
    vstream_fflush(VSTREAM_OUT);
    return (0);
}

#endif
