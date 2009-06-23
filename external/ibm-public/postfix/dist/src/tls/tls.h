/*	$NetBSD: tls.h,v 1.1.1.1 2009/06/23 10:08:57 tron Exp $	*/

#ifndef _TLS_H_INCLUDED_
#define _TLS_H_INCLUDED_

/*++
/* NAME
/*      tls 3h
/* SUMMARY
/*      libtls internal interfaces
/* SYNOPSIS
/*      #include <tls.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <name_code.h>
#include <argv.h>

 /*
  * TLS enforcement levels. Non-sentinel values may also be used to indicate
  * the actual security level of a session.
  * 
  * XXX TLS_LEV_NOTFOUND no longer belongs in this list. The SMTP client will
  * have to use something else to report that policy table lookup failed.
  */
#define TLS_LEV_INVALID		-2	/* sentinel */
#define TLS_LEV_NOTFOUND	-1	/* XXX not in policy table */
#define TLS_LEV_NONE		0	/* plain-text only */
#define TLS_LEV_MAY		1	/* wildcard */
#define TLS_LEV_ENCRYPT		2	/* encrypted connection */
#define TLS_LEV_FPRINT		3	/* "peer" CA-less verification */
#define TLS_LEV_VERIFY		4	/* certificate verified */
#define TLS_LEV_SECURE		5	/* "secure" verification */

extern const NAME_CODE tls_level_table[];

#define tls_level_lookup(s) name_code(tls_level_table, NAME_CODE_FLAG_NONE, (s))
#define str_tls_level(l) str_name_code(tls_level_table, (l))

#ifdef USE_TLS

 /*
  * OpenSSL library.
  */
#include <openssl/lhash.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
#error "need OpenSSL version 0.9.5 or later"
#endif

 /*
  * Utility library.
  */
#include <vstream.h>
#include <name_mask.h>
#include <name_code.h>

#define TLS_BIO_BUFSIZE	8192

 /*
  * Names of valid tlsmgr(8) session caches.
  */
#define TLS_MGR_SCACHE_SMTPD	"smtpd"
#define TLS_MGR_SCACHE_SMTP	"smtp"
#define TLS_MGR_SCACHE_LMTP	"lmtp"

 /*
  * TLS session context, also used by the VSTREAM call-back routines for SMTP
  * input/output, and by OpenSSL call-back routines for key verification.
  * 
  * Only some members are (read-only) accessible by the public.
  */
#define CCERT_BUFSIZ	256

typedef struct {
    /* Public, read-only. */
    char   *peer_CN;			/* Peer Common Name */
    char   *issuer_CN;			/* Issuer Common Name */
    char   *peer_fingerprint;		/* ASCII fingerprint */
    int     peer_status;		/* Certificate and match status */
    const char *protocol;
    const char *cipher_name;
    int     cipher_usebits;
    int     cipher_algbits;
    /* Private. */
    SSL    *con;
    BIO    *internal_bio;		/* postfix/TLS side of pair */
    BIO    *network_bio;		/* network side of pair */
    char   *cache_type;			/* tlsmgr(8) cache type if enabled */
    char   *serverid;			/* unique server identifier */
    char   *namaddr;			/* nam[addr] for logging */
    int     log_level;			/* TLS library logging level */
    int     session_reused;		/* this session was reused */
    int     am_server;			/* Are we an SSL server or client? */
} TLS_SESS_STATE;

 /*
  * Peer status bits. TLS_CERT_FLAG_MATCHED implies TLS_CERT_FLAG_TRUSTED
  * only in the case of a hostname match.
  */
#define TLS_CERT_FLAG_PRESENT		(1<<0)
#define TLS_CERT_FLAG_ALTNAME		(1<<1)
#define TLS_CERT_FLAG_TRUSTED		(1<<2)
#define TLS_CERT_FLAG_MATCHED		(1<<3)
#define TLS_CERT_FLAG_LOGGED		(1<<4)	/* Logged trust chain error */

#define TLS_CERT_IS_PRESENT(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_PRESENT))
#define TLS_CERT_IS_ALTNAME(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_ALTNAME))
#define TLS_CERT_IS_TRUSTED(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_TRUSTED))
#define TLS_CERT_IS_MATCHED(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_MATCHED))

 /*
  * Opaque client context handle.
  */
typedef struct TLS_APPL_STATE TLS_APPL_STATE;

#ifdef TLS_INTERNAL

 /*
  * Client and Server application contexts
  */
