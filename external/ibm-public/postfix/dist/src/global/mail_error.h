/*	$NetBSD: mail_error.h,v 1.1.1.1.2.2 2009/09/15 06:02:45 snj Exp $	*/

#ifndef _MAIL_ERROR_H_INCLUDED_
#define _MAIL_ERROR_H_INCLUDED_

/*++
/* NAME
/*	mail_error 3h
/* SUMMARY
/*	mail error classes
/* SYNOPSIS
/*	#include <mail_error.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <name_mask.h>

 /*
  * External interface.
  */
#define MAIL_ERROR_POLICY	(1<<0)
#define MAIL_ERROR_PROTOCOL	(1<<1)
#define MAIL_ERROR_BOUNCE	(1<<2)
#define MAIL_ERROR_SOFTWARE	(1<<3)
#define MAIL_ERROR_RESOURCE	(1<<4)
#define MAIL_ERROR_2BOUNCE	(1<<5)
#define MAIL_ERROR_DELAY	(1<<6)

extern const NAME_MASK mail_error_masks[];

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
