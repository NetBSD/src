/*	$NetBSD: mynetworks.h,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

#ifndef _MYNETWORKS_H_INCLUDED_
#define _MYNETWORKS_H_INCLUDED_

/*++
/* NAME
/*	mynetworks 3h
/* SUMMARY
/*	lookup patterns for my own interface addresses
/* SYNOPSIS
/*	#include <mynetworks.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern const char *mynetworks(void);
extern const char *mynetworks_host(void);

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
