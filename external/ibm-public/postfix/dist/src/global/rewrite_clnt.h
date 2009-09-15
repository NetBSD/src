/*	$NetBSD: rewrite_clnt.h,v 1.1.1.1.2.2 2009/09/15 06:02:53 snj Exp $	*/

#ifndef _REWRITE_CLNT_H_INCLUDED_
#define _REWRITE_CLNT_H_INCLUDED_

/*++
/* NAME
/*	rewrite_clnt 3h
/* SUMMARY
/*	address rewriter client
/* SYNOPSIS
/*	#include <rewrite_clnt.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_proto.h>			/* MAIL_ATTR_RWR_LOCAL */

 /*
  * External interface.
  */
#define REWRITE_ADDR	"rewrite"
#define REWRITE_CANON	MAIL_ATTR_RWR_LOCAL	/* backwards compatibility */

extern VSTRING *rewrite_clnt(const char *, const char *, VSTRING *);
extern VSTRING *rewrite_clnt_internal(const char *, const char *, VSTRING *);

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
