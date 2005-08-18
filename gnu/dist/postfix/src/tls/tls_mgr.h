/*	$NetBSD: tls_mgr.h,v 1.1.1.1 2005/08/18 21:11:06 rpaulo Exp $	*/

#ifndef _TLS_MGR_CLNT_H_INCLUDED_
#define _TLS_MGR_CLNT_H_INCLUDED_

/*++
/* NAME
/*	tls_mgr 3h
/* SUMMARY
/*	tlsmgr client interface
/* SYNOPSIS
/*	#include <tls_mgr.h>
/* DESCRIPTION
/* .nf

 /*
  * TLS manager protocol.
  */
#define TLS_MGR_SERVICE		"tlsmgr"
#define TLS_MGR_CLASS		"private"

#define TLS_MGR_ATTR_REQ	"request"
#define TLS_MGR_REQ_SEED	"seed"
#define TLS_MGR_REQ_POLICY	"policy"
#define TLS_MGR_REQ_LOOKUP	"lookup"
#define TLS_MGR_REQ_UPDATE	"update"
#define TLS_MGR_REQ_DELETE	"delete"
#define TLS_MGR_ATTR_POLICY	"policy"
#define TLS_MGR_ATTR_CACHE_TYPE	"cache_type"
#define TLS_MGR_ATTR_SEED	"seed"
#define TLS_MGR_ATTR_CACHE_ID	"cache_id"
#define TLS_MGR_ATTR_VERSION	"version"
#define TLS_MGR_ATTR_FLAGS	"flags"
#define TLS_MGR_ATTR_SESSION	"session"
#define TLS_MGR_ATTR_SIZE	"size"
#define TLS_MGR_ATTR_STATUS	"status"
#define TLS_MGR_ATTR_FLAGS	"flags"

 /*
  * TLS manager request status codes.
  */
#define TLS_MGR_STAT_OK		0	/* success */
#define TLS_MGR_STAT_ERR	(-1)	/* object not found */
#define TLS_MGR_STAT_FAIL	(-2)	/* protocol error */

 /*
  * Are we talking about the client or server cache?
  */
#define TLS_MGR_SCACHE_CLIENT	(1<<0)
#define TLS_MGR_SCACHE_SERVER	(1<<1)

 /*
  * Functional interface.
  */
extern int tls_mgr_seed(VSTRING *, int);
extern int tls_mgr_policy(int *);
extern int tls_mgr_lookup(int, const char *, long, int, VSTRING *);
extern int tls_mgr_update(int, const char *, long, int, const char *, int);
extern int tls_mgr_delete(int, const char *);

#define TLS_MGR_NO_FLAGS	0

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
