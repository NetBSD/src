/*	$NetBSD: tls.h,v 1.1.1.3.2.2 2014/08/19 23:59:44 tls Exp $	*/

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
  * 
  * The order of levels matters, but we hide most of the details in macros.
  * 
  * "dane" vs. "fingerprint", both must lie between "encrypt" and "verify".
  * 
  * - With "may" and higher, TLS is enabled.
  * 
  * - With "encrypt" and higher, TLS encryption must be applied.
  * 
  * - Strictly above "encrypt", the peer certificate must match.
  * 
  * - At "dane" and higher, the peer certificate must also be trusted. With
  * "dane" the trust may be self-asserted, so we only log trust verification
  * errors when TA associations are involved.
  */
#define TLS_LEV_INVALID		-2	/* sentinel */
#define TLS_LEV_NOTFOUND	-1	/* XXX not in policy table */
#define TLS_LEV_NONE		0	/* plain-text only */
#define TLS_LEV_MAY		1	/* wildcard */
#define TLS_LEV_ENCRYPT		2	/* encrypted connection */
#define TLS_LEV_FPRINT		3	/* "peer" CA-less verification */
#define TLS_LEV_DANE		4	/* Opportunistic TLSA policy */
#define TLS_LEV_DANE_ONLY	5	/* Required TLSA policy */
#define TLS_LEV_VERIFY		6	/* certificate verified */
#define TLS_LEV_SECURE		7	/* "secure" verification */

#define TLS_REQUIRED(l)		((l) > TLS_LEV_MAY)
#define TLS_MUST_MATCH(l)	((l) > TLS_LEV_ENCRYPT)
#define TLS_MUST_TRUST(l)	((l) >= TLS_LEV_DANE)
#define TLS_MUST_PKIX(l)	((l) >= TLS_LEV_VERIFY)

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

 /* Appease indent(1) */
#define x509_stack_t STACK_OF(X509)
#define x509_extension_stack_t STACK_OF(X509_EXTENSION)
#define general_name_stack_t STACK_OF(GENERAL_NAME)
#define ssl_cipher_stack_t STACK_OF(SSL_CIPHER)
#define ssl_comp_stack_t STACK_OF(SSL_COMP)

#if (OPENSSL_VERSION_NUMBER < 0x00090700f)
#error "need OpenSSL version 0.9.7 or later"
#endif

/* SSL_CIPHER_get_name() got constified in 0.9.7g */
#if OPENSSL_VERSION_NUMBER >= 0x0090707fL	/* constification */
#define SSL_CIPHER_const const
#else
#define SSL_CIPHER_const
#endif

/* d2i_X509() got constified in 0.9.8a */
#if OPENSSL_VERSION_NUMBER >= 0x0090801fL
#define D2I_const const
#else
#define D2I_const
#endif

 /*
  * Utility library.
  */
#include <vstream.h>
#include <name_mask.h>
#include <name_code.h>
#include <dns.h>

 /*
  * Names of valid tlsmgr(8) session caches.
  */
#define TLS_MGR_SCACHE_SMTPD	"smtpd"
#define TLS_MGR_SCACHE_SMTP	"smtp"
#define TLS_MGR_SCACHE_LMTP	"lmtp"

 /*
  * RFC 6698 DANE
  */
#define TLS_DANE_TA	0		/* Match trust-anchor digests */
#define TLS_DANE_EE	1		/* Match end-entity digests */

#define TLS_DANE_CERT	0		/* Match the certificate digest */
#define TLS_DANE_PKEY	1		/* Match the public key digest */

#define TLS_DANE_FLAG_NORRS	(1<<0)	/* Nothing found in DNS */
#define TLS_DANE_FLAG_EMPTY	(1<<1)	/* Nothing usable found in DNS */
#define TLS_DANE_FLAG_ERROR	(1<<2)	/* TLSA record lookup error */

#define tls_dane_unusable(dane)	((dane)->flags & TLS_DANE_FLAG_EMPTY)
#define tls_dane_notfound(dane)	((dane)->flags & TLS_DANE_FLAG_NORRS)

