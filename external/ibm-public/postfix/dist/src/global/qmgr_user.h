/*	$NetBSD: qmgr_user.h,v 1.1.1.1.2.2 2009/09/15 06:02:51 snj Exp $	*/

#ifndef _QMGR_USER_H_INCLUDED_
#define _QMGR_USER_H_INCLUDED_

/*++
/* NAME
/*	qmgr_user 3h
/* SUMMARY
/*	qmgr user interface codes
/* SYNOPSIS
/*	#include <qmgr_user.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <dsn_mask.h>

 /*
  * Queue file read options. Flags 16- are reserved by qmgr.h; unfortunately
  * DSN_NOTIFY_* needs to be shifted to avoid breaking compatibility with
  * already queued mail that uses QMGR_READ_FLAG_MIXED_RCPT_OTHER.
  */
#define QMGR_READ_FLAG_NONE		0	/* No special features */
#define QMGR_READ_FLAG_MIXED_RCPT_OTHER	(1<<0)
#define QMGR_READ_FLAG_FROM_DSN(x)	((x) << 1)

#define QMGR_READ_FLAG_NOTIFY_NEVER	(DSN_NOTIFY_NEVER << 1)
#define QMGR_READ_FLAG_NOTIFY_SUCCESS	(DSN_NOTIFY_SUCCESS << 1)
#define QMGR_READ_FLAG_NOTIFY_DELAY	(DSN_NOTIFY_DELAY << 1)
#define QMGR_READ_FLAG_NOTIFY_FAILURE	(DSN_NOTIFY_FAILURE << 1)

#define QMGR_READ_FLAG_USER \
    (QMGR_READ_FLAG_NOTIFY_NEVER | QMGR_READ_FLAG_NOTIFY_SUCCESS \
    | QMGR_READ_FLAG_NOTIFY_DELAY | QMGR_READ_FLAG_NOTIFY_FAILURE \
    | QMGR_READ_FLAG_MIXED_RCPT_OTHER)

 /*
  * Backwards compatibility.
  */
#define QMGR_READ_FLAG_DEFAULT	(QMGR_READ_FLAG_MIXED_RCPT_OTHER)

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
