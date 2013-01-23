/*	$NetBSD: verify_sender_addr.h,v 1.1.1.1.2.2 2013/01/23 00:05:04 yamt Exp $	*/

#ifndef _VERIFY_SENDER_ADDR_H_INCLUDED_
#define _VERIFY_SENDER_ADDR_H_INCLUDED_

/*++
/* NAME
/*	verify_sender_addr 3h
/* SUMMARY
/*	address verify sender utilities
/* SYNOPSIS
/*	#include <verify_sender_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
const char *make_verify_sender_addr(void);
const char *valid_verify_sender_addr(const char *);

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
