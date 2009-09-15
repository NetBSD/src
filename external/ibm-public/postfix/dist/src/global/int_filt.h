/*	$NetBSD: int_filt.h,v 1.1.1.1.2.2 2009/09/15 06:02:44 snj Exp $	*/

#ifndef _INT_FILT_INCLUDED_
#define _INT_FILT_INCLUDED_

/*++
/* NAME
/*	int_filt 3h
/* SUMMARY
/*	internal mail classification
/* SYNOPSIS
/*	#include <int_filt.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#define INT_FILT_MASK_NONE		(0)
#define INT_FILT_MASK_NOTIFY		(1<<1)
#define INT_FILT_MASK_BOUNCE		(1<<2)

extern int int_filt_flags(int);

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
