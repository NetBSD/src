/*	$NetBSD: myflock.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _MYFLOCK_H_INCLUDED_
#define _MYFLOCK_H_INCLUDED_

/*++
/* NAME
/*	myflock 3h
/* SUMMARY
/*	lock open file
/* SYNOPSIS
/*	#include <myflock.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int WARN_UNUSED_RESULT myflock(int, int, int);

 /*
  * Lock styles.
  */
#define MYFLOCK_STYLE_FLOCK	1
#define MYFLOCK_STYLE_FCNTL	2

 /*
  * Lock request types.
  */
#define MYFLOCK_OP_NONE		0
#define MYFLOCK_OP_SHARED	1
#define MYFLOCK_OP_EXCLUSIVE	2
#define MYFLOCK_OP_NOWAIT	4

#define MYFLOCK_OP_BITS \
	(MYFLOCK_OP_SHARED | MYFLOCK_OP_EXCLUSIVE | MYFLOCK_OP_NOWAIT)

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
