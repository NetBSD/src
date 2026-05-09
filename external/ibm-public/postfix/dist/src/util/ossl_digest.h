/*	$NetBSD: ossl_digest.h,v 1.2 2026/05/09 18:49:23 christos Exp $	*/

#ifndef _OSSL_DIGEST_H_INCLUDED_
#define _OSSL_DIGEST_H_INCLUDED_

#ifdef USE_TLS

/*++
/* NAME
/*	ossl_digest 3h
/* SUMMARY
/*	OpenSSL message digest wrapper
/* SYNOPSIS
/*	#include <ossl_digest.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys_defs.h>

/*
 * Utility library.
 */
#include <argv.h>
#include <vstring.h>

/*
 * External interface.
 */
typedef struct OSSL_DGST OSSL_DGST;

extern OSSL_DGST *ossl_digest_new(const char *);
extern int ossl_digest_data(OSSL_DGST *, const void *data, ssize_t data_len,
			            VSTRING *out);
extern ARGV *ossl_digest_get_errors(void);
extern void ossl_digest_log_errors(void (*logger) (const char *,...));
extern ssize_t ossl_digest_get_size(OSSL_DGST *);
extern void ossl_digest_free(OSSL_DGST *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif					/* USE_TLS */

#endif					/* _OSSL_DIGEST_H_INCLUDED_ */
