/*	$NetBSD: smtp_addr.h,v 1.1.1.3 2004/05/31 00:24:46 heas Exp $	*/

/*++
/* NAME
/*	smtp_addr 3h
/* SUMMARY
/*	SMTP server address lookup
/* SYNOPSIS
/*	#include "smtp_addr.h"
/* DESCRIPTION
/* .nf

 /*
  * DNS library.
  */
#include <dns.h>

 /*
  * Internal interfaces.
  */
extern DNS_RR *smtp_host_addr(char *, int, VSTRING *);
extern DNS_RR *smtp_domain_addr(char *, int, VSTRING *);

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
