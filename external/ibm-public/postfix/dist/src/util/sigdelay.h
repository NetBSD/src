/*	$NetBSD: sigdelay.h,v 1.1.1.1.4.2 2010/04/21 05:24:24 matt Exp $	*/

#ifndef _SIGDELAY_H_INCLUDED_
#define _SIGDELAY_H_INCLUDED_

/*++
/* NAME
/*	sigdelay 3h
/* SUMMARY
/*	delay/resume signal delivery
/* SYNOPSIS
/*	#include <sigdelay.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void sigdelay(void);
extern void sigresume(void);

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
