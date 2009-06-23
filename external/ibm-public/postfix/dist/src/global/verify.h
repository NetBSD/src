/*	$NetBSD: verify.h,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

#ifndef _VERIFY_H_INCLUDED_
#define _VERIFY_H_INCLUDED_

/*++
/* NAME
/*	verify 3h
/* SUMMARY
/*	update user message delivery record
/* SYNOPSIS
/*	#include <verify.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>

 /*
  * Global library.
  */
#include <deliver_request.h>

 /*
  * External interface.
  */
extern int verify_append(const char *, MSG_STATS *, RECIPIENT *,
			         const char *, DSN *, int);

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
