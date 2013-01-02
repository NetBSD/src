/*	$NetBSD: smtpd_sasl_proto.h,v 1.1.1.2 2013/01/02 18:59:10 tron Exp $	*/

/*++
/* NAME
/*	smtpd_sasl_proto 3h
/* SUMMARY
/*	Postfix SMTP protocol support for SASL authentication
/* SYNOPSIS
/*	#include "smtpd_sasl_proto.h"
/* DESCRIPTION
/* .nf

 /*
  * SMTP protocol interface.
  */
extern int smtpd_sasl_auth_cmd(SMTPD_STATE *, int, SMTPD_TOKEN *);
extern void smtpd_sasl_auth_reset(SMTPD_STATE *);
extern char *smtpd_sasl_mail_opt(SMTPD_STATE *, const char *);
extern void smtpd_sasl_mail_log(SMTPD_STATE *);
extern void smtpd_sasl_mail_reset(SMTPD_STATE *);

#define smtpd_sasl_auth_extern	smtpd_sasl_login
#define smtpd_sasl_auth_reset	smtpd_sasl_logout

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