struct TLS_APPL_STATE {
    SSL_CTX *ssl_ctx;
    char   *cache_type;
    char   *cipher_exclusions;		/* Last cipher selection state */
    char   *cipher_list;		/* Last cipher selection state */
    int     cipher_grade;		/* Last cipher selection state */
    VSTRING *why;
};

 /*
  * tls_misc.c One time finalization of application context.
  */
extern void tls_free_app_context(TLS_APPL_STATE *);

 /*
  * tls_misc.c
  */

extern void tls_param_init(void);

 /*
  * Protocol selection.
  */
#define TLS_PROTOCOL_INVALID	(~0)	/* All protocol bits masked */
#define TLS_PROTOCOL_SSLv2	(1<<0)	/* SSLv2 */
#define TLS_PROTOCOL_SSLv3	(1<<1)	/* SSLv3 */
#define TLS_PROTOCOL_TLSv1	(1<<2)	/* TLSv1 */
#define TLS_KNOWN_PROTOCOLS	\
	( TLS_PROTOCOL_SSLv2 | TLS_PROTOCOL_SSLv3 | TLS_PROTOCOL_TLSv1 )

extern int tls_protocol_mask(const char *);

 /*
  * Cipher grade selection.
  */
#define TLS_CIPHER_NONE		0
#define TLS_CIPHER_NULL		1
#define TLS_CIPHER_EXPORT	2
#define TLS_CIPHER_LOW		3
#define TLS_CIPHER_MEDIUM	4
#define TLS_CIPHER_HIGH		5

extern const NAME_CODE tls_cipher_grade_table[];

#define tls_cipher_grade(str) \
    name_code(tls_cipher_grade_table, NAME_CODE_FLAG_NONE, (str))
#define str_tls_cipher_grade(gr) \
    str_name_code(tls_cipher_grade_table, (gr))

 /*
  * Cipher lists with exclusions.
  */
extern const char *tls_set_ciphers(TLS_APPL_STATE *, const char *,
				           const char *, const char *);

#endif

 /*
  * tls_client.c
  */
typedef struct {
    int     log_level;
    int     verifydepth;
    const char *cache_type;
    const char *cert_file;
    const char *key_file;
    const char *dcert_file;
    const char *dkey_file;
    const char *eccert_file;
    const char *eckey_file;
    const char *CAfile;
    const char *CApath;
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_CLIENT_INIT_PROPS;

typedef struct {
    TLS_APPL_STATE *ctx;
    VSTREAM *stream;
    int     log_level;
    int     timeout;
    int     tls_level;			/* Security level */
    const char *nexthop;		/* destination domain */
    const char *host;			/* MX hostname */
    const char *namaddr;		/* nam[addr] for logging */
    const char *serverid;		/* Session cache key */
    const char *protocols;		/* Enabled protocols */
    const char *cipher_grade;		/* Minimum cipher grade */
    const char *cipher_exclusions;	/* Ciphers to exclude */
    const ARGV *matchargv;		/* Cert match patterns */
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_CLIENT_START_PROPS;

extern TLS_APPL_STATE *tls_client_init(const TLS_CLIENT_INIT_PROPS *);
extern TLS_SESS_STATE *tls_client_start(const TLS_CLIENT_START_PROPS *);

#define tls_client_stop(ctx, stream, timeout, failure, TLScontext) \
	tls_session_stop(ctx, (stream), (timeout), (failure), (TLScontext))

#define TLS_CLIENT_INIT(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12) \
    tls_client_init((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), (props)))

#define TLS_CLIENT_START(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12, a13, a14) \
    tls_client_start((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14), (props)))

 /*
  * tls_server.c
  */
typedef struct {
    int     log_level;
    int     verifydepth;
    const char *cache_type;
    long    scache_timeout;
    int     set_sessid;
    const char *cert_file;
    const char *key_file;
    const char *dcert_file;
    const char *dkey_file;
    const char *eccert_file;
    const char *eckey_file;
    const char *CAfile;
    const char *CApath;
    const char *protocols;
    const char *eecdh_grade;
    const char *dh1024_param_file;
    const char *dh512_param_file;
    int     ask_ccert;
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_SERVER_INIT_PROPS;

typedef struct {
    TLS_APPL_STATE *ctx;		/* TLS application context */
    VSTREAM *stream;			/* Client stream */
    int     log_level;			/* TLS log level */
    int     timeout;			/* TLS handshake timeout */
    int     requirecert;		/* Insist on client cert? */
    const char *serverid;		/* Server instance (salt cache key) */
    const char *namaddr;		/* Client nam[addr] for logging */
    const char *cipher_grade;
    const char *cipher_exclusions;
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_SERVER_START_PROPS;

extern TLS_APPL_STATE *tls_server_init(const TLS_SERVER_INIT_PROPS *);
extern TLS_SESS_STATE *tls_server_start(const TLS_SERVER_START_PROPS *props);

#define tls_server_stop(ctx, stream, timeout, failure, TLScontext) \
	tls_session_stop(ctx, (stream), (timeout), (failure), (TLScontext))

#define TLS_SERVER_INIT(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
    tls_server_init((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14), ((props)->a15), \
    ((props)->a16), ((props)->a17), ((props)->a18), ((props)->a19), (props)))

#define TLS_SERVER_START(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
    tls_server_start((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), (props)))

 /*
  * tls_session.c
  */
