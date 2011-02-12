/*	$NetBSD: xsasl_saslc.h,v 1.1 2011/02/12 19:07:09 christos Exp $	*/

#ifndef _XSASL_SASLC_H_INCLUDED_
#define _XSASL_SASLC_H_INCLUDED_

/*++
/* NAME
/*	xsasl_saslc 3h
/* SUMMARY
/*	Saslc SASL plug-in
/* SYNOPSIS
/*	#include <xsasl_saslc.h>
/* DESCRIPTION
/* .nf

 /*
  * XSASL library.
  */
#include "xsasl.h"

#if defined(USE_SASL_AUTH) && defined(USE_SASLC_SASL)

 /*
  * SASL protocol interface
  */
#define XSASL_TYPE_SASLC "saslc"

extern XSASL_CLIENT_IMPL *xsasl_saslc_client_init(const char *, const char *);

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

#endif /* _XSASL_SASLC_H_INCLUDED_ */
