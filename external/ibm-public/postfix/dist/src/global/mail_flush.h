/*	$NetBSD: mail_flush.h,v 1.1.1.1.2.2 2009/09/15 06:02:45 snj Exp $	*/

#ifndef _MAIL_FLUSH_H_INCLUDED_
#define _MAIL_FLUSH_H_INCLUDED_

/*++
/* NAME
/*	mail_flush 3h
/* SUMMARY
/*	flush backed up mail
/* SYNOPSIS
/*	#include <mail_flush.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int mail_flush_deferred(void);
extern int mail_flush_maildrop(void);

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
