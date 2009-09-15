/*	$NetBSD: timed_wait.h,v 1.1.1.1.2.2 2009/09/15 06:04:04 snj Exp $	*/

#ifndef _TIMED_WAIT_H_INCLUDED_
#define _TIMED_WAIT_H_INCLUDED_

/*++
/* NAME
/*	timed_wait 3h
/* SUMMARY
/*	wait operations with timeout
/* SYNOPSIS
/*	#include <timed_wait.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int timed_waitpid(pid_t, WAIT_STATUS_T *, int, int);

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
