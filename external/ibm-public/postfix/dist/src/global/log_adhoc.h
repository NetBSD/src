/*	$NetBSD: log_adhoc.h,v 1.1.1.1.2.3 2011/01/07 01:24:03 riz Exp $	*/

#ifndef _LOG_ADHOC_H_INCLUDED_
#define _LOG_ADHOC_H_INCLUDED_

/*++
/* NAME
/*	log_adhoc 3h
/* SUMMARY
/*	ad-hoc delivery event logging
/* SYNOPSIS
/*	#include <log_adhoc.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>

 /*
  * Global library.
  */
#include <recipient_list.h>
#include <dsn.h>
#include <msg_stats.h>

 /*
  * Client interface.
  */
extern void log_adhoc(const char *, MSG_STATS *, RECIPIENT *, const char *,
		              DSN *, const char *);

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
