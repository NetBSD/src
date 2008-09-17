#ifndef _VRFY_CLNT_H_INCLUDED_
#define _VRFY_CLNT_H_INCLUDED_

/*++
/* NAME
/*	verify_clnt 3h
/* SUMMARY
/*	address verification client interface
/* SYNOPSIS
/*	#include <verify_clnt.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Global library.
  */
#include <deliver_request.h>

 /*
  * Address verification requests.
  */
#define VRFY_REQ_QUERY		"query"
#define VRFY_REQ_UPDATE		"update"

 /*
  * Request (NOT: address) status codes.
  */
#define VRFY_STAT_OK		0
#define VRFY_STAT_FAIL		(-1)
#define VRFY_STAT_BAD		(-2)

 /*
  * Functional interface.
  */
extern int verify_clnt_query(const char *, int *, VSTRING *);
extern int verify_clnt_update(const char *, int, const char *);

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
