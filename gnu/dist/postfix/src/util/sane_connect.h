/*	$NetBSD: sane_connect.h,v 1.1.1.2 2004/05/31 00:25:00 heas Exp $	*/

#ifndef _SANE_CONNECT_H_
#define _SANE_CONNECT_H_

/*++
/* NAME
/*	sane_connect 3h
/* SUMMARY
/*	sanitize connect() results
/* SYNOPSIS
/*	#include <sane_connect.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int sane_connect(int, struct sockaddr *, SOCKADDR_SIZE);

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
