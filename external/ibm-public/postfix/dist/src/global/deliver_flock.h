/*	$NetBSD: deliver_flock.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _DELIVER_FLOCK_H_INCLUDED_
#define _DELIVER_FLOCK_H_INCLUDED_

/*++
/* NAME
/*	deliver_flock 3h
/* SUMMARY
/*	lock open file for mail delivery
/* SYNOPSIS
/*	#include <deliver_flock.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <myflock.h>
#include <vstring.h>

 /*
  * External interface.
  */
extern int deliver_flock(int, int, VSTRING *);

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
