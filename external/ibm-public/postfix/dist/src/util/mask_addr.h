/*	$NetBSD: mask_addr.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _MASK_ADDR_H_INCLUDED_
#define _MASK_ADDR_H_INCLUDED_

/*++
/* NAME
/*	mask_addr 3h
/* SUMMARY
/*	address bit banging
/* SYNOPSIS
/*	#include <mask_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void mask_addr(unsigned char *, unsigned, unsigned);

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
