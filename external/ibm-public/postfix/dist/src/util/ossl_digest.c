/*	$NetBSD: ossl_digest.c,v 1.2.2.2 2026/05/11 17:14:04 martin Exp $	*/

/*++
/* NAME
/*	ossl_digest 3
/* SUMMARY
/*	OpenSSL message digest wrapper
/* SYNOPSIS
/*	#define USE_TLS
/*
/*	#include <ossl_digest.h>
/*
/*	OSSL_DGST *ossl_digest_new(
/*	const char *alg_name)
/*
/*	int	ossl_digest_data(
/*	OSSL_DGST *dgst,
/*	const void *data,
/*	ssize_t	data_len,
/*	VSTRING *out);
/*
/*	ARGV	*ossl_digest_get_errors(void)
/*
/*	ARGV	*ossl_digest_log_errors(
/*	void	(*logger)(const char *,...))
/*
/*	ssize_t	ossl_digest_get_size(
/*	OSSL_DGST *dgst)
/*
/*	void	ossl_digest_free(
/*	OSSL_DGST *dgst)
/* DESCRIPTION
/*	ossl_digest_new() allocates a wrapper for the named message
/*	digest algorithm. This wrapper can be used in multiple successive
/*	calls to compute a digest, and can be disposed of with
/*	ossl_digest_free().
/*
/*	ossl_digest_data() uses the specified message digest wrapper to
/*	compute a digest over the specified data.
/*
/*	ossl_digest_get_errors() dumps and clears the OpenSSL error stack.
/*	Each stack entry is copied to one ARGV element. NOTE: The caller
/*	should be prepared for the call to return an empty result,
/*	and always report their own error info.
/*
/*	ossl_digest_log_errors() logs and clears the OpenSSL error stack.
/*	Each stack entry is logged by the specified function. NOTE:
/*	The caller should be prepared for the call to return an empty
/*	result, and log their own error message.
/*
/*	ossl_digest_get_size() returns the output byte count for the
/*	specified message digest wrapper.
/*
/*	ossl_digest_free() releases storage allocated for or by the
/*	specified message wrapper.
/* DIAGNOSTICS
/*	Panic: ossl_digest_data() was called with an invalid data_len
/*	argument; an ossl_digest_free() argument was not created with
/*	ossl_digest_new().
/*
/*	ossl_digest_new() returns NULL after error. ossl_digest_data()
/*	returns 0 after success, -1 after error.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#ifdef USE_TLS

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * OpenSSL library.
  */
#include <openssl/err.h>
#include <openssl/evp.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <ossl_digest.h>
#include <vstring.h>

#ifndef OPENSSL_VERSION_PREREQ
#define OPENSSL_VERSION_PREREQ(m,n) 0
#endif

 /*
  * OpenSSL 1.1.1 compatibility crutches. Note: EVP_get_digestbyname()
  * returns const EVP_MD * which can't be passed to EVP_MD_free(EVP_MD *).
  */
#if !OPENSSL_VERSION_PREREQ(3,0)
#define EVP_MD_fetch(ct, alg_name, pr)	EVP_get_digestbyname(alg_name)
#define BC_CONST			const
#define EVP_MD_free(m)			/* */
#define EVP_MD_get_size			EVP_MD_size
#else
#define BC_CONST			/* */
#endif

 /*
  * Opaque object.
  */
struct OSSL_DGST {
    EVP_MD_CTX *mdctx;
    BC_CONST EVP_MD *dgst_alg;
};

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* ossl_digest_new - create OpenSSL digest wrapper */

