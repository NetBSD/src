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
extern void smtp_sasl_connect(SMTP_STATE *);
extern int smtp_sasl_passwd_lookup(SMTP_STATE *);
extern void smtp_sasl_start(SMTP_STATE *, const char *, const char *);
extern int smtp_sasl_authenticate(SMTP_STATE *, VSTRING *);
extern void smtp_sasl_cleanup(SMTP_STATE *);

extern void smtp_sasl_helo_auth(SMTP_STATE *, const char *);
extern int smtp_sasl_helo_login(SMTP_STATE *);

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
