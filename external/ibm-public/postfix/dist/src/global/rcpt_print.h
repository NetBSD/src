/*	$NetBSD: rcpt_print.h,v 1.1.1.1.2.2 2009/09/15 06:02:52 snj Exp $	*/

#ifndef _RCPT_PRINT_H_INCLUDED_
#define _RCPT_PRINT_H_INCLUDED_

/*++
/* NAME
/*	rcpt_print 3h
/* SUMMARY
/*	write RECIPIENT structure to stream
/* SYNOPSIS
/*	#include <rcpt_print.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <attr.h>

 /*
  * Global library.
  */
#include <recipient_list.h>

 /*
  * External interface.
  */
extern int rcpt_print(ATTR_SCAN_MASTER_FN, VSTREAM *, int, void *);

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
