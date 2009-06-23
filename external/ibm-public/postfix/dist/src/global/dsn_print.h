/*	$NetBSD: dsn_print.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _DSN_PRINT_H_INCLUDED_
#define _DSN_PRINT_H_INCLUDED_

/*++
/* NAME
/*	dsn_print 3h
/* SUMMARY
/*	write DSN structure to stream
/* SYNOPSIS
/*	#include <dsn_print.h>
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
#include <dsn.h>

 /*
  * External interface.
  */
extern int dsn_print(ATTR_PRINT_MASTER_FN, VSTREAM *, int, void *);

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
