/*	$NetBSD: sent.h,v 1.1.1.5.2.1 2006/11/20 13:30:25 tron Exp $	*/

#ifndef _SENT_H_INCLUDED_
#define _SENT_H_INCLUDED_

/*++
/* NAME
/*	sent 3h
/* SUMMARY
/*	log that message was sent
/* SYNOPSIS
/*	#include <sent.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>
#include <stdarg.h>

 /*
  * Global library.
  */
#include <deliver_request.h>
#include <bounce.h>

 /*
  * External interface.
  */
#define SENT_FLAG_NONE	(0)

extern int sent(int, const char *, MSG_STATS *, RECIPIENT *, const char *, 
			DSN *);

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
