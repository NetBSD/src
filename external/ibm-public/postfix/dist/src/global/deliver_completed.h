/*	$NetBSD: deliver_completed.h,v 1.1.1.1.4.2 2010/04/21 05:23:49 matt Exp $	*/

#ifndef _DELIVER_COMPLETED_H_INCLUDED_
#define _DELIVER_COMPLETED_H_INCLUDED_

/*++
/* NAME
/*	deliver_completed 3h
/* SUMMARY
/*	recipient delivery completion
/* SYNOPSIS
/*	#include <deliver_completed.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface.
  */
extern void deliver_completed(VSTREAM *, long);

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
