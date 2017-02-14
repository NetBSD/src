/*	$NetBSD: smtp_reply_footer.h,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

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
extern int smtp_reply_footer(VSTRING *, ssize_t, const char *, const char *,
			             MAC_EXP_LOOKUP_FN, void *);

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
