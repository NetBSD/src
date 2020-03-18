/*	$NetBSD: smtpd_expand.h,v 1.3 2020/03/18 19:05:20 christos Exp $	*/

/*++
/* NAME
/*	smtpd_expand 3h
/* SUMMARY
/*	SMTP server macro expansion
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_expand.h>
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
extern VSTRING *smtpd_expand_filter;
void    smtpd_expand_init(void);
const char *smtpd_expand_lookup(const char *, int, void *);
int     smtpd_expand(SMTPD_STATE *, VSTRING *, const char *, int);

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
