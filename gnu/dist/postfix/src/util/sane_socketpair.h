/*	$NetBSD: sane_socketpair.h,v 1.1.1.2 2004/05/31 00:25:00 heas Exp $	*/

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

extern int sane_socketpair(int, int, int, int *);

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
