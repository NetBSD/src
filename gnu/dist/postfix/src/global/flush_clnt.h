#ifndef _FLUSH_CLNT_H_INCLUDED_
#define _FLUSH_CLNT_H_INCLUDED_

/*++
/* NAME
/*	flush_clnt 3h
/* SUMMARY
/*	flush backed up mail
/* SYNOPSIS
/*	#include <flush_clnt.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int flush_add(const char *, const char *);
extern int flush_send(const char *);
extern int flush_refresh(void);
extern int flush_purge(void);

 /*
  * Mail flush server requests.
  */
#define FLUSH_REQ_ADD		"add"	/* append queue ID to site log */
#define FLUSH_REQ_SEND		"send"	/* flush mail queued for site */
#define FLUSH_REQ_REFRESH	"rfrsh"	/* refresh old logfiles */
#define FLUSH_REQ_PURGE		"purge"	/* refresh all logfiles */

 /*
  * Mail flush server status codes.
  */
#define FLUSH_STAT_FAIL		-1	/* request failed */
#define FLUSH_STAT_OK		0	/* request executed */
#define FLUSH_STAT_BAD		3	/* invalid parameter */


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
