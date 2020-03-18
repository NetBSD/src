/*	$NetBSD: timed_wait.h,v 1.3 2020/03/18 19:05:22 christos Exp $	*/

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
extern int WARN_UNUSED_RESULT timed_waitpid(pid_t, WAIT_STATUS_T *, int, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
