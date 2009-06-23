/*	$NetBSD: anvil_clnt.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _ANVIL_CLNT_H_INCLUDED_
#define _ANVIL_CLNT_H_INCLUDED_

/*++
/* NAME
/*	anvil_clnt 3h
/* SUMMARY
/*	connection count and rate management client interface
/* SYNOPSIS
/*	#include <anvil_clnt.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include <attr_clnt.h>

 /*
  * Protocol interface: requests and endpoints.
  */
#define ANVIL_SERVICE		"anvil"
#define ANVIL_CLASS		"private"

#define ANVIL_ATTR_REQ		"request"
#define ANVIL_REQ_CONN		"connect"
#define ANVIL_REQ_DISC		"disconnect"
#define ANVIL_REQ_MAIL		"message"
#define ANVIL_REQ_RCPT		"recipient"
#define ANVIL_REQ_NTLS		"newtls"
#define ANVIL_REQ_NTLS_STAT	"newtls_status"
#define ANVIL_REQ_LOOKUP	"lookup"
#define ANVIL_ATTR_IDENT	"ident"
#define ANVIL_ATTR_COUNT	"count"
#define ANVIL_ATTR_RATE		"rate"
#define ANVIL_ATTR_MAIL		"mail"
#define ANVIL_ATTR_RCPT		"rcpt"
#define ANVIL_ATTR_NTLS		"newtls"
#define ANVIL_ATTR_STATUS	"status"

#define ANVIL_STAT_OK		0
#define ANVIL_STAT_FAIL		(-1)

 /*
  * Functional interface.
  */
typedef struct ANVIL_CLNT ANVIL_CLNT;

extern ANVIL_CLNT *anvil_clnt_create(void);
extern int anvil_clnt_connect(ANVIL_CLNT *, const char *, const char *, int *, int *);
extern int anvil_clnt_mail(ANVIL_CLNT *, const char *, const char *, int *);
extern int anvil_clnt_rcpt(ANVIL_CLNT *, const char *, const char *, int *);
extern int anvil_clnt_newtls(ANVIL_CLNT *, const char *, const char *, int *);
extern int anvil_clnt_newtls_stat(ANVIL_CLNT *, const char *, const char *, int *);
extern int anvil_clnt_lookup(ANVIL_CLNT *, const char *, const char *, int *, int *, int *, int *, int *);
extern int anvil_clnt_disconnect(ANVIL_CLNT *, const char *, const char *);
extern void anvil_clnt_free(ANVIL_CLNT *);

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

#endif
