/*	$NetBSD: dsn_buf.h,v 1.1.1.1.16.1 2014/08/19 23:59:42 tls Exp $	*/

#ifndef _DSN_BUF_H_INCLUDED_
#define _DSN_BUF_H_INCLUDED_

/*++
/* NAME
/*	dsn_buf 3h
/* SUMMARY
/*	delivery status buffer
/* SYNOPSIS
/*	#include <dsn_buf.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Global library.
  */
#include <dsn.h>

 /*
  * Delivery status buffer, Postfix-internal form.
  */
typedef struct {
    DSN     dsn;			/* convenience */
    /* Formal members. */
    VSTRING *status;			/* RFC 3463 */
    VSTRING *action;			/* RFC 3464 */
    VSTRING *mtype;			/* null or remote MTA type */
    VSTRING *mname;			/* null or remote MTA name */
    VSTRING *dtype;			/* null, smtp, x-unix */
    VSTRING *dtext;			/* null, RFC 2821, sysexits.h */
    /* Informal free text. */
    VSTRING *reason;			/* free text */
} DSN_BUF;

#define DSB_DEF_ACTION	((char *) 0)

#define DSB_SKIP_RMTA	((char *) 0), ((char *) 0)
#define DSB_MTYPE_NONE	((char *) 0)
#define DSB_MTYPE_DNS	"dns"		/* RFC 2821 */

#define DSB_SKIP_REPLY	(char *) 0, " "	/* XXX Bogus? */
#define DSB_DTYPE_NONE	((char *) 0)
#define DSB_DTYPE_SMTP	"smtp"		/* RFC 2821 */
#define DSB_DTYPE_UNIX	"x-unix"	/* sysexits.h */
#define DSB_DTYPE_SASL	"x-sasl"	/* libsasl */

extern DSN_BUF *dsb_create(void);
extern DSN_BUF *PRINTFLIKE(8, 9) dsb_update(DSN_BUF *, const char *, const char *, const char *, const char *, const char *, const char *, const char *,...);
extern DSN_BUF *vdsb_simple(DSN_BUF *, const char *, const char *, va_list);
extern DSN_BUF *PRINTFLIKE(3, 4) dsb_simple(DSN_BUF *, const char *, const char *,...);
extern DSN_BUF *PRINTFLIKE(4, 5) dsb_unix(DSN_BUF *, const char *, const char *, const char *,...);
extern DSN_BUF *dsb_formal(DSN_BUF *, const char *, const char *, const char *, const char *, const char *, const char *);
extern DSN_BUF *dsb_status(DSN_BUF *, const char *);
extern void dsb_reset(DSN_BUF *);
extern void dsb_free(DSN_BUF *);

 /*
  * Early implementations of the DSN structure represented unavailable
  * information with null pointers. This resulted in hard to maintain code.
  * We now use empty strings instead, so there is no need anymore to convert
  * empty strings to null pointers in the macro below.
  */
#define DSN_FROM_DSN_BUF(dsb) \
    DSN_ASSIGN(&(dsb)->dsn, \
	vstring_str((dsb)->status), \
        vstring_str((dsb)->action), \
        vstring_str((dsb)->reason), \
        vstring_str((dsb)->dtype), \
        vstring_str((dsb)->dtext), \
        vstring_str((dsb)->mtype), \
        vstring_str((dsb)->mname))

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
