/*	$NetBSD: ehlo_mask.h,v 1.1.1.1 2009/06/23 10:08:46 tron Exp $	*/

#ifndef _EHLO_MASK_H_INCLUDED_
#define _EHLO_MASK_H_INCLUDED_

/*++
/* NAME
/*	name_mask 3h
/* SUMMARY
/*	map names to bit mask
/* SYNOPSIS
/*	#include <name_mask.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#define EHLO_MASK_8BITMIME	(1<<0)	/* start of first byte */
#define EHLO_MASK_PIPELINING	(1<<1)
#define EHLO_MASK_SIZE		(1<<2)
#define EHLO_MASK_VRFY		(1<<3)
#define EHLO_MASK_ETRN		(1<<4)
#define EHLO_MASK_AUTH		(1<<5)
#define EHLO_MASK_VERP		(1<<6)
#define EHLO_MASK_STARTTLS	(1<<7)

#define EHLO_MASK_XCLIENT	(1<<8)	/* start of second byte */
#define EHLO_MASK_XFORWARD	(1<<9)
#define EHLO_MASK_ENHANCEDSTATUSCODES	(1<<10)
#define EHLO_MASK_DSN		(1<<11)
#define EHLO_MASK_SILENT	(1<<15)

extern int ehlo_mask(const char *);
extern const char *str_ehlo_mask(int);

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
