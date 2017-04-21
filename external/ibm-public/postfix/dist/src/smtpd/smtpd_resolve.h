/*	$NetBSD: smtpd_resolve.h,v 1.1.1.1.36.1 2017/04/21 16:52:52 bouyer Exp $	*/

/*++
/* NAME
/*	smtpd_resolve 3h
/* SUMMARY
/*	caching resolve client
/* SYNOPSIS
/*	include <smtpd_resolve.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <resolve_clnt.h>

 /*
  * External interface.
  */
extern void smtpd_resolve_init(int);
extern const RESOLVE_REPLY *smtpd_resolve_addr(const char*, const char *);

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
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*--*/
