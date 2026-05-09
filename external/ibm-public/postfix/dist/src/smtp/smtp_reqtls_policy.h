/*	$NetBSD: smtp_reqtls_policy.h,v 1.1.1.1 2026/05/09 18:39:21 christos Exp $	*/

#ifndef _SMTP_REQTLS_POLICY_INCLUDED_
#define _SMTP_REQTLS_POLICY_INCLUDED_

/*++
/* NAME
/*	smtp_reqtls_policy 3h
/* SUMMARY
/*	requiretls per-mx policy
/* SYNOPSIS
/*	#include <smtp_reqtls_policy.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct SMTP_REQTLS_POLICY SMTP_REQTLS_POLICY;

extern SMTP_REQTLS_POLICY *smtp_reqtls_policy_parse(const char *, const char *);
extern int smtp_reqtls_policy_eval(SMTP_REQTLS_POLICY *, const char *);
extern void smtp_reqtls_policy_free(SMTP_REQTLS_POLICY *);

#define SMTP_REQTLS_POLICY_NAME_ENFORCE		"enforce"
#define SMTP_REQTLS_POLICY_NAME_OPP_TLS		"opportunistic+starttls"
#define SMTP_REQTLS_POLICY_NAME_OPPORTUNISTIC	"opportunistic"
#define SMTP_REQTLS_POLICY_NAME_DISABLE		"disable"
#define SMTP_REQTLS_POLICY_NAME_ERROR		"error"

#define SMTP_REQTLS_POLICY_NAME_DEFAULT		SMTP_REQTLS_POLICY_NAME_ENFORCE

#define SMTP_REQTLS_POLICY_ACT_ENFORCE		3
#define SMTP_REQTLS_POLICY_ACT_OPP_TLS		2
#define SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC	1
#define SMTP_REQTLS_POLICY_ACT_DISABLE		0
#define SMTP_REQTLS_POLICY_ACT_ERROR		(-1)

#define SMTP_REQTLS_POLICY_ACT_DEFAULT		SMTP_REQTLS_POLICY_ACT_ENFORCE

#define STATE_REQTLS_IS_REQUESTED(var, state) \
	SENDOPTS_REQTLS_IS_REQUESTED((var), (state)->request->sendopts)

#define SENDOPTS_REQTLS_IS_REQUESTED(var, sendopts) \
	((var) && (sendopts) & SOPT_REQUIRETLS_ESMTP)

#define TLS_REQUIRED_BY_REQTLS_POLICY(reqtls_level) \
	((reqtls_level) >= SMTP_REQTLS_POLICY_ACT_OPP_TLS)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
