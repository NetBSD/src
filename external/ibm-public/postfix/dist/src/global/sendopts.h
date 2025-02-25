/*	$NetBSD: sendopts.h,v 1.2 2025/02/25 19:15:46 christos Exp $	*/

#ifndef _SENDOPTS_H_INCLUDED_
#define _SENDOPTS_H_INCLUDED_

/*++
/* NAME
/*	sendopts 3h
/* SUMMARY
/*	Support for SMTPUTF8, REQUIRETLS, etc.
/* SYNOPSIS
/*	#include <sendopts.h>
/* DESCRIPTION
/* .nf

 /*
  * Support for SMTPUTF8 (RFC 6531, RFC 6532, RFC 6533). These flags were
  * migrated from <smtputf8.h> and MUST NOT be changed, to maintain queue
  * file compatibility.
  */
#define SOPT_SMTPUTF8_NONE	(0)
#define SOPT_SMTPUTF8_REQUESTED	(1<<0)	/* queue file/delivery/bounce request */
#define SOPT_SMTPUTF8_HEADER	(1<<1)	/* queue file/delivery/bounce request */
#define SOPT_SMTPUTF8_SENDER	(1<<2)	/* queue file/delivery/bounce request */
#define SOPT_SMTPUTF8_RECIPIENT	(1<<3)	/* delivery request only */
#define SOPT_SMTPUTF8_ALL	(SOPT_SMTPUTF8_REQUESTED | \
				SOPT_SMTPUTF8_HEADER | \
				SOPT_SMTPUTF8_SENDER | \
				SOPT_SMTPUTF8_RECIPIENT)
#define SOPT_SMTPUTF8_DERIVED	\
	(SOPT_SMTPUTF8_ALL & ~SOPT_SMTPUTF8_REQUESTED)

 /*
  * Support for REQUIRETLS (RFC 8689). At this time only the TLS-Required:
  * header is implemented, but we reserve the flag that would support it.
  */
#define SOPT_REQUIRETLS_HEADER	(1<<4)	/* TLS-Required: no */
#define SOPT_REQUIRETLS_ESMTP	(1<<5)	/* MAIL FROM ... REQUIRETLS */
#define SOPT_REQUIRETLS_ALL	(SOPT_REQUIRETLS_HEADER | \
				 SOPT_REQUIRETLS_ESMTP)
#define SOPT_REQUIRETLS_DERIVED	SOPT_REQUIRETLS_HEADER

#define SOPT_FLAG_ALL	(SOPT_SMTPUTF8_ALL | SOPT_REQUIRETLS_ALL)
#define SOPT_FLAG_DERIVED (SOPT_SMTPUTF8_DERIVED | SOPT_REQUIRETLS_DERIVED)

 /*
  * Debug helper.
  */
extern const char *sendopts_strflags(unsigned flags, int delim);

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
/*	porcupine.org
/*--*/

#endif
