/*	$NetBSD: watchdog.h,v 1.1.1.1.2.2 2009/09/15 06:04:05 snj Exp $	*/

#ifndef _WATCHDOG_H_INCLUDED_
#define _WATCHDOG_H_INCLUDED_

/*++
/* NAME
/*	watchdog 3h
/* SUMMARY
/*	watchdog timer
/* SYNOPSIS
/*	#include "watchdog.h"
 DESCRIPTION
 .nf

 /*
  * External interface.
  */
typedef struct WATCHDOG WATCHDOG;
typedef void (*WATCHDOG_FN) (WATCHDOG *, char *);
extern WATCHDOG *watchdog_create(unsigned, WATCHDOG_FN, char *);
extern void watchdog_start(WATCHDOG *);
extern void watchdog_stop(WATCHDOG *);
extern void watchdog_destroy(WATCHDOG *);
extern void watchdog_pat(void);

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
