/*	$NetBSD: dsn.h,v 1.1.1.1.2.2 2009/09/15 06:02:41 snj Exp $	*/

#ifndef _DSN_H_INCLUDED_
#define _DSN_H_INCLUDED_

/*++
/* NAME
/*	dsn 3h
/* SUMMARY
/*	RFC-compliant delivery status information
/* SYNOPSIS
/*	#include <dsn.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct {
    const char *status;			/* RFC 3463 status */
    const char *action;			/* Null / RFC 3464 action */
    const char *reason;			/* descriptive reason */
    const char *dtype;			/* Null / RFC 3464 diagnostic type */
    const char *dtext;			/* Null / RFC 3464 diagnostic code */
    const char *mtype;			/* Null / RFC 3464 MTA type */
    const char *mname;			/* Null / RFC 3464 remote MTA */
} DSN;

extern DSN *dsn_create(const char *, const char *, const char *, const char *,
		               const char *, const char *, const char *);
extern void dsn_free(DSN *);

#define DSN_ASSIGN(dsn, _status, _action, _reason, _dtype, _dtext, _mtype, _mname) \
    (((dsn)->status = (_status)), \
     ((dsn)->action = (_action)), \
     ((dsn)->reason = (_reason)), \
     ((dsn)->dtype = (_dtype)), \
     ((dsn)->dtext = (_dtext)), \
     ((dsn)->mtype = (_mtype)), \
     ((dsn)->mname = (_mname)), \
     (dsn))

#define DSN_SIMPLE(dsn, _status, _reason) \
    (((dsn)->status = (_status)), \
     ((dsn)->action = DSN_NO_ACTION), \
     ((dsn)->reason = (_reason)), \
     ((dsn)->dtype = DSN_NO_DTYPE), \
     ((dsn)->dtext = DSN_NO_DTEXT), \
     ((dsn)->mtype = DSN_NO_MTYPE), \
     ((dsn)->mname = DSN_NO_MNAME), \
     (dsn))

#define DSN_NO_ACTION	""
#define DSN_NO_DTYPE	""
#define DSN_NO_DTEXT	""
#define DSN_NO_MTYPE	""
#define DSN_NO_MNAME	""

 /*
  * Early implementations represented unavailable information with null
  * pointers. This resulted in code that is hard to maintain. We now use
  * empty strings instead. This does not waste precious memory as long as we
  * can represent empty strings efficiently by collapsing them.
  * 
  * The only restriction left is that the status and reason are never null or
  * empty; this is enforced by dsn_create() which is invoked by DSN_COPY().
  * This complicates the server reply parsing code in the smtp(8) and lmtp(8)
  * clients. they must never supply empty strings for these required fields.
  */
#define DSN_COPY(dsn) \
    dsn_create((dsn)->status, (dsn)->action, (dsn)->reason, \
	(dsn)->dtype, (dsn)->dtext, \
	(dsn)->mtype, (dsn)->mname)

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
