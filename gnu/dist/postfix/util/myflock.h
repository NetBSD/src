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
extern int myflock(int, int);
extern int myflock_locked(int);

#define MYFLOCK_NONE		0
#define MYFLOCK_SHARED		1
#define MYFLOCK_EXCLUSIVE	2
#define MYFLOCK_LOCK_MASK	(MYFLOCK_SHARED | MYFLOCK_EXCLUSIVE)
#define MYFLOCK_NOWAIT		4
#define MYFLOCK_BITS		(MYFLOCK_LOCK_MASK | MYFLOCK_NOWAIT)

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