#define TLS_DANE_CACHE_TTL_MIN 1	/* A lot can happen in ~2 seconds */
#define TLS_DANE_CACHE_TTL_MAX 100	/* Comparable to max_idle */

 /*
  * Certificate and public key digests (typically from TLSA RRs), grouped by
  * algorithm.
  */
typedef struct TLS_TLSA {
    char   *mdalg;			/* Algorithm for this digest list */
    ARGV   *certs;			/* Complete certificate digests */
    ARGV   *pkeys;			/* SubjectPublicKeyInfo digests */
    struct TLS_TLSA *next;		/* Chain to next algorithm */
} TLS_TLSA;

 /*
  * Linked list of full X509 trust-anchor certs.
  */
typedef struct TLS_CERTS {
    X509   *cert;
    struct TLS_CERTS *next;
} TLS_CERTS;

 /*
  * Linked list of full EVP_PKEY trust-anchor public keys.
  */
typedef struct TLS_PKEYS {
    EVP_PKEY *pkey;
    struct TLS_PKEYS *next;
} TLS_PKEYS;

typedef struct TLS_DANE {
    TLS_TLSA *ta;			/* Trust-anchor cert/pubkey digests */
    TLS_TLSA *ee;			/* End-entity cert/pubkey digests */
    TLS_CERTS *certs;			/* Full trust-anchor certificates */
    TLS_PKEYS *pkeys;			/* Full trust-anchor public keys */
    char   *base_domain;		/* Base domain of TLSA RRset */
    int     flags;			/* Conflate cert and pkey digests */
    time_t  expires;			/* Expiration time of this record */
    int     refs;			/* Reference count */
} TLS_DANE;

#define TLS_DANE_HASTA(d)	((d) ? (d)->ta : 0)
#define TLS_DANE_HASEE(d)	((d) ? (d)->ee : 0)

 /*
  * tls_dane.c
  */
extern int tls_dane_avail(void);
extern void tls_dane_flush(void);
extern void tls_dane_verbose(int);
extern TLS_DANE *tls_dane_alloc(void);
extern void tls_dane_add_ee_digests(TLS_DANE *, const char *, const char *,
				            const char *);
extern void tls_dane_free(TLS_DANE *);
extern TLS_DANE *tls_dane_resolve(unsigned, const char *, DNS_RR *, int);
extern int tls_dane_load_trustfile(TLS_DANE *, const char *);

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
    char   *peer_cert_fprint;		/* ASCII certificate fingerprint */
    char   *peer_pkey_fprint;		/* ASCII public key fingerprint */
    int     peer_status;		/* Certificate and match status */
    const char *protocol;
    const char *cipher_name;
    int     cipher_usebits;
    int     cipher_algbits;
    /* Private. */
    SSL    *con;
    char   *cache_type;			/* tlsmgr(8) cache type if enabled */
    int     ticketed;			/* Session ticket issued */
    char   *serverid;			/* unique server identifier */
    char   *namaddr;			/* nam[addr] for logging */
    int     log_mask;			/* What to log */
    int     session_reused;		/* this session was reused */
    int     am_server;			/* Are we an SSL server or client? */
    const char *mdalg;			/* default message digest algorithm */
    /* Built-in vs external SSL_accept/read/write/shutdown support. */
    VSTREAM *stream;			/* Blocking-mode SMTP session */
    /* RFC 6698 DANE trust input and verification state */
    const TLS_DANE *dane;		/* DANE TLSA digests */
    int     errordepth;			/* Chain depth of error cert */
    int     tadepth;			/* Chain depth of trust anchor */
    int     errorcode;			/* First error at error depth */
    X509   *errorcert;			/* Error certificate closest to leaf */
    x509_stack_t *untrusted;		/* Certificate chain fodder */
    x509_stack_t *trusted;		/* Internal root CA list */
} TLS_SESS_STATE;

 /*
  * Peer status bits. TLS_CERT_FLAG_MATCHED implies TLS_CERT_FLAG_TRUSTED
  * only in the case of a hostname match.
  */
