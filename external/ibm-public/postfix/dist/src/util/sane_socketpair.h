/*	$NetBSD: sane_socketpair.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _SANE_SOCKETPAIR_H_
#define _SANE_SOCKETPAIR_H_

/*++
/* NAME
/*	sane_socketpair 3h
/* SUMMARY
/*	sanitize socketpair() error returns
/* SYNOPSIS
/*	#include <sane_socketpair.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int WARN_UNUSED_RESULT sane_socketpair(int, int, int, int *);

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