extern void tls_session_stop(TLS_APPL_STATE *, VSTREAM *, int, int, TLS_SESS_STATE *);

#ifdef TLS_INTERNAL

#include <vstring.h>

extern VSTRING *tls_session_passivate(SSL_SESSION *);
extern SSL_SESSION *tls_session_activate(const char *, int);

 /*
  * tls_stream.c.
  */
extern void tls_stream_start(VSTREAM *, TLS_SESS_STATE *);
extern void tls_stream_stop(VSTREAM *);

 /*
  * tls_bio_ops.c: a generic multi-personality driver that retries SSL
  * operations until they are satisfied or until a hard error happens.
  * Because of its ugly multi-personality user interface we invoke it via
  * not-so-ugly single-personality wrappers.
  */
extern int tls_bio(int, int, TLS_SESS_STATE *,
		           int (*) (SSL *),	/* handshake */
		           int (*) (SSL *, void *, int),	/* read */
		           int (*) (SSL *, const void *, int),	/* write */
		           void *, int);

#define tls_bio_connect(fd, timeout, context) \
        tls_bio((fd), (timeout), (context), SSL_connect, \
		NULL, NULL, NULL, 0)
#define tls_bio_accept(fd, timeout, context) \
        tls_bio((fd), (timeout), (context), SSL_accept, \
		NULL, NULL, NULL, 0)
#define tls_bio_shutdown(fd, timeout, context) \
	tls_bio((fd), (timeout), (context), SSL_shutdown, \
		NULL, NULL, NULL, 0)
#define tls_bio_read(fd, buf, len, timeout, context) \
	tls_bio((fd), (timeout), (context), NULL, \
		SSL_read, NULL, (buf), (len))
#define tls_bio_write(fd, buf, len, timeout, context) \
	tls_bio((fd), (timeout), (context), NULL, \
		NULL, SSL_write, (buf), (len))

 /*
  * tls_dh.c
  */
extern void tls_set_dh_from_file(const char *, int);
extern DH *tls_tmp_dh_cb(SSL *, int, int);
extern int tls_set_eecdh_curve(SSL_CTX *, const char *);

 /*
  * tls_rsa.c
  */
extern RSA *tls_tmp_rsa_cb(SSL *, int, int);

 /*
  * tls_verify.c
  */
extern char *tls_peer_CN(X509 *, const TLS_SESS_STATE *);
extern char *tls_issuer_CN(X509 *, const TLS_SESS_STATE *);
extern const char *tls_dns_name(const GENERAL_NAME *, const TLS_SESS_STATE *);
extern char *tls_fingerprint(X509 *, const char *);
extern int tls_verify_certificate_callback(int, X509_STORE_CTX *);

 /*
  * tls_certkey.c
  */
extern int tls_set_ca_certificate_info(SSL_CTX *, const char *, const char *);
extern int tls_set_my_certificate_key_info(SSL_CTX *,
				       /* RSA */ const char *, const char *,
				       /* DSA */ const char *, const char *,
				    /* ECDSA */ const char *, const char *);

 /*
  * tls_misc.c
  */
extern int TLScontext_index;

extern TLS_APPL_STATE *tls_alloc_app_context(SSL_CTX *);
extern TLS_SESS_STATE *tls_alloc_sess_context(int, const char *);
extern void tls_free_context(TLS_SESS_STATE *);
extern void tls_check_version(void);
extern long tls_bug_bits(void);
extern void tls_print_errors(void);
extern void tls_info_callback(const SSL *, int, int);
extern long tls_bio_dump_cb(BIO *, int, const char *, int, long, long);

 /*
  * tls_seed.c
  */
extern void tls_int_seed(void);
extern int tls_ext_seed(int);

#endif					/* TLS_INTERNAL */

/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      IBM T.J. Watson Research
/*      P.O. Box 704
/*      Yorktown Heights, NY 10598, USA
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

#endif					/* USE_TLS */
#endif					/* _TLS_H_INCLUDED_ */
