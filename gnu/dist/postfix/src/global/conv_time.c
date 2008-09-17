/*++
/* NAME
/*	conv_time 3
/* SUMMARY
/*	time value conversion
/* SYNOPSIS
/*	#include <conv_time.h>
/*
/*	int	conv_time(strval, timval, def_unit);
/*	const char *strval;
/*	int	*timval;
/*	int	def_unit;
/* DESCRIPTION
/*	conv_time() converts a numerical time value with optional
/*	one-letter suffix that specifies an explicit time unit: s
/*	(seconds), m (minutes), h (hours), d (days) or w (weeks).
/*	Internally, time is represented in seconds.
/*
/*	Arguments:
/* .IP strval
/*	Input value.
/* .IP timval
/*	Result pointer.
/* .IP def_unit
/*	The default time unit suffix character.
/* DIAGNOSTICS
/*	The result value is non-zero in case of success, zero in
/*	case of a bad time value or a bad time unit suffix.
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
#include <limits.h>			/* INT_MAX */
#include <stdio.h>			/* sscanf() */

/* Utility library. */

#include <msg.h>

/* Global library. */

#include <conv_time.h>

#define MINUTE	(60)
#define HOUR	(60 * MINUTE)
#define DAY	(24 * HOUR)
#define WEEK	(7 * DAY)

/* conv_time - convert time value */

int     conv_time(const char *strval, int *timval, int def_unit)
{
    char    unit;
    char    junk;
    int     intval;

    switch (sscanf(strval, "%d%c%c", &intval, &unit, &junk)) {
    case 1:
	unit = def_unit;
	/* FALLTHROUGH */
    case 2:
	if (intval < 0)
	    return (0);
	switch (unit) {
	case 'w':
	    if (intval < INT_MAX / WEEK) {
		*timval = intval * WEEK;
		return (1);
	    } else {
		return (0);
	    }
	case 'd':
	    if (intval < INT_MAX / DAY) {
		*timval = intval * DAY;
		return (1);
	    } else {
		return (0);
	    }
	case 'h':
	    if (intval < INT_MAX / HOUR) {
		*timval = intval * HOUR;
		return (1);
	    } else {
		return (0);
	    }
	case 'm':
	    if (intval < INT_MAX / MINUTE) {
		*timval = intval * MINUTE;
		return (1);
	    } else {
		return (0);
	    }
	case 's':
	    *timval = intval;
	    return (1);
	}
    }
    return (0);
}
