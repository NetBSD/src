/*	$NetBSD: log_adhoc.h,v 1.1.1.1.62.1 2026/11/04 23:14:47 perseant Exp $	*/

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
#include <pol_stats.h>

 /*
  * Client interface.
  */
extern void log_adhoc(const char *, MSG_STATS *, RECIPIENT *, const char *,
		              const POL_STATS *, DSN *, const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