#define TLS_CERT_FLAG_PRESENT		(1<<0)
#define TLS_CERT_FLAG_ALTNAME		(1<<1)
#define TLS_CERT_FLAG_TRUSTED		(1<<2)
#define TLS_CERT_FLAG_MATCHED		(1<<3)

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
  * Log mask details are internal to the library.
  */
extern int tls_log_mask(const char *, const char *);

 /*
  * What to log.
  */
#define TLS_LOG_NONE			(1<<0)
#define TLS_LOG_SUMMARY			(1<<1)
#define TLS_LOG_UNTRUSTED		(1<<2)
#define TLS_LOG_PEERCERT		(1<<3)
#define TLS_LOG_CERTMATCH		(1<<4)
#define TLS_LOG_VERBOSE			(1<<5)
#define TLS_LOG_CACHE			(1<<6)
#define TLS_LOG_DEBUG			(1<<7)
#define TLS_LOG_TLSPKTS			(1<<8)
#define TLS_LOG_ALLPKTS			(1<<9)
#define TLS_LOG_SESSTKT			(1<<10)

 /*
  * Client and Server application contexts
  */
struct TLS_APPL_STATE {
    SSL_CTX *ssl_ctx;
    int     log_mask;
    char   *cache_type;
    char   *cipher_exclusions;		/* Last cipher selection state */
    char   *cipher_list;		/* Last cipher selection state */
    int     cipher_grade;		/* Last cipher selection state */
    VSTRING *why;
};

 /*
  * tls_misc.c Application-context update and disposal.
  */
extern void tls_update_app_logmask(TLS_APPL_STATE *, int);
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
#ifdef SSL_TXT_TLSV1_1
#define TLS_PROTOCOL_TLSv1_1	(1<<3)	/* TLSv1_1 */
#else
#define TLS_PROTOCOL_TLSv1_1	0	/* Unknown */
#undef  SSL_OP_NO_TLSv1_1
#define SSL_OP_NO_TLSv1_1	0L	/* Noop */
#endif
#ifdef SSL_TXT_TLSV1_2
#define TLS_PROTOCOL_TLSv1_2	(1<<4)	/* TLSv1_2 */
#else
#define TLS_PROTOCOL_TLSv1_2	0	/* Unknown */
#undef  SSL_OP_NO_TLSv1_2
#define SSL_OP_NO_TLSv1_2	0L	/* Noop */
#endif
#define TLS_KNOWN_PROTOCOLS \
	( TLS_PROTOCOL_SSLv2 | TLS_PROTOCOL_SSLv3 | TLS_PROTOCOL_TLSv1 \
	   | TLS_PROTOCOL_TLSv1_1 | TLS_PROTOCOL_TLSv1_2 )
#define TLS_SSL_OP_PROTOMASK(m) \
	    ((((m) & TLS_PROTOCOL_SSLv2) ? SSL_OP_NO_SSLv2 : 0L) \
	     | (((m) & TLS_PROTOCOL_SSLv3) ? SSL_OP_NO_SSLv3 : 0L) \
	     | (((m) & TLS_PROTOCOL_TLSv1) ? SSL_OP_NO_TLSv1 : 0L) \
	     | (((m) & TLS_PROTOCOL_TLSv1_1) ? SSL_OP_NO_TLSv1_1 : 0L) \
	     | (((m) & TLS_PROTOCOL_TLSv1_2) ? SSL_OP_NO_TLSv1_2 : 0L))

/*
 * SSL options that are managed via dedicated Postfix features, rather than
 * just exposed via hex codes or named elements of tls_ssl_options.
 */
#define TLS_SSL_OP_MANAGED_BITS \
	(SSL_OP_CIPHER_SERVER_PREFERENCE | TLS_SSL_OP_PROTOMASK(~0))

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
    const char *log_param;
    const char *log_level;
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
    const char *mdalg;			/* default message digest algorithm */
} TLS_CLIENT_INIT_PROPS;

