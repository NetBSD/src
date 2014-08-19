/*	$NetBSD: trigger.h,v 1.1.1.2.10.1 2014/08/19 23:59:45 tls Exp $	*/

#ifndef _TRIGGER_H_INCLUDED_
#define _TRIGGER_H_INCLUDED_

/*++
/* NAME
/*	trigger 3h
/* SUMMARY
/*	client interface file
/* SYNOPSIS
/*	#include <trigger.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int unix_trigger(const char *, const char *, ssize_t, int);
extern int inet_trigger(const char *, const char *, ssize_t, int);
extern int fifo_trigger(const char *, const char *, ssize_t, int);
extern int stream_trigger(const char *, const char *, ssize_t, int);
extern int pass_trigger(const char *, const char *, ssize_t, int);

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
