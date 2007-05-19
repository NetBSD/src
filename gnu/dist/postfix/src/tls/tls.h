/*	$NetBSD: tls.h,v 1.1.1.4 2007/05/19 16:28:30 heas Exp $	*/

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

 /*
  * TLS enforcement levels. Non-sentinel values may also be used to indicate
  * the actual security level of a session.
  */
#define TLS_LEV_NOTFOUND	-1	/* sentinel */
#define TLS_LEV_NONE		0	/* plain-text only */
#define TLS_LEV_MAY		1	/* wildcard */
#define TLS_LEV_ENCRYPT		2	/* encrypted connection */
#define TLS_LEV_VERIFY		3	/* certificate verified */
#define TLS_LEV_SECURE		4	/* "secure" verification */

extern NAME_CODE tls_level_table[];

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
  */
#define CCERT_BUFSIZ	256

typedef struct {
    SSL    *con;
    BIO    *internal_bio;		/* postfix/TLS side of pair */
    BIO    *network_bio;		/* network side of pair */
    char   *cache_type;			/* tlsmgr(8) cache type if enabled */
    char   *serverid;			/* unique server identifier */
    char   *peer_CN;			/* Peer Common Name */
    char   *issuer_CN;			/* Issuer Common Name */
    char   *peer_fingerprint;		/* ASCII fingerprint */
    char   *peername;
    int     enforce_verify_errors;
    int     enforce_CN;
    int     hostname_matched;
    int     peer_verified;
    const char *protocol;
    const char *cipher_name;
    int     cipher_usebits;
    int     cipher_algbits;
    int     log_level;			/* TLS library logging level */
    int     session_reused;		/* this session was reused */
} TLScontext_t;

 /*
  * Client protocol selection bitmask
  */
#define TLS_PROTOCOL_SSLv2	(1<<0)	/* SSLv2 */
#define TLS_PROTOCOL_SSLv3	(1<<1)	/* SSLv3 */
#define TLS_PROTOCOL_TLSv1	(1<<2)	/* TLSv1 */
#define TLS_ALL_PROTOCOLS	\
	( TLS_PROTOCOL_SSLv2 | TLS_PROTOCOL_SSLv3 | TLS_PROTOCOL_TLSv1 )

 /*
  * tls_misc.c
  */
#define TLS_CIPHER_NONE		0
#define TLS_CIPHER_NULL		1
#define TLS_CIPHER_EXPORT	2
#define TLS_CIPHER_LOW		3
#define TLS_CIPHER_MEDIUM	4
#define TLS_CIPHER_HIGH		5

extern NAME_MASK tls_protocol_table[];
extern NAME_CODE tls_cipher_level_table[];

#define tls_protocol_mask(tag, protocols) \
    name_mask_delim_opt((tag), tls_protocol_table, (protocols), \
		        ":" NAME_MASK_DEFAULT_DELIM, \
		        NAME_MASK_ANY_CASE | NAME_MASK_RETURN)

#define tls_protocol_names(tag, mask) \
    str_name_mask_opt((VSTRING *)0, (tag), tls_protocol_table, (mask), \
		      NAME_MASK_FATAL|NAME_MASK_COMMA)

#define tls_cipher_level(str) \
    name_code(tls_cipher_level_table, NAME_CODE_FLAG_NONE, (str))

#define TLS_END_EXCLUDE ((char *)0)
extern const char *tls_cipher_list(int,...);
extern const char *tls_set_cipher_list(SSL_CTX *, const char *);

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
    const char *CAfile;
    const char *CApath;
} tls_client_init_props;

typedef struct {
    SSL_CTX *ctx;
    VSTREAM *stream;
    int     log_level;
    int     timeout;
    int     tls_level;			/* Security level */
    char   *nexthop;			/* destination domain */
    char   *host;			/* MX hostname */
    char   *serverid;			/* Session cache key */
    int     protocols;			/* Encrypt level protocols, 0 => all */
    char   *cipherlist;			/* Encrypt level ciphers */
    char   *certmatch;			/* Verify level match patterns */
} tls_client_start_props;

extern SSL_CTX *tls_client_init(const tls_client_init_props *);
extern TLScontext_t *tls_client_start(const tls_client_start_props *);

#define tls_client_stop(ctx , stream, timeout, failure, TLScontext) \
	tls_session_stop((ctx), (stream), (timeout), (failure), (TLScontext))

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
    const char *CAfile;
    const char *CApath;
    const char *cipherlist;
    int     protocols;			/* protocols, 0 => all */
    const char *dh1024_param_file;
    const char *dh512_param_file;
    int     ask_ccert;
} tls_server_props;

typedef struct {
    SSL_CTX *ctx;			/* SSL application context */
    VSTREAM *stream;			/* Client stream */
    int     log_level;			/* TLS log level */
    int     timeout;			/* TLS handshake timeout */
    int     requirecert;		/* Insist on client cert? */
    char   *serverid;			/* Server instance (salt cache key) */
    char   *peername;			/* Client name */
    char   *peeraddr;			/* Client address */
} tls_server_start_props;

extern SSL_CTX *tls_server_init(const tls_server_props *);
extern TLScontext_t *tls_server_start(const tls_server_start_props *props);

#define tls_server_stop(ctx , stream, timeout, failure, TLScontext) \
	tls_session_stop((ctx), (stream), (timeout), (failure), (TLScontext))

 /*
  * tls_session.c
  */
extern void tls_session_stop(SSL_CTX *, VSTREAM *, int, int,
			             TLScontext_t *);

#ifdef TLS_INTERNAL

#include <vstring.h>

extern VSTRING *tls_session_passivate(SSL_SESSION *);
extern SSL_SESSION *tls_session_activate(const char *, int);

 /*
  * tls_stream.c.
  */
extern void tls_stream_start(VSTREAM *, TLScontext_t *);
extern void tls_stream_stop(VSTREAM *);

 /*
  * tls_bio_ops.c: a generic multi-personality driver that retries SSL
  * operations until they are satisfied or until a hard error happens.
  * Because of its ugly multi-personality user interface we invoke it via
  * not-so-ugly single-personality wrappers.
  */
extern int tls_bio(int, int, TLScontext_t *,
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
extern void tls_set_dh_1024_from_file(const char *);
extern void tls_set_dh_512_from_file(const char *);
extern DH *tls_tmp_dh_cb(SSL *, int, int);

 /*
  * tls_rsa.c
  */
extern RSA *tls_tmp_rsa_cb(SSL *, int, int);

 /*
  * tls_verify.c
  */
extern char *tls_peer_CN(X509 *);
extern char *tls_issuer_CN(X509 *);
extern int tls_verify_certificate_callback(int, X509_STORE_CTX *);

 /*
  * tls_certkey.c
  */
extern int tls_set_ca_certificate_info(SSL_CTX *, const char *, const char *);
extern int tls_set_my_certificate_key_info(SSL_CTX *, const char *,
					           const char *,
					           const char *,
					           const char *);

 /*
  * tls_misc.c
  */
extern int TLScontext_index;
extern int TLSscache_index;

extern TLScontext_t *tls_alloc_context(int, const char *);
extern void tls_free_context(TLScontext_t *);
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
/*--*/

#endif					/* USE_TLS */
#endif					/* _TLS_H_INCLUDED_ */
