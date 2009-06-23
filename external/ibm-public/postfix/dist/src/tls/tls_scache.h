/*	$NetBSD: tls_scache.h,v 1.1.1.1 2009/06/23 10:08:57 tron Exp $	*/

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
    char   *cache_label;		/* "smtpd", "smtp" or "lmtp" */
    int     verbose;			/* enable verbose logging */
    int     timeout;			/* smtp(d)_tls_session_cache_timeout */
    char   *saved_cursor;		/* cursor cache ID */
} TLS_SCACHE;

#define TLS_SCACHE_FLAG_DEL_SAVED_CURSOR	(1<<0)

extern TLS_SCACHE *tls_scache_open(const char *, const char *, int, int);
extern void tls_scache_close(TLS_SCACHE *);
extern int tls_scache_lookup(TLS_SCACHE *, const char *, VSTRING *);
extern int tls_scache_update(TLS_SCACHE *, const char *, const char *, ssize_t);
extern int tls_scache_delete(TLS_SCACHE *, const char *);
extern int tls_scache_sequence(TLS_SCACHE *, int, char **, VSTRING *);

#define TLS_SCACHE_DONT_NEED_CACHE_ID		((char **) 0)
#define TLS_SCACHE_DONT_NEED_SESSION		((VSTRING *) 0)

#define TLS_SCACHE_SEQUENCE_NOTHING \
	TLS_SCACHE_DONT_NEED_CACHE_ID, TLS_SCACHE_DONT_NEED_SESSION

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
