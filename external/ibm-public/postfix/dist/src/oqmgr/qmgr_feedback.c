/*	$NetBSD: qmgr_feedback.c,v 1.1.1.1.2.2 2009/09/15 06:03:21 snj Exp $	*/

/*++
/* NAME
/*	qmgr_feedback 3
/* SUMMARY
/*	delivery agent feedback management
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	void	qmgr_feedback_init(fbck_ctl, name_prefix, name_tail,
/*					def_name, def_value)
/*	QMGR_FEEDBACK *fbck_ctl;
/*	const char *name_prefix;
/*	const char *name_tail;
/*	const char *def_name;
/*	const char *def_value;
/*
/*	double	QMGR_FEEDBACK_VAL(fbck_ctl, concurrency)
/*	QMGR_FEEDBACK *fbck_ctl;
/*	const int concurrency;
/* DESCRIPTION
/*	Upon completion of a delivery request, a delivery agent
/*	provides a hint that the scheduler should dedicate fewer or
/*	more resources to a specific destination.
/*
/*	qmgr_feedback_init() looks up transport-dependent positive
/*	or negative concurrency feedback control information from
/*	main.cf, and converts it to internal form.
/*
/*	QMGR_FEEDBACK_VAL() computes a concurrency adjustment based
/*	on a preprocessed feedback control information and the
/*	current concurrency window. This is an "unsafe" macro that
/*	evaluates some arguments multiple times.
/*
/*	Arguments:
/* .IP fbck_ctl
/*	Pointer to QMGR_FEEDBACK structure where the result will
/*	be stored.
/* .IP name_prefix
/*	Mail delivery transport name, used as the initial portion
/*	of a transport-dependent concurrency feedback parameter
/*	name.
/* .IP name_tail
/*	The second, and fixed, portion of a transport-dependent
/*	concurrency feedback parameter.
/* .IP def_name
/*	The name of a default feedback parameter.
/* .IP def_val
/*	The value of the default feedback parameter.
/* .IP concurrency
/*	Delivery concurrency for concurrency-dependent feedback calculation.
/* DIAGNOSTICS
/*	Warning: configuration error or unreasonable input. The program
/*	uses name_tail feedback instead.
/*	Panic: consistency check failure.
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
#include <limits.h>			/* INT_MAX */
#include <stdio.h>			/* sscanf() */
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <name_code.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_params.h>
#include <mail_conf.h>

/* Application-specific. */

#include "qmgr.h"

 /*
  * Lookup tables for main.cf feedback method names.
  */
const NAME_CODE qmgr_feedback_map[] = {
    CONC_FDBACK_NAME_WIN, QMGR_FEEDBACK_IDX_WIN,
#ifdef QMGR_FEEDBACK_IDX_SQRT_WIN
    CONC_FDBACK_NAME_SQRT_WIN, QMGR_FEEDBACK_IDX_SQRT_WIN,
#endif
    0, QMGR_FEEDBACK_IDX_NONE,
};

/* qmgr_feedback_init - initialize feedback control */

void    qmgr_feedback_init(QMGR_FEEDBACK *fb,
			           const char *name_prefix,
			           const char *name_tail,
			           const char *def_name,
			           const char *def_val)
{
    double  enum_val;
    char    denom_str[30 + 1];
    double  denom_val;
    char    slash;
    char    junk;
    char   *fbck_name;
    char   *fbck_val;

    /*
     * Look up the transport-dependent feedback value.
     */
    fbck_name = concatenate(name_prefix, name_tail, (char *) 0);
    fbck_val = get_mail_conf_str(fbck_name, def_val, 1, 0);

    /*
     * We allow users to express feedback as 1/8, as a more user-friendly
     * alternative to 0.125 (or worse, having users specify the number of
     * events in a feedback hysteresis cycle).
     * 
     * We use some sscanf() fu to parse the value into numerator and optional
     * "/" followed by denominator. We're doing this only a few times during
     * the process life time, so we strive for convenience instead of speed.
     */
#define INCLUSIVE_BOUNDS(val, low, high) ((val) >= (low) && (val) <= (high))

    fb->hysteresis = 1;				/* legacy */
    fb->base = -1;				/* assume error */

    switch (sscanf(fbck_val, "%lf %1[/] %30s%c",
		   &enum_val, &slash, denom_str, &junk)) {
    case 1:
	fb->index = QMGR_FEEDBACK_IDX_NONE;
	fb->base = enum_val;
	break;
    case 3:
	if ((fb->index = name_code(qmgr_feedback_map, NAME_CODE_FLAG_NONE,
				   denom_str)) != QMGR_FEEDBACK_IDX_NONE) {
	    fb->base = enum_val;
	} else if (INCLUSIVE_BOUNDS(enum_val, 0, INT_MAX)
		   && sscanf(denom_str, "%lf%c", &denom_val, &junk) == 1
		   && INCLUSIVE_BOUNDS(denom_val, 1.0 / INT_MAX, INT_MAX)) {
	    fb->base = enum_val / denom_val;
	}
	break;
    }

    /*
     * Sanity check. If input is bad, we just warn and use a reasonable
     * default.
     */
    if (!INCLUSIVE_BOUNDS(fb->base, 0, 1)) {
	msg_warn("%s: ignoring malformed or unreasonable feedback: %s",
		 strcmp(fbck_val, def_val) ? fbck_name : def_name, fbck_val);
	fb->index = QMGR_FEEDBACK_IDX_NONE;
	fb->base = 1;
    }

    /*
     * Performance debugging/analysis.
     */
    if (var_conc_feedback_debug)
	msg_info("%s: %s feedback type %d value at %d: %g",
		 name_prefix, strcmp(fbck_val, def_val) ?
		 fbck_name : def_name, fb->index, var_init_dest_concurrency,
		 QMGR_FEEDBACK_VAL(*fb, var_init_dest_concurrency));

    myfree(fbck_name);
    myfree(fbck_val);
}
