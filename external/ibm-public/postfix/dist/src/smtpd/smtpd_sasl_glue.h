/*	$NetBSD: smtpd_sasl_glue.h,v 1.1.1.2 2013/01/02 18:59:10 tron Exp $	*/

/*++
/* NAME
/*	smtpd_sasl_glue 3h
/* SUMMARY
/*	Postfix SMTP server, SASL support interface
/* SYNOPSIS
/*	#include "smtpd_sasl_glue.h"
/* DESCRIPTION
/* .nf

 /*
  * SASL protocol interface
  */
extern void smtpd_sasl_state_init(SMTPD_STATE *);
extern void smtpd_sasl_initialize(void);
extern void smtpd_sasl_activate(SMTPD_STATE *, const char *, const char *);
extern void smtpd_sasl_deactivate(SMTPD_STATE *);
extern int smtpd_sasl_authenticate(SMTPD_STATE *, const char *, const char *);
extern void smtpd_sasl_login(SMTPD_STATE *, const char *, const char *);
extern void smtpd_sasl_logout(SMTPD_STATE *);
extern int permit_sasl_auth(SMTPD_STATE *, int, int);

#define smtpd_sasl_is_active(s)		((s)->sasl_server != 0)
#define smtpd_sasl_set_inactive(s)	((void) ((s)->sasl_server = 0))

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
