/*	$NetBSD: dict_proxy.h,v 1.2.22.1 2023/12/25 12:43:31 martin Exp $	*/

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
#include <mkmap.h>

 /*
  * External interface.
  */
#define DICT_TYPE_PROXY	"proxy"

extern DICT *dict_proxy_open(const char *, int, int);
extern MKMAP *mkmap_proxy_open(const char *);

 /*
  * Protocol interface.
  */
#define PROXY_REQ_OPEN		"open"
#define PROXY_REQ_LOOKUP	"lookup"
#define PROXY_REQ_UPDATE	"update"
#define PROXY_REQ_DELETE	"delete"
#define PROXY_REQ_SEQUENCE	"sequence"

#define PROXY_STAT_OK		0	/* operation succeeded */
#define PROXY_STAT_NOKEY	1	/* requested key not found */
#define PROXY_STAT_RETRY	2	/* try lookup again later */
#define PROXY_STAT_BAD		3	/* invalid request parameter */
#define PROXY_STAT_DENY		4	/* table not approved for proxying */
#define PROXY_STAT_CONFIG	5	/* DICT_ERR_CONFIG error */

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
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
