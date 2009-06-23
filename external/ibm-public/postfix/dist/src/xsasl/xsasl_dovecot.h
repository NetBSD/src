/*	$NetBSD: xsasl_dovecot.h,v 1.1.1.1 2009/06/23 10:09:02 tron Exp $	*/

#ifndef _XSASL_DOVECOT_H_INCLUDED_
#define _XSASL_DOVECOT_H_INCLUDED_

/*++
/* NAME
/*	xsasl_dovecot 3h
/* SUMMARY
/*	Dovecot SASL plug-in
/* SYNOPSIS
/*	#include <xsasl_dovecot.h>
/* DESCRIPTION
/* .nf

 /*
  * XSASL library.
  */
#include <xsasl.h>

#if defined(USE_SASL_AUTH)

 /*
  * SASL protocol interface
  */
#define XSASL_TYPE_DOVECOT "dovecot"

extern XSASL_SERVER_IMPL *xsasl_dovecot_server_init(const char *, const char *);

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
