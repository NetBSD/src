/*	$NetBSD: smtp_sasl.h,v 1.1.1.1 2009/06/23 10:08:54 tron Exp $	*/

/*++
/* NAME
/*	smtp_sasl 3h
/* SUMMARY
/*	Postfix SASL interface for SMTP client
/* SYNOPSIS
/*	#include "smtp_sasl.h"
/* DESCRIPTION
/* .nf

 /*
 * SASL protocol functions
 */
extern void smtp_sasl_initialize(void);
extern void smtp_sasl_connect(SMTP_SESSION *);
extern int smtp_sasl_passwd_lookup(SMTP_SESSION *);
extern void smtp_sasl_start(SMTP_SESSION *, const char *, const char *);
extern int smtp_sasl_authenticate(SMTP_SESSION *, DSN_BUF *);
extern void smtp_sasl_cleanup(SMTP_SESSION *);

extern void smtp_sasl_helo_auth(SMTP_SESSION *, const char *);
extern int smtp_sasl_helo_login(SMTP_STATE *);

extern void smtp_sasl_passivate(SMTP_SESSION *, VSTRING *);
extern int smtp_sasl_activate(SMTP_SESSION *, char *);
extern STRING_LIST *smtp_sasl_mechs;

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Initial implementation by:
/*	Till Franke
/*	SuSE Rhein/Main AG
/*	65760 Eschborn, Germany
/*
/*	Adopted by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/
