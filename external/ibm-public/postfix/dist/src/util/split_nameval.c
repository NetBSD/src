/*	$NetBSD: split_nameval.c,v 1.1.1.1.16.1 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	split_nameval 3
/* SUMMARY
/*	name-value splitter
/* SYNOPSIS
/*	#include <split_nameval.h>
/*
/*	const char *split_nameval(buf, name, value)
/*	char	*buf;
/*	char	**name;
/*	char	**value;
/* DESCRIPTION
/*	split_nameval() takes a logical line from readlline() and expects
/*	text of the form "name = value" or "name =". The buffer
/*	argument is broken up into name and value substrings.
/*
/*	Arguments:
/* .IP buf
/*	Result from readlline() or equivalent. The buffer is modified.
/* .IP name
/*	Upon successful completion, this is set to the name
/*	substring.
/* .IP value
/*	Upon successful completion, this is set to the value
/*	substring.
/* FEATURES
/* SEE ALSO
/*	dict(3) mid-level dictionary routines
/* BUGS
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
/*
/*	The result is a null pointer in case of success, a string
/*	describing the error otherwise: missing '=' after attribute
/*	name; missing attribute name.
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

/* System libraries. */

#include "sys_defs.h"
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <stringops.h>

/* split_nameval - split text into name and value */

const char *split_nameval(char *buf, char **name, char **value)
{
    char   *np;				/* name substring */
    char   *vp;				/* value substring */
    char   *cp;
    char   *ep;

    /*
     * Ugly macros to make complex expressions less unreadable.
     */
#define SKIP(start, var, cond) do { \
	for (var = start; *var && (cond); var++) \
	    /* void */; \
    } while (0)

#define TRIM(s) do { \
	char *p; \
	for (p = (s) + strlen(s); p > (s) && ISSPACE(p[-1]); p--) \
	    /* void */; \
	*p = 0; \
    } while (0)

    SKIP(buf, np, ISSPACE(*np));		/* find name begin */
    if (*np == 0)
	return ("missing attribute name");
    SKIP(np, ep, !ISSPACE(*ep) && *ep != '=');	/* find name end */
    SKIP(ep, cp, ISSPACE(*cp));			/* skip blanks before '=' */
    if (*cp != '=')				/* need '=' */
	return ("missing '=' after attribute name");
    *ep = 0;					/* terminate name */
    cp++;					/* skip over '=' */
    SKIP(cp, vp, ISSPACE(*vp));			/* skip leading blanks */
    TRIM(vp);					/* trim trailing blanks */
    *name = np;
    *value = vp;
    return (0);
}