OSSL_DGST *ossl_digest_new(const char *alg_name)
{
    OSSL_DGST *dgst = (OSSL_DGST *) mymalloc(sizeof(*dgst));

    /*
     * https://docs.openssl.org/3.3/man7/ossl-guide-libcrypto-introduction
     * "If you perform the same operation many times with the same algorithm
     * then it is recommended to use a single explicit fetch of the algorithm
     * and then reuse the explicitly fetched algorithm each subsequent time.
     * This will typically be faster than implicitly fetching the algorithm
     * every time you use it".
     * 
     * That same text mentions that calling, for example, EVP_sha256(3), uses
     * implicit fetching.
     */
    if ((dgst->dgst_alg = EVP_MD_fetch(NULL, alg_name, NULL)) != 0) {
	if ((dgst->mdctx = EVP_MD_CTX_new()) != 0) {
	    /* Success. */
	    return (dgst);
	}
	EVP_MD_free(dgst->dgst_alg);
    }
    /* Failure. */
    myfree(dgst);
    return (0);
}

/* ossl_digest_data - digest one data buffer */

int     ossl_digest_data(OSSL_DGST *dgst, const void *data,
			         ssize_t data_len, VSTRING *out)
{
    unsigned int out_len;

    if (data_len < 0)
	msg_panic("ossl_digest_data: bad data_len %ld", (long) data_len);

    VSTRING_RESET(out);
    VSTRING_SPACE(out, EVP_MD_get_size(dgst->dgst_alg));
    if (EVP_DigestInit_ex(dgst->mdctx, dgst->dgst_alg, 0) != 1
	|| EVP_DigestUpdate(dgst->mdctx, data, data_len) != 1
	|| EVP_DigestFinal_ex(dgst->mdctx, (void *) STR(out),
			      &out_len) != 1)
	return (-1);
    vstring_set_payload_size(out, out_len);
    return (0);
}

/* ossl_digest_get_size - determine digest output byte count */

ssize_t ossl_digest_get_size(OSSL_DGST *dgst)
{
    return (EVP_MD_get_size(dgst->dgst_alg));
}

/* ossl_digest_get_errors - export and clear OpenSSL error stack */

ARGV   *ossl_digest_get_errors(void)
{
    ARGV   *argv = argv_alloc(1);
    VSTRING *tmp = vstring_alloc(100);
    unsigned long err;
    char    buffer[1024];		/* XXX */
    const char *file;
    const char *data;
    int     line;
    int     flags;

    /*
     * Shamelessly copied from Postfix TLS library.
     */
#if OPENSSL_VERSION_PREREQ(3,0)
/* XXX: We're ignoring the function name, do we want to log it? */
#define ERRGET(fi, l, d, fl) ERR_get_error_all(fi, l, 0, d, fl)
#else
#define ERRGET(fi, l, d, fl) ERR_get_error_line_data(fi, l, d, fl)
#endif

    while ((err = ERRGET(&file, &line, &data, &flags)) != 0) {
	ERR_error_string_n(err, buffer, sizeof(buffer));
	if (flags & ERR_TXT_STRING)
	    vstring_sprintf(tmp, "%s:%s:%d:%s:",
			    buffer, file, line, data);
	else
	    vstring_sprintf(tmp, "%s:%s:%d:", buffer, file, line);
	argv_add(argv, STR(tmp), (char *) 0);
    }
    vstring_free(tmp);
    return (argv);
}

/* ossl_digest_log_errors - log and clear OpenSSL error stack */

void    ossl_digest_log_errors(void (*logger) (const char *,...))
{
    unsigned long err;
    char    buffer[1024];		/* XXX */
    const char *file;
    const char *data;
    int     line;
    int     flags;

    while ((err = ERRGET(&file, &line, &data, &flags)) != 0) {
	ERR_error_string_n(err, buffer, sizeof(buffer));
	if (flags & ERR_TXT_STRING)
	    logger("%s:%s:%d:%s:", buffer, file, line, data);
	else
	    logger("%s:%s:%d:", buffer, file, line);
    }
}

/* ossl_digest_free - dispose of digest wrapper */

void    ossl_digest_free(OSSL_DGST *dgst)
{
    EVP_MD_CTX_destroy(dgst->mdctx);
    EVP_MD_free(dgst->dgst_alg);
    myfree(dgst);
}

#endif					/* USE_TLS */
