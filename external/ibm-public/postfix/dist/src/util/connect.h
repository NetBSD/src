/*	$NetBSD: connect.h,v 1.1.1.2.4.1 2014/05/22 14:08:04 yamt Exp $	*/

#ifndef _CONNECT_H_INCLUDED_
#define _CONNECT_H_INCLUDED_

/*++
/* NAME
/*	connect 3h
/* SUMMARY
/*	client interface file
/* SYNOPSIS
/*	#include <connect.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <iostuff.h>

 /*
  * Client external interface.
  */
extern int unix_connect(const char *, int, int);
extern int inet_connect(const char *, int, int);
extern int stream_connect(const char *, int, int);

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
