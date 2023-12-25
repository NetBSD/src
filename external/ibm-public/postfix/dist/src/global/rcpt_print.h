/*	$NetBSD: rcpt_print.h,v 1.1.1.1.52.1 2023/12/25 12:55:03 martin Exp $	*/

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
extern int rcpt_print(ATTR_SCAN_COMMON_FN, VSTREAM *, int, const void *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
