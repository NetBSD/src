/*	$NetBSD: sane_time.h,v 1.1.1.1.2.2 2009/09/15 06:04:03 snj Exp $	*/

#ifndef _SANE_TIME_H_
#define _SANE_TIME_H_

/*++
/* NAME
/*	sane_time 3h
/* SUMMARY
/*	time(2) with backward time jump protection
/* SYNOPSIS
/*	#include <sane_time.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>

 /*
  * External interface.
  */
extern time_t sane_time(void);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Patrik Rak
/*	Modra 6
/*	155 00, Prague, Czech Republic
/*--*/

#endif