typedef struct {
    TLS_APPL_STATE *ctx;
    VSTREAM *stream;
    int     timeout;
    int     tls_level;			/* Security level */
    const char *nexthop;		/* destination domain */
    const char *host;			/* MX hostname */
    const char *namaddr;		/* nam[addr] for logging */
    const char *serverid;		/* Session cache key */
    const char *helo;			/* Server name from EHLO response */
    const char *protocols;		/* Enabled protocols */
    const char *cipher_grade;		/* Minimum cipher grade */
    const char *cipher_exclusions;	/* Ciphers to exclude */
    const ARGV *matchargv;		/* Cert match patterns */
    const char *mdalg;			/* default message digest algorithm */
    const TLS_DANE *dane;		/* RFC 6698 verification */
} TLS_CLIENT_START_PROPS;

extern TLS_APPL_STATE *tls_client_init(const TLS_CLIENT_INIT_PROPS *);
extern TLS_SESS_STATE *tls_client_start(const TLS_CLIENT_START_PROPS *);

#define tls_client_stop(ctx, stream, timeout, failure, TLScontext) \
	tls_session_stop(ctx, (stream), (timeout), (failure), (TLScontext))

#define TLS_CLIENT_INIT(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12, a13) \
    tls_client_init((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), (props)))

#define TLS_CLIENT_START(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12, a13, a14, a15) \
    tls_client_start((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14), ((props)->a15), (props)))

 /*
  * tls_server.c
  */
typedef struct {
    const char *log_param;
    const char *log_level;
    int     verifydepth;
    const char *cache_type;
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
    const char *mdalg;			/* default message digest algorithm */
} TLS_SERVER_INIT_PROPS;

typedef struct {
    TLS_APPL_STATE *ctx;		/* TLS application context */
    VSTREAM *stream;			/* Client stream */
    int     fd;				/* Event-driven file descriptor */
    int     timeout;			/* TLS handshake timeout */
    int     requirecert;		/* Insist on client cert? */
    const char *serverid;		/* Server instance (salt cache key) */
    const char *namaddr;		/* Client nam[addr] for logging */
    const char *cipher_grade;
    const char *cipher_exclusions;
    const char *mdalg;			/* default message digest algorithm */
} TLS_SERVER_START_PROPS;

extern TLS_APPL_STATE *tls_server_init(const TLS_SERVER_INIT_PROPS *);
extern TLS_SESS_STATE *tls_server_start(const TLS_SERVER_START_PROPS *props);
extern TLS_SESS_STATE *tls_server_post_accept(TLS_SESS_STATE *);

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
extern int tls_verify_certificate_callback(int, X509_STORE_CTX *);
extern void tls_log_verify_error(TLS_SESS_STATE *);

 /*
  * tls_dane.c
  */
extern int tls_dane_match(TLS_SESS_STATE *, int, X509 *, int);
extern void tls_dane_set_callback(SSL_CTX *, TLS_SESS_STATE *);

 /*
  * tls_fprint.c
  */
extern char *tls_digest_encode(const unsigned char *, int);
extern char *tls_data_fprint(const char *, int, const char *);
extern char *tls_cert_fprint(X509 *, const char *);
extern char *tls_pkey_fprint(X509 *, const char *);
extern char *tls_serverid_digest(const TLS_CLIENT_START_PROPS *, long,
				         const char *);

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

extern TLS_APPL_STATE *tls_alloc_app_context(SSL_CTX *, int);
extern TLS_SESS_STATE *tls_alloc_sess_context(int, const char *);
extern void tls_free_context(TLS_SESS_STATE *);
extern void tls_check_version(void);
extern long tls_bug_bits(void);
extern void tls_print_errors(void);
extern void tls_info_callback(const SSL *, int, int);
extern long tls_bio_dump_cb(BIO *, int, const char *, int, long, long);
extern int tls_validate_digest(const char *);

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
