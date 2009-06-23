/*	$NetBSD: dict_proxy.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _DICT_PROXY_H_INCLUDED_
#define _DICT_PROXY_H_INCLUDED_

/*++
/* NAME
/*	dict_proxy 3h
/* SUMMARY
/*	dictionary manager interface to PROXY maps
/* SYNOPSIS
/*	#include <dict_proxy.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_PROXY	"proxy"

extern DICT *dict_proxy_open(const char *, int, int);

 /*
  * Protocol interface.
  */
#define PROXY_REQ_OPEN		"open"
#define PROXY_REQ_LOOKUP	"lookup"
#define PROXY_REQ_UPDATE	"update"
#define PROXY_REQ_DELETE	"delete"

#define PROXY_STAT_OK		0	/* operation succeeded */
#define PROXY_STAT_NOKEY	1	/* requested key not found */
#define PROXY_STAT_RETRY	2	/* try lookup again later */
#define PROXY_STAT_BAD		3	/* invalid request parameter */
#define PROXY_STAT_DENY		4	/* table not approved for proxying */

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
