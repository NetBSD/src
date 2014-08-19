/*	$NetBSD: timecmp.c,v 1.1.1.1.6.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	timecmp 3
/* SUMMARY
/*	compare two time_t values
/* SYNOPSIS
/*	#include <timecmp.h>
/*
/*	int	timecmp(t1, t2)
/*	time_t	t1;
/*	time_t	t2;
/* DESCRIPTION
/*	The timecmp() function return an integer greater than, equal to, or
/*	less than 0, according as the time t1 is greater than, equal to, or
/*	less than the time t2.  The comparison is made in a manner that is
/*	insensitive to clock wrap-around, provided the underlying times are
/*	within half of the time interval between the smallest and largest
/*	representable time values.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Viktor Dukhovni
/*--*/

#include "timecmp.h"

/* timecmp - wrap-safe time_t comparison */

int     timecmp(time_t t1, time_t t2)
{
    time_t  delta = t1 - t2;

    if (delta == 0)
	return 0;

#define UNSIGNED(type) ( ((type)-1) > ((type)0) )

    /*
     * With a constant switch value, the compiler will emit only the code for
     * the correct case, so the signed/unsigned test happens at compile time.
     */
    switch (UNSIGNED(time_t) ? 0 : 1) {
    case 0:
	return ((2 * delta > delta) ? 1 : -1);
    case 1:
	return ((delta > (time_t) 0) ? 1 : -1);
    }
}

#ifdef TEST
#include <assert.h>

 /*
  * Bit banging!! There is no official constant that defines the INT_MAX
  * equivalent of the off_t type. Wietse came up with the following macro
  * that works as long as off_t is some two's complement number.
  * 
  * Note, however, that C99 permits signed integer representations other than
  * two's complement.
  */
#include <limits.h>
#define __MAXINT__(T) ((T) (((((T) 1) << ((sizeof(T) * CHAR_BIT) - 1)) ^ ((T) -1))))

int     main()
{
    time_t  now = time((time_t *) 0);

    /* Test that it works for normal times */
    assert(timecmp(now + 10, now) > 0);
    assert(timecmp(now, now) == 0);
    assert(timecmp(now - 10, now) < 0);

    /* Test that it works at a boundary time */
    if (UNSIGNED(time_t))
	now = (time_t) -1;
    else
	now = __MAXINT__(time_t);

    assert(timecmp(now + 10, now) > 0);
    assert(timecmp(now, now) == 0);
    assert(timecmp(now - 10, now) < 0);

    return (0);
}

#endif
