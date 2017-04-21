/*	$NetBSD: attr_override.h,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

#ifndef _ATTR_OVERRIDE_H_INCLUDED_
#define _ATTR_OVERRIDE_H_INCLUDED_

/*++
/* NAME
/*	attr_override 3h
/* SUMMARY
/*	apply name=value settings from string
/* SYNOPSIS
/*	#include <attr_override.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#include <check_arg.h>

extern void attr_override(char *, const char *, const char *,...);

typedef struct {
    const char *name;
    CONST_CHAR_STAR *target;
    int     min;
    int     max;
} ATTR_OVER_STR;

typedef struct {
    const char *name;
    const char *defval;
    int    *target;
    int     min;
    int     max;
} ATTR_OVER_TIME;

typedef struct {
    const char *name;
    int    *target;
    int     min;
    int     max;
} ATTR_OVER_INT;

/* Type-unchecked API, internal use only. */
#define ATTR_OVER_END		0
#define ATTR_OVER_STR_TABLE	1
#define ATTR_OVER_TIME_TABLE	2
#define ATTR_OVER_INT_TABLE	3

/* Type-checked API, external use only. */
#define CA_ATTR_OVER_END		0
#define CA_ATTR_OVER_STR_TABLE(v)	ATTR_OVER_STR_TABLE, CHECK_CPTR(ATTR_OVER, ATTR_OVER_STR, (v))
#define CA_ATTR_OVER_TIME_TABLE(v)	ATTR_OVER_TIME_TABLE, CHECK_CPTR(ATTR_OVER, ATTR_OVER_TIME, (v))
#define CA_ATTR_OVER_INT_TABLE(v)	ATTR_OVER_INT_TABLE, CHECK_CPTR(ATTR_OVER, ATTR_OVER_INT, (v))

CHECK_CPTR_HELPER_DCL(ATTR_OVER, ATTR_OVER_TIME);
CHECK_CPTR_HELPER_DCL(ATTR_OVER, ATTR_OVER_STR);
CHECK_CPTR_HELPER_DCL(ATTR_OVER, ATTR_OVER_INT);

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

#endif
