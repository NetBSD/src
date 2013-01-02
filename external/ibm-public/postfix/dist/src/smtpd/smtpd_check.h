/*	$NetBSD: smtpd_check.h,v 1.1.1.2 2013/01/02 18:59:09 tron Exp $	*/

/*++
/* NAME
/*	smtpd_check 3h
/* SUMMARY
/*	SMTP client request filtering
/* SYNOPSIS
/*	#include "smtpd.h"
/*	#include "smtpd_check.h"
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void smtpd_check_init(void);
extern int smtpd_check_addr(const char *);
extern char *smtpd_check_rewrite(SMTPD_STATE *);
extern char *smtpd_check_client(SMTPD_STATE *);
extern char *smtpd_check_helo(SMTPD_STATE *, char *);
extern char *smtpd_check_mail(SMTPD_STATE *, char *);
extern char *smtpd_check_size(SMTPD_STATE *, off_t);
extern char *smtpd_check_queue(SMTPD_STATE *);
extern char *smtpd_check_rcpt(SMTPD_STATE *, char *);
extern char *smtpd_check_etrn(SMTPD_STATE *, char *);
extern char *smtpd_check_data(SMTPD_STATE *);
extern char *smtpd_check_eod(SMTPD_STATE *);
extern char *smtpd_check_policy(SMTPD_STATE *, char *);

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
