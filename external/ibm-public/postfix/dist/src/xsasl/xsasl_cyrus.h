/*	$NetBSD: xsasl_cyrus.h,v 1.1.1.1.12.1 2012/06/13 19:29:05 riz Exp $	*/

#ifndef _XSASL_CYRUS_H_INCLUDED_
#define _XSASL_CYRUS_H_INCLUDED_

/*++
/* NAME
/*	xsasl_cyrus 3h
/* SUMMARY
/*	Cyrus SASL plug-in
/* SYNOPSIS
/*	#include <xsasl_cyrus.h>
/* DESCRIPTION
/* .nf

 /*
  * XSASL library.
  */
#include <xsasl.h>

#if defined(USE_SASL_AUTH) && defined(USE_CYRUS_SASL)

 /*
  * SASL protocol interface
  */
#define XSASL_TYPE_CYRUS "cyrus"

extern XSASL_SERVER_IMPL *xsasl_cyrus_server_init(const char *, const char *);
extern XSASL_CLIENT_IMPL *xsasl_cyrus_client_init(const char *, const char *);

 /*
  * Internal definitions for client and server module.
  */
typedef int (*XSASL_CYRUS_CB) (void);

#endif

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
