/*	$NetBSD: timed_connect.h,v 1.1.1.1.4.2 2010/04/21 05:24:24 matt Exp $	*/

#ifndef _TIMED_CONNECT_H_INCLUDED_
#define _TIMED_CONNECT_H_INCLUDED_

/*++
/* NAME
/*	timed_connect 3h
/* SUMMARY
/*	connect operation with timeout
/* SYNOPSIS
/*	#include <sys/socket.h>
/*	#include <timed_connect.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int timed_connect(int, struct sockaddr *, int, int);

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
