/*	$NetBSD: dsn_mask.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _DSN_MASK_H_INCLUDED_
#define _DSN_MASK_H_INCLUDED_

/*++
/* NAME
/*	dsn_mask 3h
/* SUMMARY
/*	DSN embedding in SMTP
/* SYNOPSIS
/*	#include "dsn_mask.h"
/* DESCRIPTION
/* .nf

 /*
  * Support for MAIL FROM ... RET=mumble.
  */
#define DSN_RET_FULL	(1<<0)
#define DSN_RET_HDRS	(1<<1)
#define DSN_RET_BITS	(2)

 /*
  * Use this to filter bad content in queue files.
  */
#define DSN_RET_OK(v)	((v) == DSN_RET_FULL || (v) == DSN_RET_HDRS)

 /*
  * Only when RET is specified by the sender is the SMTP client allowed to
  * specify RET=mumble while delivering mail (RFC 3461 section 5.2.1).
  * However, if RET is not requested, then the MTA is allowed to interpret
  * this as RET=FULL or RET=HDRS (RFC 3461 section 4.3). Postfix chooses the
  * former.
  */

 /*
  * Conversion routines: string to mask and reverse.
  */
extern int dsn_ret_code(const char *);
extern const char *dsn_ret_str(int);

 /*
  * Support for RCPT TO ... NOTIFY=mumble is in the form of bit masks.
  */
#define DSN_NOTIFY_NEVER	(1<<0)	/* must not */
#define DSN_NOTIFY_SUCCESS	(1<<1)	/* must */
#define DSN_NOTIFY_FAILURE	(1<<2)	/* must */
#define DSN_NOTIFY_DELAY	(1<<3)	/* may */
#define DSN_NOTIFY_BITS		(4)

 /*
  * Any form of sender-requested notification.
  */
#define DSN_NOTIFY_ANY \
    (DSN_NOTIFY_SUCCESS | DSN_NOTIFY_FAILURE | DSN_NOTIFY_DELAY)

 /*
  * Override the sender-specified notification restriction.
  */
#define DSN_NOTIFY_OVERRIDE	(DSN_NOTIFY_ANY | DSN_NOTIFY_NEVER)

 /*
  * Use this to filter bad content in queue files.
  */
#define DSN_NOTIFY_OK(v) \
    ((v) == DSN_NOTIFY_NEVER || (v) == ((v) & DSN_NOTIFY_ANY))

 /*
  * Only when NOTIFY=something was requested by the sender is the SMTP client
  * allowed to specify NOTIFY=mumble while delivering mail (RFC 3461 section
  * 5.2.1). However, if NOTIFY is not requested, then the MTA is allowed to
  * interpret this as NOTIFY=FAILURE or NOTIFY=FAILURE,DELAY (RFC 3461
  * section 4.1). Postfix chooses the latter.
  */

 /*
  * Conversion routines: string to mask and reverse.
  */
extern int dsn_notify_mask(const char *);
extern const char *dsn_notify_str(int);

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
