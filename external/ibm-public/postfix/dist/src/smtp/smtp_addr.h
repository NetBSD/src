/*	$NetBSD: smtp_addr.h,v 1.2.22.1 2023/12/25 12:43:35 martin Exp $	*/

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
extern DNS_RR *smtp_host_addr(const char *, int, DSN_BUF *);
extern DNS_RR *smtp_domain_addr(const char *, DNS_RR **, int, DSN_BUF *, int *);
extern DNS_RR *smtp_service_addr(const char *, const char *, DNS_RR **, int, DSN_BUF *, int *);

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
