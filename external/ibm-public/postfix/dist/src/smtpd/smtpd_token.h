/*	$NetBSD: smtpd_token.h,v 1.1.1.1 2009/06/23 10:08:56 tron Exp $	*/

/*++
/* NAME
/*	smtpd_token 3h
/* SUMMARY
/*	tokenize SMTPD command
/* SYNOPSIS
/*	#include <smtpd_token.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
typedef struct SMTPD_TOKEN {
    int     tokval;
    char   *strval;
    VSTRING *vstrval;
} SMTPD_TOKEN;

#define SMTPD_TOK_OTHER	0
#define SMTPD_TOK_ADDR	1
#define SMTPD_TOK_ERROR	2

extern int smtpd_token(char *, SMTPD_TOKEN **);

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
