/*	$NetBSD: dsn_util.h,v 1.1.1.1.2.2 2009/09/15 06:02:42 snj Exp $	*/

#ifndef _DSN_UTIL_H_INCLUDED_
#define _DSN_UTIL_H_INCLUDED_

/*++
/* NAME
/*	dsn_util 3h
/* SUMMARY
/*	DSN status parsing routines
/* SYNOPSIS
/*	#include <dsn_util.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Detail format is digit "." digit{1,3} "." digit{1,3}.
  */
#define DSN_DIGS1	1		/* leading digits */
#define DSN_DIGS2	3		/* middle digits */
#define DSN_DIGS3	3		/* trailing digits */
#define DSN_LEN		(DSN_DIGS1 + 1 + DSN_DIGS2 + 1 + DSN_DIGS3)
#define DSN_SIZE	(DSN_LEN + 1)

 /*
  * Storage for an enhanced status code. Avoid using malloc for itty-bitty
  * strings with a known size limit.
  * 
  * XXX gcc version 2 complains about sizeof() as format width specifier.
  */
typedef struct {
    char    data[DSN_SIZE];		/* NOT a public interface */
} DSN_STAT;

#define DSN_UPDATE(dsn_buf, dsn, len) do { \
	if (len >= sizeof((dsn_buf).data)) \
	    msg_panic("DSN_UPDATE: bad DSN code \"%.*s...\" length %d", \
		INT_SIZEOF((dsn_buf).data) - 1, dsn, len); \
	strncpy((dsn_buf).data, (dsn), (len)); \
	(dsn_buf).data[len] = 0; \
    } while (0)

#define DSN_STATUS(dsn_buf) ((const char *) (dsn_buf).data)

#define DSN_CLASS(dsn_buf) ((dsn_buf).data[0])

 /*
  * Split flat text into detail code and free text.
  */
typedef struct {
    DSN_STAT dsn;			/* RFC 3463 status */
    const char *text;			/* free text */
} DSN_SPLIT;

extern DSN_SPLIT *dsn_split(DSN_SPLIT *, const char *, const char *);
extern size_t dsn_valid(const char *);

 /*
  * Create flat text from detail code and free text.
  */
extern char *dsn_prepend(const char *, const char *);

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
