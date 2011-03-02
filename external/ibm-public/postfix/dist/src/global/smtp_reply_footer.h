/*	$NetBSD: smtp_reply_footer.h,v 1.1.1.1 2011/03/02 19:32:18 tron Exp $	*/

#ifndef _SMTP_REPLY_FOOTER_H_INCLUDED_
#define _SMTP_REPLY_FOOTER_H_INCLUDED_

/*++
/* NAME
/*	smtp_reply_footer 3h
/* SUMMARY
/*	SMTP reply footer text support
/* SYNOPSIS
/*	#include <smtp_reply_footer.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <mac_expand.h>

 /*
  * External interface.
  */
extern int smtp_reply_footer(VSTRING *, ssize_t, char *, const char *,
			             MAC_EXP_LOOKUP_FN, char *);

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
