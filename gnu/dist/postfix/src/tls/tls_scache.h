/*	$NetBSD: tls_scache.h,v 1.1.1.1 2005/08/18 21:11:08 rpaulo Exp $	*/

#ifndef _TLS_SCACHE_H_INCLUDED_
#define _TLS_SCACHE_H_INCLUDED_

/*++
/* NAME
/*	tls_scache 3h
/* SUMMARY
/*	TLS session cache manager
/* SYNOPSIS
/*	#include <tls_scache.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>
#include <vstring.h>

 /*
  * External interface.
  */
typedef struct {
    int     flags;			/* see below */
    DICT   *db;				/* database handle */
    char   *cache_label;		/* "client" or "server" */
    int     log_level;			/* smtp(d)_tls_log_level */
    int     timeout;			/* smtp(d)_tls_session_cache_timeout */
    char   *saved_cursor;		/* cursor cache ID */
    long    saved_openssl_version;	/* cursor OpenSSL version */
    int     saved_flags;		/* cursor lookup flags */
} TLS_SCACHE;

#define TLS_SCACHE_FLAG_DEL_SAVED_CURSOR	(1<<0)

extern TLS_SCACHE *tls_scache_open(const char *, const char *, int, int);
extern void tls_scache_close(TLS_SCACHE *);
extern int tls_scache_lookup(TLS_SCACHE *, const char *, long, int, long *, int *, VSTRING *);
extern int tls_scache_update(TLS_SCACHE *, const char *, long, int, const char *, int);
extern int tls_scache_delete(TLS_SCACHE *, const char *);
extern int tls_scache_sequence(TLS_SCACHE *, int, long, int, char **, long *, int *, VSTRING *);

#define TLS_SCACHE_ANY_OPENSSL_VSN		((long) 0)
#define TLS_SCACHE_ANY_FLAGS			(0)

#define TLS_SCACHE_DONT_NEED_CACHE_ID		((char **) 0)
#define TLS_SCACHE_DONT_NEED_OPENSSL_VSN	((long *) 0)
#define TLS_SCACHE_DONT_NEED_FLAGS		((int *) 0)
#define TLS_SCACHE_DONT_NEED_SESSION		((VSTRING *) 0)

#define TLS_SCACHE_SEQUENCE_NOTHING \
	TLS_SCACHE_ANY_FLAGS, TLS_SCACHE_ANY_OPENSSL_VSN, \
	TLS_SCACHE_DONT_NEED_CACHE_ID, TLS_SCACHE_DONT_NEED_OPENSSL_VSN, \
	TLS_SCACHE_DONT_NEED_FLAGS, TLS_SCACHE_DONT_NEED_SESSION

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
