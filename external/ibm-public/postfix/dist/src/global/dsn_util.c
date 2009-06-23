/*	$NetBSD: dsn_util.c,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

/*++
/* NAME
/*	dsn_util 3
/* SUMMARY
/*	DSN status parsing routines
/* SYNOPSIS
/*	#include <dsn_util.h>
/*
/*	#define DSN_SIZE ...
/*
/*	typedef struct { ... } DSN_BUF;
/*
/*	typedef struct {
/* .in +4
/*		DSN_STAT dsn;		/* RFC 3463 status */
/*		const char *text;	/* Free text */
/* .in -4
/*	} DSN_SPLIT;
/*
/*	DSN_SPLIT *dsn_split(dp, def_dsn, text)
/*	DSN_SPLIT *dp;
/*	const char *def_dsn;
/*	const char *text;
/*
/*	char	*dsn_prepend(def_dsn, text)
/*	const char *def_dsn;
/*	const char *text;
/*
/*	size_t	dsn_valid(text)
/*	const char *text;
/*
/*	void	DSN_UPDATE(dsn_buf, dsn, len)
/*	DSN_BUF	dsn_buf;
/*	const char *dsn;
/*	size_t	len;
/*
/*	const char *DSN_CODE(dsn_buf)
/*	DSN_BUF dsn_buf;
/*
/*	char	*DSN_CLASS(dsn_buf)
/*	DSN_BUF	dsn_buf;
/* DESCRIPTION
/*	The functions in this module manipulate pairs of RFC 3463
/*	status codes and descriptive free text.
/*
/*	dsn_split() splits text into an RFC 3463 status code and
/*	descriptive free text.  When the text does not start with
/*	a status code, the specified default status code is used
/*	instead.  Whitespace before the optional status code or
/*	text is skipped.  dsn_split() returns a copy of the RFC
/*	3463 status code, and returns a pointer to (not copy of)
/*	the remainder of the text.  The result value is the first
/*	argument.
/*
/*	dsn_prepend() prepends the specified default RFC 3463 status
/*	code to the specified text if no status code is present in
/*	the text. This function produces the same result as calling
/*	concatenate() with the results from dsn_split().  The result
/*	should be passed to myfree(). Whitespace before the optional
/*	status code or text is skipped.
/*
/*	dsn_valid() returns the length of the RFC 3463 status code
/*	at the beginning of text, or zero. It does not skip initial
/*	whitespace.
/*
/*	Arguments:
/* .IP def_dsn
/*	Null-terminated default RFC 3463 status code that will be
/*	used when the free text does not start with one.
/* .IP dp
/*	Pointer to storage for copy of DSN status code, and for
/*	pointer to free text.
/* .IP dsn
/*	Null-terminated RFC 3463 status code.
/* .IP text
/*	Null-terminated free text.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Panic: invalid default DSN code.
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <stringops.h>

/* Global library. */

#include <dsn_util.h>

/* dsn_valid - check RFC 3463 enhanced status code, return length or zero */

size_t  dsn_valid(const char *text)
{
    const unsigned char *cp = (unsigned char *) text;
    size_t  len;

    /* First portion is one digit followed by dot. */
    if ((cp[0] != '2' && cp[0] != '4' && cp[0] != '5') || cp[1] != '.')
	return (0);

    /* Second portion is 1-3 digits followed by dot. */
    cp += 2;
    if ((len = strspn((char *) cp, "0123456789")) < 1 || len > DSN_DIGS2
	|| cp[len] != '.')
	return (0);

    /* Last portion is 1-3 digits followed by end-of-string or whitespace. */
    cp += len + 1;
    if ((len = strspn((char *) cp, "0123456789")) < 1 || len > DSN_DIGS3
	|| (cp[len] != 0 && !ISSPACE(cp[len])))
	return (0);

    return (((char *) cp - text) + len);
}

/* dsn_split - split text into DSN and text */

DSN_SPLIT *dsn_split(DSN_SPLIT *dp, const char *def_dsn, const char *text)
{
    const char *myname = "dsn_split";
    const char *cp = text;
    size_t  len;

    /*
     * Look for an optional RFC 3463 enhanced status code.
     * 
     * XXX If we want to enforce that the first digit of the status code in the
     * text matches the default status code, then pipe_command() needs to be
     * changed. It currently auto-detects the reply code without knowing in
     * advance if the result will start with '4' or '5'.
     */
    while (ISSPACE(*cp))
	cp++;
    if ((len = dsn_valid(cp)) > 0) {
	strncpy(dp->dsn.data, cp, len);
	dp->dsn.data[len] = 0;
	cp += len + 1;
    } else if ((len = dsn_valid(def_dsn)) > 0) {
	strncpy(dp->dsn.data, def_dsn, len);
	dp->dsn.data[len] = 0;
    } else {
	msg_panic("%s: bad default status \"%s\"", myname, def_dsn);
    }

    /*
     * The remainder is free text.
     */
    while (ISSPACE(*cp))
	cp++;
    dp->text = cp;

    return (dp);
}

/* dsn_prepend - prepend optional status to text, result on heap */

char   *dsn_prepend(const char *def_dsn, const char *text)
{
    DSN_SPLIT dp;

    dsn_split(&dp, def_dsn, text);
    return (concatenate(DSN_STATUS(dp.dsn), " ", dp.text, (char *) 0));
}
