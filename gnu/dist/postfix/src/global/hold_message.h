/*	$NetBSD: hold_message.h,v 1.1.1.2 2004/05/31 00:24:31 heas Exp $	*/

#ifndef _HOLD_MESSAGE_H_INCLUDED_
#define _HOLD_MESSAGE_H_INCLUDED_

/*++
/* NAME
/*	hold_message 3h
/* SUMMARY
/*	mark queue file as corrupt
/* SYNOPSIS
/*	#include <hold_message.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int hold_message(VSTRING *, const char *, const char *);

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
