/*	$NetBSD: percentm.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _PERCENT_H_INCLUDED_
#define _PERCENT_H_INCLUDED_

/*++
/* NAME
/*	percentm 3h
/* SUMMARY
/*	expand %m embedded in string to system error text
/* SYNOPSIS
/*	#include <percentm.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern char *percentm(const char *, int);

/* HISTORY
/* .ad
/* .fi
/*	A percentm() routine appears in the TCP Wrapper software
/*	by Wietse Venema.
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
