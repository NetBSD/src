/*++
/* NAME
/*	smtpd_sasl_glue 3h
/* SUMMARY
/*	Postfix SMTP server, SASL support interface
/* SYNOPSIS
/*	#include "smtpd_sasl.h"
/* DESCRIPTION
/* .nf

 /*
  * SASL protocol interface
  */
extern void smtpd_sasl_initialize(void);
extern void smtpd_sasl_connect(SMTPD_STATE *, const char *, const char *);
extern void smtpd_sasl_disconnect(SMTPD_STATE *);
extern char *smtpd_sasl_authenticate(SMTPD_STATE *, const char *, const char *);
extern void smtpd_sasl_logout(SMTPD_STATE *);
extern int permit_sasl_auth(SMTPD_STATE *, int, int);

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
