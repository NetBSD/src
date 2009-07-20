/* tls.c - Handle tls/ssl using SSLeay, OpenSSL or GNUTLS. */
/* $OpenLDAP: pkg/ldap/libraries/libldap/tls.c,v 1.133.2.11 2008/07/09 23:56:44 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS: GNUTLS support written by Howard Chu and
 * Matt Backes; sponsored by The Written Word (thewrittenword.com)
 * and Stanford University (stanford.edu).
 */

#include "portable.h"
#include "ldap_config.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/time.h>
#include <ac/unistd.h>
#include <ac/param.h>
#include <ac/dirent.h>

#include "ldap-int.h"

#ifdef HAVE_TLS

#ifdef LDAP_R_COMPILE
#include <ldap_pvt_thread.h>
#endif

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gcrypt.h>

#define DH_BITS	(1024)

#else
#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/safestack.h>
#elif defined( HAVE_SSL_H )
#include <ssl.h>
#endif
#endif

#define HAS_TLS( sb )	ber_sockbuf_ctrl( sb, LBER_SB_OPT_HAS_IO, \
				(void *)&sb_tls_sbio )

#endif /* HAVE_TLS */

/* RFC2459 minimum required set of supported attribute types
 * in a certificate DN
 */
typedef struct oid_name {
	struct berval oid;
	struct berval name;
} oid_name;

#define	CN_OID	oids[0].oid.bv_val

static oid_name oids[] = {
	{ BER_BVC("2.5.4.3"), BER_BVC("cn") },
	{ BER_BVC("2.5.4.4"), BER_BVC("sn") },
	{ BER_BVC("2.5.4.6"), BER_BVC("c") },
	{ BER_BVC("2.5.4.7"), BER_BVC("l") },
	{ BER_BVC("2.5.4.8"), BER_BVC("st") },
	{ BER_BVC("2.5.4.10"), BER_BVC("o") },
	{ BER_BVC("2.5.4.11"), BER_BVC("ou") },
	{ BER_BVC("2.5.4.12"), BER_BVC("title") },
	{ BER_BVC("2.5.4.41"), BER_BVC("name") },
	{ BER_BVC("2.5.4.42"), BER_BVC("givenName") },
	{ BER_BVC("2.5.4.43"), BER_BVC("initials") },
	{ BER_BVC("2.5.4.44"), BER_BVC("generationQualifier") },
	{ BER_BVC("2.5.4.46"), BER_BVC("dnQualifier") },
	{ BER_BVC("1.2.840.113549.1.9.1"), BER_BVC("email") },
	{ BER_BVC("0.9.2342.19200300.100.1.25"), BER_BVC("dc") },
	{ BER_BVNULL, BER_BVNULL }
};

#ifdef HAVE_TLS
#ifdef HAVE_GNUTLS

typedef struct tls_cipher_suite {
	const char *name;
	gnutls_kx_algorithm_t kx;
	gnutls_cipher_algorithm_t cipher;
	gnutls_mac_algorithm_t mac;
	gnutls_protocol_t version;
} tls_cipher_suite;

static tls_cipher_suite *ciphers;
static int n_ciphers;

/* sorta replacing SSL_CTX */
typedef struct tls_ctx {
	struct ldapoptions *lo;
	gnutls_certificate_credentials_t cred;
	gnutls_dh_params_t dh_params;
	unsigned long verify_depth;
	int refcount;
	int *kx_list;
	int *cipher_list;
	int *mac_list;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_t ref_mutex;
#endif
} tls_ctx;

/* sorta replacing SSL */
typedef struct tls_session {
	tls_ctx *ctx;
	gnutls_session_t session;
	struct berval peer_der_dn;
} tls_session;

#ifdef LDAP_R_COMPILE

static int
ldap_pvt_gcry_mutex_init( void **priv )
{
	int err = 0;
	ldap_pvt_thread_mutex_t *lock = LDAP_MALLOC( sizeof( ldap_pvt_thread_mutex_t ));

	if ( !lock )
		err = ENOMEM;
	if ( !err ) {
		err = ldap_pvt_thread_mutex_init( lock );
		if ( err )
			LDAP_FREE( lock );
		else
			*priv = lock;
	}
	return err;
}
static int
ldap_pvt_gcry_mutex_destroy( void **lock )
{
	int err = ldap_pvt_thread_mutex_destroy( *lock );
	LDAP_FREE( *lock );
	return err;
}
static int
ldap_pvt_gcry_mutex_lock( void **lock )
{
	return ldap_pvt_thread_mutex_lock( *lock );
}
static int
ldap_pvt_gcry_mutex_unlock( void **lock )
{
	return ldap_pvt_thread_mutex_unlock( *lock );
}

static struct gcry_thread_cbs ldap_generic_thread_cbs = {
	GCRY_THREAD_OPTION_USER,
	NULL,
	ldap_pvt_gcry_mutex_init,
	ldap_pvt_gcry_mutex_destroy,
	ldap_pvt_gcry_mutex_lock,
	ldap_pvt_gcry_mutex_unlock,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static void
tls_init_threads( void )
{
	gcry_control (GCRYCTL_SET_THREAD_CBS, &ldap_generic_thread_cbs);
}
#endif /* LDAP_R_COMPILE */

void
ldap_pvt_tls_ctx_free ( void *c )
{
	int refcount;
	tls_ctx *ctx = c;

	if ( !ctx ) return;

#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &ctx->ref_mutex );
#endif
	refcount = --ctx->refcount;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &ctx->ref_mutex );
#endif
	if ( refcount )
		return;
	LDAP_FREE( ctx->kx_list );
	gnutls_certificate_free_credentials( ctx->cred );
	ber_memfree ( ctx );
}

static void *
tls_ctx_new ( struct ldapoptions *lo )
{
	tls_ctx *ctx;

	ctx = ber_memcalloc ( 1, sizeof (*ctx) );
	if ( ctx ) {
		ctx->lo = lo;
		if ( gnutls_certificate_allocate_credentials( &ctx->cred )) {
			ber_memfree( ctx );
			return NULL;
		}
		ctx->refcount = 1;
#ifdef LDAP_R_COMPILE
		ldap_pvt_thread_mutex_init( &ctx->ref_mutex );
#endif
	}
	return ctx;
}

static void
tls_ctx_ref( tls_ctx *ctx )
{
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &ctx->ref_mutex );
#endif
	ctx->refcount++;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &ctx->ref_mutex );
#endif
}

tls_session *
tls_session_new ( tls_ctx * ctx, int is_server )
{
	tls_session *session;

	session = ber_memcalloc ( 1, sizeof (*session) );
	if ( !session )
		return NULL;

	session->ctx = ctx;
	gnutls_init( &session->session, is_server ? GNUTLS_SERVER : GNUTLS_CLIENT );
	gnutls_set_default_priority( session->session );
	if ( ctx->kx_list ) {
		gnutls_kx_set_priority( session->session, ctx->kx_list );
		gnutls_cipher_set_priority( session->session, ctx->cipher_list );
		gnutls_mac_set_priority( session->session, ctx->mac_list );
	}
	if ( ctx->cred )
		gnutls_credentials_set( session->session, GNUTLS_CRD_CERTIFICATE, ctx->cred );
	
	if ( is_server ) {
		int flag = 0;
		if ( ctx->lo->ldo_tls_require_cert ) {
			flag = GNUTLS_CERT_REQUEST;
			if ( ctx->lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_DEMAND ||
				ctx->lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_HARD )
				flag = GNUTLS_CERT_REQUIRE;
			gnutls_certificate_server_set_request( session->session, flag );
		}
	}
	return session;
} 

void
tls_session_free ( tls_session * session )
{
	ber_memfree ( session );
}

#define	tls_session_connect( ssl )	gnutls_handshake( ssl->session )
#define	tls_session_accept( ssl )	gnutls_handshake( ssl->session )

/* suites is a string of colon-separated cipher suite names. */
static int
tls_parse_ciphers( tls_ctx *ctx, char *suites )
{
	char *ptr, *end;
	int i, j, len, num;
	int *list, nkx = 0, ncipher = 0, nmac = 0;
	int *kx, *cipher, *mac;

	num = 0;
	ptr = suites;
	do {
		end = strchr(ptr, ':');
		if ( end )
			len = end - ptr;
		else
			len = strlen(ptr);
		for (i=0; i<n_ciphers; i++) {
			if ( !strncasecmp( ciphers[i].name, ptr, len )) {
				num++;
				break;
			}
		}
		if ( i == n_ciphers ) {
			/* unrecognized cipher suite */
			return -1;
		}
		ptr += len + 1;
	} while (end);

	/* Space for all 3 lists */
	list = LDAP_MALLOC( (num+1) * sizeof(int) * 3 );
	if ( !list )
		return -1;
	kx = list;
	cipher = kx+num+1;
	mac = cipher+num+1;

	ptr = suites;
	do {
		end = strchr(ptr, ':');
		if ( end )
			len = end - ptr;
		else
			len = strlen(ptr);
		for (i=0; i<n_ciphers; i++) {
			/* For each cipher suite, insert its algorithms into
			 * their respective priority lists. Make sure they
			 * only appear once in each list.
			 */
			if ( !strncasecmp( ciphers[i].name, ptr, len )) {
				for (j=0; j<nkx; j++)
					if ( kx[j] == ciphers[i].kx )
						break;
				if ( j == nkx )
					kx[nkx++] = ciphers[i].kx;
				for (j=0; j<ncipher; j++)
					if ( cipher[j] == ciphers[i].cipher )
						break;
				if ( j == ncipher ) 
					cipher[ncipher++] = ciphers[i].cipher;
				for (j=0; j<nmac; j++)
					if ( mac[j] == ciphers[i].mac )
						break;
				if ( j == nmac )
					mac[nmac++] = ciphers[i].mac;
				break;
			}
		}
		ptr += len + 1;
	} while (end);
	kx[nkx] = 0;
	cipher[ncipher] = 0;
	mac[nmac] = 0;
	ctx->kx_list = kx;
	ctx->cipher_list = cipher;
	ctx->mac_list = mac;
	return 0;
}

#else /* OpenSSL */

typedef SSL_CTX tls_ctx;
typedef SSL tls_session;

static int  tls_opt_trace = 1;
static char *tls_opt_randfile = NULL;

static void tls_report_error( void );

static void tls_info_cb( const SSL *ssl, int where, int ret );
static int tls_verify_cb( int ok, X509_STORE_CTX *ctx );
static int tls_verify_ok( int ok, X509_STORE_CTX *ctx );
static RSA * tls_tmp_rsa_cb( SSL *ssl, int is_export, int key_length );

static DH * tls_tmp_dh_cb( SSL *ssl, int is_export, int key_length );

typedef struct dhplist {
	struct dhplist *next;
	int keylength;
	DH *param;
} dhplist;

static dhplist *dhparams;

static int tls_seed_PRNG( const char *randfile );

#ifdef LDAP_R_COMPILE
/*
 * provide mutexes for the SSLeay library.
 */
static ldap_pvt_thread_mutex_t	tls_mutexes[CRYPTO_NUM_LOCKS];

static void tls_locking_cb( int mode, int type, const char *file, int line )
{
	if ( mode & CRYPTO_LOCK ) {
		ldap_pvt_thread_mutex_lock( &tls_mutexes[type] );
	} else {
		ldap_pvt_thread_mutex_unlock( &tls_mutexes[type] );
	}
}

static unsigned long tls_thread_self( void )
{
	/* FIXME: CRYPTO_set_id_callback only works when ldap_pvt_thread_t
	 * is an integral type that fits in an unsigned long
	 */

	/* force an error if the ldap_pvt_thread_t type is too large */
	enum { ok = sizeof( ldap_pvt_thread_t ) <= sizeof( unsigned long ) };
	typedef struct { int dummy: ok ? 1 : -1; } Check[ok ? 1 : -1];

	return (unsigned long) ldap_pvt_thread_self();
}

static void tls_init_threads( void )
{
	int i;

	for( i=0; i< CRYPTO_NUM_LOCKS ; i++ ) {
		ldap_pvt_thread_mutex_init( &tls_mutexes[i] );
	}
	CRYPTO_set_locking_callback( tls_locking_cb );
	CRYPTO_set_id_callback( tls_thread_self );
}
#endif /* LDAP_R_COMPILE */

void
ldap_pvt_tls_ctx_free ( void *c )
{

	SSL_CTX_free( c );
}

static void *
tls_ctx_new( struct ldapoptions *lo )
{
	return SSL_CTX_new( SSLv23_method() );
}

static void
tls_ctx_ref( void *c )
{
	SSL_CTX *ctx = c;
	CRYPTO_add( &ctx->references, 1, CRYPTO_LOCK_SSL_CTX );
}

static tls_session *
tls_session_new( tls_ctx *ctx, int is_server )
{
	return SSL_new( ctx );
}

#define	tls_session_connect( ssl )	SSL_connect( ssl )
#define	tls_session_accept( ssl )	SSL_accept( ssl )

static STACK_OF(X509_NAME) *
get_ca_list( char * bundle, char * dir )
{
	STACK_OF(X509_NAME) *ca_list = NULL;

	if ( bundle ) {
		ca_list = SSL_load_client_CA_file( bundle );
	}
#if defined(HAVE_DIRENT_H) || defined(dirent)
	if ( dir ) {
		int freeit = 0;

		if ( !ca_list ) {
			ca_list = sk_X509_NAME_new_null();
			freeit = 1;
		}
		if ( !SSL_add_dir_cert_subjects_to_stack( ca_list, dir ) &&
			freeit ) {
			sk_X509_NAME_free( ca_list );
			ca_list = NULL;
		}
	}
#endif
	return ca_list;
}

#endif /* HAVE_GNUTLS */

#ifdef LDAP_R_COMPILE
/*
 * an extra mutex for the default ctx.
 */
static ldap_pvt_thread_mutex_t tls_def_ctx_mutex;
#endif

void
ldap_int_tls_destroy( struct ldapoptions *lo )
{
	if ( lo->ldo_tls_ctx ) {
		ldap_pvt_tls_ctx_free( lo->ldo_tls_ctx );
		lo->ldo_tls_ctx = NULL;
	}

	if ( lo->ldo_tls_certfile ) {
		LDAP_FREE( lo->ldo_tls_certfile );
		lo->ldo_tls_certfile = NULL;
	}
	if ( lo->ldo_tls_keyfile ) {
		LDAP_FREE( lo->ldo_tls_keyfile );
		lo->ldo_tls_keyfile = NULL;
	}
	if ( lo->ldo_tls_dhfile ) {
		LDAP_FREE( lo->ldo_tls_dhfile );
		lo->ldo_tls_dhfile = NULL;
	}
	if ( lo->ldo_tls_cacertfile ) {
		LDAP_FREE( lo->ldo_tls_cacertfile );
		lo->ldo_tls_cacertfile = NULL;
	}
	if ( lo->ldo_tls_cacertdir ) {
		LDAP_FREE( lo->ldo_tls_cacertdir );
		lo->ldo_tls_cacertdir = NULL;
	}
	if ( lo->ldo_tls_ciphersuite ) {
		LDAP_FREE( lo->ldo_tls_ciphersuite );
		lo->ldo_tls_ciphersuite = NULL;
	}
#ifdef HAVE_GNUTLS
	if ( lo->ldo_tls_crlfile ) {
		LDAP_FREE( lo->ldo_tls_crlfile );
		lo->ldo_tls_crlfile = NULL;
	}
#endif
}

/*
 * Tear down the TLS subsystem. Should only be called once.
 */
void
ldap_pvt_tls_destroy( void )
{
	struct ldapoptions *lo = LDAP_INT_GLOBAL_OPT();   

	ldap_int_tls_destroy( lo );

#ifdef HAVE_GNUTLS
	LDAP_FREE( ciphers );
	ciphers = NULL;

	gnutls_global_deinit();
#else
	EVP_cleanup();
	ERR_remove_state(0);
	ERR_free_strings();

	if ( tls_opt_randfile ) {
		LDAP_FREE( tls_opt_randfile );
		tls_opt_randfile = NULL;
	}
#endif
}

/*
 * Initialize TLS subsystem. Should be called only once.
 */
int
ldap_pvt_tls_init( void )
{
	static int tls_initialized = 0;

	if ( tls_initialized++ ) return 0;

#ifdef LDAP_R_COMPILE
	tls_init_threads();
	ldap_pvt_thread_mutex_init( &tls_def_ctx_mutex );
#endif

#ifdef HAVE_GNUTLS
	gnutls_global_init ();

	/* GNUtls cipher suite handling: The library ought to parse suite
	 * names for us, but it doesn't. It will return a list of suite names
	 * that it supports, so we can do parsing ourselves. It ought to tell
	 * us how long the list is, but it doesn't do that either, so we just
	 * have to count it manually...
	 */
	{
		int i = 0;
		tls_cipher_suite *ptr, tmp;
		char cs_id[2];

		while ( gnutls_cipher_suite_info( i, cs_id, &tmp.kx, &tmp.cipher,
			&tmp.mac, &tmp.version ))
			i++;
		n_ciphers = i;

		/* Store a copy */
		ciphers = LDAP_MALLOC(n_ciphers * sizeof(tls_cipher_suite));
		if ( !ciphers )
			return -1;
		for ( i=0; i<n_ciphers; i++ ) {
			ciphers[i].name = gnutls_cipher_suite_info( i, cs_id,
				&ciphers[i].kx, &ciphers[i].cipher, &ciphers[i].mac,
				&ciphers[i].version );
		}
	}

#else /* !HAVE_GNUTLS */

#ifdef HAVE_EBCDIC
	{
		char *file = LDAP_STRDUP( tls_opt_randfile );
		if ( file ) __atoe( file );
		(void) tls_seed_PRNG( file );
		LDAP_FREE( file );
	}
#else
	(void) tls_seed_PRNG( tls_opt_randfile );
#endif

	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();

	/* FIXME: mod_ssl does this */
	X509V3_add_standard_extensions();

#endif /* HAVE_GNUTLS */
	return 0;
}

/*
 * initialize a new TLS context
 */
static int
ldap_int_tls_init_ctx( struct ldapoptions *lo, int is_server )
{
	int i, rc = 0;
	char *ciphersuite = lo->ldo_tls_ciphersuite;
	char *cacertfile = lo->ldo_tls_cacertfile;
	char *cacertdir = lo->ldo_tls_cacertdir;
	char *certfile = lo->ldo_tls_certfile;
	char *keyfile = lo->ldo_tls_keyfile;
#ifdef HAVE_GNUTLS
	char *crlfile = lo->ldo_tls_crlfile;
#else
	char *dhfile = lo->ldo_tls_dhfile;
#endif

	if ( lo->ldo_tls_ctx )
		return 0;

	ldap_pvt_tls_init();

	if ( is_server && !certfile && !keyfile && !cacertfile && !cacertdir ) {
		/* minimum configuration not provided */
		return LDAP_NOT_SUPPORTED;
	}

#ifdef HAVE_EBCDIC
	/* This ASCII/EBCDIC handling is a real pain! */
	if ( ciphersuite ) {
		ciphersuite = LDAP_STRDUP( ciphersuite );
		__atoe( ciphersuite );
	}
	if ( cacertfile ) {
		cacertfile = LDAP_STRDUP( cacertfile );
		__atoe( cacertfile );
	}
	if ( certfile ) {
		certfile = LDAP_STRDUP( certfile );
		__atoe( certfile );
	}
	if ( keyfile ) {
		keyfile = LDAP_STRDUP( keyfile );
		__atoe( keyfile );
	}
#ifdef HAVE_GNUTLS
	if ( crlfile ) {
		crlfile = LDAP_STRDUP( crlfile );
		__atoe( crlfile );
	}
#else
	if ( cacertdir ) {
		cacertdir = LDAP_STRDUP( cacertdir );
		__atoe( cacertdir );
	}
	if ( dhfile ) {
		dhfile = LDAP_STRDUP( dhfile );
		__atoe( dhfile );
	}
#endif
#endif
	lo->ldo_tls_ctx = tls_ctx_new( lo );
	if ( lo->ldo_tls_ctx == NULL ) {
#ifdef HAVE_GNUTLS
		Debug( LDAP_DEBUG_ANY,
		   "TLS: could not allocate default ctx.\n",
			0,0,0);
#else
		Debug( LDAP_DEBUG_ANY,
		   "TLS: could not allocate default ctx (%lu).\n",
			ERR_peek_error(),0,0);
#endif
		rc = -1;
		goto error_exit;
	}

#ifdef HAVE_GNUTLS
 	if ( lo->ldo_tls_ciphersuite &&
		tls_parse_ciphers( lo->ldo_tls_ctx,
			ciphersuite )) {
 		Debug( LDAP_DEBUG_ANY,
 			   "TLS: could not set cipher list %s.\n",
 			   lo->ldo_tls_ciphersuite, 0, 0 );
 		rc = -1;
 		goto error_exit;
 	}

	if (lo->ldo_tls_cacertdir != NULL) {
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: warning: cacertdir not implemented for gnutls\n",
		       NULL, NULL, NULL );
	}

	if (lo->ldo_tls_cacertfile != NULL) {
		rc = gnutls_certificate_set_x509_trust_file( 
			((tls_ctx*) lo->ldo_tls_ctx)->cred,
			cacertfile,
			GNUTLS_X509_FMT_PEM );
		if ( rc < 0 ) goto error_exit;
	}

	if ( lo->ldo_tls_certfile && lo->ldo_tls_keyfile ) {
		rc = gnutls_certificate_set_x509_key_file( 
			((tls_ctx*) lo->ldo_tls_ctx)->cred,
			certfile,
			keyfile,
			GNUTLS_X509_FMT_PEM );
		if ( rc ) goto error_exit;
	} else if ( lo->ldo_tls_certfile || lo->ldo_tls_keyfile ) {
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: only one of certfile and keyfile specified\n",
		       NULL, NULL, NULL );
		rc = 1;
		goto error_exit;
	}

	if ( lo->ldo_tls_dhfile ) {
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: warning: ignoring dhfile\n", 
		       NULL, NULL, NULL );
	}

	if ( lo->ldo_tls_crlfile ) {
		rc = gnutls_certificate_set_x509_crl_file( 
			((tls_ctx*) lo->ldo_tls_ctx)->cred,
			crlfile,
			GNUTLS_X509_FMT_PEM );
		if ( rc < 0 ) goto error_exit;
		rc = 0;
	}
	if ( is_server ) {
		gnutls_dh_params_init (&((tls_ctx*) 
					lo->ldo_tls_ctx)->dh_params);
		gnutls_dh_params_generate2 (((tls_ctx*) 
						 lo->ldo_tls_ctx)->dh_params, 
						DH_BITS);
	}

#else /* !HAVE_GNUTLS */

	if ( is_server ) {
		SSL_CTX_set_session_id_context( lo->ldo_tls_ctx,
			(const unsigned char *) "OpenLDAP", sizeof("OpenLDAP")-1 );
	}

	if ( lo->ldo_tls_ciphersuite &&
		!SSL_CTX_set_cipher_list( lo->ldo_tls_ctx, ciphersuite ) )
	{
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not set cipher list %s.\n",
			   lo->ldo_tls_ciphersuite, 0, 0 );
		tls_report_error();
		rc = -1;
		goto error_exit;
	}

	if (lo->ldo_tls_cacertfile != NULL || lo->ldo_tls_cacertdir != NULL) {
		if ( !SSL_CTX_load_verify_locations( lo->ldo_tls_ctx,
				cacertfile, cacertdir ) ||
			!SSL_CTX_set_default_verify_paths( lo->ldo_tls_ctx ) )
		{
			Debug( LDAP_DEBUG_ANY, "TLS: "
				"could not load verify locations (file:`%s',dir:`%s').\n",
				lo->ldo_tls_cacertfile ? lo->ldo_tls_cacertfile : "",
				lo->ldo_tls_cacertdir ? lo->ldo_tls_cacertdir : "",
				0 );
			tls_report_error();
			rc = -1;
			goto error_exit;
		}

		if ( is_server ) {
			STACK_OF(X509_NAME) *calist;
			/* List of CA names to send to a client */
			calist = get_ca_list( cacertfile, cacertdir );
			if ( !calist ) {
				Debug( LDAP_DEBUG_ANY, "TLS: "
					"could not load client CA list (file:`%s',dir:`%s').\n",
					lo->ldo_tls_cacertfile ? lo->ldo_tls_cacertfile : "",
					lo->ldo_tls_cacertdir ? lo->ldo_tls_cacertdir : "",
					0 );
				tls_report_error();
				rc = -1;
				goto error_exit;
			}

			SSL_CTX_set_client_CA_list( lo->ldo_tls_ctx, calist );
		}
	}

	if ( lo->ldo_tls_certfile &&
		!SSL_CTX_use_certificate_file( lo->ldo_tls_ctx,
			certfile, SSL_FILETYPE_PEM ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"TLS: could not use certificate `%s'.\n",
			lo->ldo_tls_certfile,0,0);
		tls_report_error();
		rc = -1;
		goto error_exit;
	}

	/* Key validity is checked automatically if cert has already been set */
	if ( lo->ldo_tls_keyfile &&
		!SSL_CTX_use_PrivateKey_file( lo->ldo_tls_ctx,
			keyfile, SSL_FILETYPE_PEM ) )
	{
		Debug( LDAP_DEBUG_ANY,
			"TLS: could not use key file `%s'.\n",
			lo->ldo_tls_keyfile,0,0);
		tls_report_error();
		rc = -1;
		goto error_exit;
	}

	if ( lo->ldo_tls_dhfile ) {
		DH *dh = NULL;
		BIO *bio;
		dhplist *p;

		if (( bio=BIO_new_file( dhfile,"r" )) == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"TLS: could not use DH parameters file `%s'.\n",
				lo->ldo_tls_dhfile,0,0);
			tls_report_error();
			rc = -1;
			goto error_exit;
		}
		while (( dh=PEM_read_bio_DHparams( bio, NULL, NULL, NULL ))) {
			p = LDAP_MALLOC( sizeof(dhplist) );
			if ( p != NULL ) {
				p->keylength = DH_size( dh ) * 8;
				p->param = dh;
				p->next = dhparams;
				dhparams = p;
			}
		}
		BIO_free( bio );
	}

	if ( tls_opt_trace ) {
		SSL_CTX_set_info_callback( (SSL_CTX *)lo->ldo_tls_ctx, tls_info_cb );
	}

	i = SSL_VERIFY_NONE;
	if ( lo->ldo_tls_require_cert ) {
		i = SSL_VERIFY_PEER;
		if ( lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_DEMAND ||
			 lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_HARD ) {
			i |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
		}
	}

	SSL_CTX_set_verify( lo->ldo_tls_ctx, i,
		lo->ldo_tls_require_cert == LDAP_OPT_X_TLS_ALLOW ?
		tls_verify_ok : tls_verify_cb );
	SSL_CTX_set_tmp_rsa_callback( lo->ldo_tls_ctx, tls_tmp_rsa_cb );
	if ( lo->ldo_tls_dhfile ) {
		SSL_CTX_set_tmp_dh_callback( lo->ldo_tls_ctx, tls_tmp_dh_cb );
	}
#ifdef HAVE_OPENSSL_CRL
	if ( lo->ldo_tls_crlcheck ) {
		X509_STORE *x509_s = SSL_CTX_get_cert_store( lo->ldo_tls_ctx );
		if ( lo->ldo_tls_crlcheck == LDAP_OPT_X_TLS_CRL_PEER ) {
			X509_STORE_set_flags( x509_s, X509_V_FLAG_CRL_CHECK );
		} else if ( lo->ldo_tls_crlcheck == LDAP_OPT_X_TLS_CRL_ALL ) {
			X509_STORE_set_flags( x509_s, 
					X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL  );
		}
	}
#endif

#endif /* HAVE_GNUTLS */

error_exit:
	if ( rc == -1 && lo->ldo_tls_ctx != NULL ) {
		ldap_pvt_tls_ctx_free( lo->ldo_tls_ctx );
		lo->ldo_tls_ctx = NULL;
	}
#ifdef HAVE_EBCDIC
	LDAP_FREE( ciphersuite );
	LDAP_FREE( cacertfile );
	LDAP_FREE( certfile );
	LDAP_FREE( keyfile );
#ifdef HAVE_GNUTLS
	LDAP_FREE( crlfile );
#else
	LDAP_FREE( cacertdir );
	LDAP_FREE( dhfile );
#endif
#endif
	return rc;
}

/*
 * initialize the default context
 */
int
ldap_pvt_tls_init_def_ctx( int is_server )
{
	struct ldapoptions *lo = LDAP_INT_GLOBAL_OPT();   
	int rc;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &tls_def_ctx_mutex );
#endif
	rc = ldap_int_tls_init_ctx( lo, is_server );
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &tls_def_ctx_mutex );
#endif
	return rc;
}

static tls_session *
alloc_handle( void *ctx_arg, int is_server )
{
	tls_ctx	*ctx;
	tls_session	*ssl;

	if ( ctx_arg ) {
		ctx = ctx_arg;
	} else {
		struct ldapoptions *lo = LDAP_INT_GLOBAL_OPT();   
		if ( ldap_pvt_tls_init_def_ctx( is_server ) < 0 ) return NULL;
		ctx = lo->ldo_tls_ctx;
	}

	ssl = tls_session_new( ctx, is_server );
	if ( ssl == NULL ) {
		Debug( LDAP_DEBUG_ANY,"TLS: can't create ssl handle.\n",0,0,0);
		return NULL;
	}
	return ssl;
}

static int
update_flags( Sockbuf *sb, tls_session * ssl, int rc )
{
	sb->sb_trans_needs_read  = 0;
	sb->sb_trans_needs_write = 0;

#ifdef HAVE_GNUTLS
	if ( rc != GNUTLS_E_INTERRUPTED && rc != GNUTLS_E_AGAIN )
		return 0;

	switch (gnutls_record_get_direction (ssl->session)) {
	case 0: 
		sb->sb_trans_needs_read = 1;
		return 1;
	case 1:
		sb->sb_trans_needs_write = 1;
		return 1;
	}
#else /* !HAVE_GNUTLS */
	rc = SSL_get_error(ssl, rc);
	if (rc == SSL_ERROR_WANT_READ) {
		sb->sb_trans_needs_read  = 1;
		return 1;

	} else if (rc == SSL_ERROR_WANT_WRITE) {
		sb->sb_trans_needs_write = 1;
		return 1;

	} else if (rc == SSL_ERROR_WANT_CONNECT) {
		return 1;
	}
#endif /* HAVE_GNUTLS */
	return 0;
}

/*
 * TLS support for LBER Sockbufs
 */

struct tls_data {
	tls_session			*ssl;
	Sockbuf_IO_Desc		*sbiod;
};

#ifdef HAVE_GNUTLS

static ssize_t
sb_gtls_recv( gnutls_transport_ptr_t ptr, void *buf, size_t len )
{
	struct tls_data		*p;

	if ( buf == NULL || len <= 0 ) return 0;

	p = (struct tls_data *)ptr;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	return LBER_SBIOD_READ_NEXT( p->sbiod, buf, len );
}

static ssize_t
sb_gtls_send( gnutls_transport_ptr_t ptr, const void *buf, size_t len )
{
	struct tls_data		*p;
	
	if ( buf == NULL || len <= 0 ) return 0;
	
	p = (struct tls_data *)ptr;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	return LBER_SBIOD_WRITE_NEXT( p->sbiod, (char *)buf, len );
}

static int
sb_tls_setup( Sockbuf_IO_Desc *sbiod, void *arg )
{
	struct tls_data		*p;
	tls_session	*session = arg;

	assert( sbiod != NULL );

	p = LBER_MALLOC( sizeof( *p ) );
	if ( p == NULL ) {
		return -1;
	}
	
	gnutls_transport_set_ptr( session->session, (gnutls_transport_ptr)p );
	gnutls_transport_set_pull_function( session->session, sb_gtls_recv );
	gnutls_transport_set_push_function( session->session, sb_gtls_send );
	p->ssl = arg;
	p->sbiod = sbiod;
	sbiod->sbiod_pvt = p;
	return 0;
}

static int
sb_tls_remove( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	gnutls_deinit ( p->ssl->session );
	LBER_FREE( p->ssl );
	LBER_FREE( sbiod->sbiod_pvt );
	sbiod->sbiod_pvt = NULL;
	return 0;
}

static int
sb_tls_close( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	gnutls_bye ( p->ssl->session, GNUTLS_SHUT_RDWR );
	return 0;
}

static int
sb_tls_ctrl( Sockbuf_IO_Desc *sbiod, int opt, void *arg )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	
	if ( opt == LBER_SB_OPT_GET_SSL ) {
		*((tls_session **)arg) = p->ssl;
		return 1;
		
	} else if ( opt == LBER_SB_OPT_DATA_READY ) {
		if( gnutls_record_check_pending( p->ssl->session ) > 0 ) {
			return 1;
		}
	}
	
	return LBER_SBIOD_CTRL_NEXT( sbiod, opt, arg );
}

static ber_slen_t
sb_tls_read( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = gnutls_record_recv ( p->ssl->session, buf, len );
	switch (ret) {
	case GNUTLS_E_INTERRUPTED:
	case GNUTLS_E_AGAIN:
		sbiod->sbiod_sb->sb_trans_needs_read = 1;
		sock_errset(EWOULDBLOCK);
		ret = 0;
		break;
	case GNUTLS_E_REHANDSHAKE:
		for ( ret = gnutls_handshake ( p->ssl->session );
		      ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN;
		      ret = gnutls_handshake ( p->ssl->session ) );
		sbiod->sbiod_sb->sb_trans_needs_read = 1;
		ret = 0;
		break;
	default:
		sbiod->sbiod_sb->sb_trans_needs_read = 0;
	}
	return ret;
}

static ber_slen_t
sb_tls_write( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = gnutls_record_send ( p->ssl->session, (char *)buf, len );

	if ( ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN ) {
		sbiod->sbiod_sb->sb_trans_needs_write = 1;
		sock_errset(EWOULDBLOCK);
		ret = 0;
	} else {
		sbiod->sbiod_sb->sb_trans_needs_write = 0;
	}
	return ret;
}

#else /* !HAVE_GNUTLS */

static int
sb_tls_bio_create( BIO *b ) {
	b->init = 1;
	b->num = 0;
	b->ptr = NULL;
	b->flags = 0;
	return 1;
}

static int
sb_tls_bio_destroy( BIO *b )
{
	if ( b == NULL ) return 0;

	b->ptr = NULL;		/* sb_tls_remove() will free it */
	b->init = 0;
	b->flags = 0;
	return 1;
}

static int
sb_tls_bio_read( BIO *b, char *buf, int len )
{
	struct tls_data		*p;
	int			ret;
		
	if ( buf == NULL || len <= 0 ) return 0;

	p = (struct tls_data *)b->ptr;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	ret = LBER_SBIOD_READ_NEXT( p->sbiod, buf, len );

	BIO_clear_retry_flags( b );
	if ( ret < 0 ) {
		int err = sock_errno();
		if ( err == EAGAIN || err == EWOULDBLOCK ) {
			BIO_set_retry_read( b );
		}
	}

	return ret;
}

static int
sb_tls_bio_write( BIO *b, const char *buf, int len )
{
	struct tls_data		*p;
	int			ret;
	
	if ( buf == NULL || len <= 0 ) return 0;
	
	p = (struct tls_data *)b->ptr;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	ret = LBER_SBIOD_WRITE_NEXT( p->sbiod, (char *)buf, len );

	BIO_clear_retry_flags( b );
	if ( ret < 0 ) {
		int err = sock_errno();
		if ( err == EAGAIN || err == EWOULDBLOCK ) {
			BIO_set_retry_write( b );
		}
	}

	return ret;
}

static long
sb_tls_bio_ctrl( BIO *b, int cmd, long num, void *ptr )
{
	if ( cmd == BIO_CTRL_FLUSH ) {
		/* The OpenSSL library needs this */
		return 1;
	}
	return 0;
}

static int
sb_tls_bio_gets( BIO *b, char *buf, int len )
{
	return -1;
}

static int
sb_tls_bio_puts( BIO *b, const char *str )
{
	return sb_tls_bio_write( b, str, strlen( str ) );
}
	
static BIO_METHOD sb_tls_bio_method =
{
	( 100 | 0x400 ),		/* it's a source/sink BIO */
	"sockbuf glue",
	sb_tls_bio_write,
	sb_tls_bio_read,
	sb_tls_bio_puts,
	sb_tls_bio_gets,
	sb_tls_bio_ctrl,
	sb_tls_bio_create,
	sb_tls_bio_destroy
};

static int
sb_tls_setup( Sockbuf_IO_Desc *sbiod, void *arg )
{
	struct tls_data		*p;
	BIO			*bio;

	assert( sbiod != NULL );

	p = LBER_MALLOC( sizeof( *p ) );
	if ( p == NULL ) {
		return -1;
	}
	
	p->ssl = (SSL *)arg;
	p->sbiod = sbiod;
	bio = BIO_new( &sb_tls_bio_method );
	bio->ptr = (void *)p;
	SSL_set_bio( p->ssl, bio, bio );
	sbiod->sbiod_pvt = p;
	return 0;
}

static int
sb_tls_remove( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	SSL_free( p->ssl );
	LBER_FREE( sbiod->sbiod_pvt );
	sbiod->sbiod_pvt = NULL;
	return 0;
}

static int
sb_tls_close( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	SSL_shutdown( p->ssl );
	return 0;
}

static int
sb_tls_ctrl( Sockbuf_IO_Desc *sbiod, int opt, void *arg )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	
	if ( opt == LBER_SB_OPT_GET_SSL ) {
		*((SSL **)arg) = p->ssl;
		return 1;

	} else if ( opt == LBER_SB_OPT_DATA_READY ) {
		if( SSL_pending( p->ssl ) > 0 ) {
			return 1;
		}
	}
	
	return LBER_SBIOD_CTRL_NEXT( sbiod, opt, arg );
}

static ber_slen_t
sb_tls_read( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = SSL_read( p->ssl, (char *)buf, len );
#ifdef HAVE_WINSOCK
	errno = WSAGetLastError();
#endif
	err = SSL_get_error( p->ssl, ret );
	if (err == SSL_ERROR_WANT_READ ) {
		sbiod->sbiod_sb->sb_trans_needs_read = 1;
		sock_errset(EWOULDBLOCK);
	}
	else
		sbiod->sbiod_sb->sb_trans_needs_read = 0;
	return ret;
}

static ber_slen_t
sb_tls_write( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = SSL_write( p->ssl, (char *)buf, len );
#ifdef HAVE_WINSOCK
	errno = WSAGetLastError();
#endif
	err = SSL_get_error( p->ssl, ret );
	if (err == SSL_ERROR_WANT_WRITE ) {
		sbiod->sbiod_sb->sb_trans_needs_write = 1;
		sock_errset(EWOULDBLOCK);

	} else {
		sbiod->sbiod_sb->sb_trans_needs_write = 0;
	}
	return ret;
}

#endif

static Sockbuf_IO sb_tls_sbio =
{
	sb_tls_setup,		/* sbi_setup */
	sb_tls_remove,		/* sbi_remove */
	sb_tls_ctrl,		/* sbi_ctrl */
	sb_tls_read,		/* sbi_read */
	sb_tls_write,		/* sbi_write */
	sb_tls_close		/* sbi_close */
};

#ifdef HAVE_GNUTLS
/* Certs are not automatically varified during the handshake */
static int
tls_cert_verify( tls_session *ssl )
{
	unsigned int status = 0;
	int err;
	time_t now = time(0);

	err = gnutls_certificate_verify_peers2( ssl->session, &status );
	if ( err < 0 ) {
		Debug( LDAP_DEBUG_ANY,"TLS: gnutls_certificate_verify_peers2 failed %d\n",
			err,0,0 );
		return -1;
	}
	if ( status ) {
		Debug( LDAP_DEBUG_TRACE,"TLS: peer cert untrusted or revoked (0x%x)\n",
			status, 0,0 );
		return -1;
	}
	if ( gnutls_certificate_expiration_time_peers( ssl->session ) < now ) {
		Debug( LDAP_DEBUG_ANY, "TLS: peer certificate is expired\n",
			0, 0, 0 );
		return -1;
	}
	if ( gnutls_certificate_activation_time_peers( ssl->session ) > now ) {
		Debug( LDAP_DEBUG_ANY, "TLS: peer certificate not yet active\n",
			0, 0, 0 );
		return -1;
	}
	return 0;
}
#endif /* HAVE_GNUTLS */

/*
 * Call this to do a TLS connect on a sockbuf. ctx_arg can be
 * a SSL_CTX * or NULL, in which case the default ctx is used.
 *
 * Return value:
 *
 *  0 - Success. Connection is ready for communication.
 * <0 - Error. Can't create a TLS stream.
 * >0 - Partial success.
 *	  Do a select (using information from lber_pvt_sb_needs_{read,write}
 *		and call again.
 */

static int
ldap_int_tls_connect( LDAP *ld, LDAPConn *conn )
{
	Sockbuf *sb = conn->lconn_sb;
	int	err;
	tls_session	*ssl;

	if ( HAS_TLS( sb ) ) {
		ber_sockbuf_ctrl( sb, LBER_SB_OPT_GET_SSL, (void *)&ssl );

	} else {
		struct ldapoptions *lo;
		tls_ctx *ctx;

		ctx = ld->ld_options.ldo_tls_ctx;

		ssl = alloc_handle( ctx, 0 );

		if ( ssl == NULL ) return -1;

#ifdef LDAP_DEBUG
		ber_sockbuf_add_io( sb, &ber_sockbuf_io_debug,
			LBER_SBIOD_LEVEL_TRANSPORT, (void *)"tls_" );
#endif
		ber_sockbuf_add_io( sb, &sb_tls_sbio,
			LBER_SBIOD_LEVEL_TRANSPORT, (void *)ssl );

		lo = LDAP_INT_GLOBAL_OPT();   
		if( ctx == NULL ) {
			ctx = lo->ldo_tls_ctx;
			ld->ld_options.ldo_tls_ctx = ctx;
			tls_ctx_ref( ctx );
		}
		if ( ld->ld_options.ldo_tls_connect_cb )
			ld->ld_options.ldo_tls_connect_cb( ld, ssl, ctx,
			ld->ld_options.ldo_tls_connect_arg );
		if ( lo && lo->ldo_tls_connect_cb && lo->ldo_tls_connect_cb !=
			ld->ld_options.ldo_tls_connect_cb )
			lo->ldo_tls_connect_cb( ld, ssl, ctx, lo->ldo_tls_connect_arg );
	}

	err = tls_session_connect( ssl );

#ifdef HAVE_WINSOCK
	errno = WSAGetLastError();
#endif

#ifdef HAVE_GNUTLS
	if ( err < 0 )
#else
	if ( err <= 0 )
#endif
	{
		if ( update_flags( sb, ssl, err )) {
			return 1;
		}

#ifndef HAVE_GNUTLS
		if ((err = ERR_peek_error()))
#endif
		{
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
#ifdef HAVE_GNUTLS
			ld->ld_error = LDAP_STRDUP(gnutls_strerror( err ));
#else
			{
				char buf[256];
				ld->ld_error = LDAP_STRDUP(ERR_error_string(err, buf));
			}
#endif
#ifdef HAVE_EBCDIC
			if ( ld->ld_error ) __etoa(ld->ld_error);
#endif
		}

		Debug( LDAP_DEBUG_ANY,"TLS: can't connect: %s.\n",
			ld->ld_error ? ld->ld_error : "" ,0,0);

		ber_sockbuf_remove_io( sb, &sb_tls_sbio,
			LBER_SBIOD_LEVEL_TRANSPORT );
#ifdef LDAP_DEBUG
		ber_sockbuf_remove_io( sb, &ber_sockbuf_io_debug,
			LBER_SBIOD_LEVEL_TRANSPORT );
#endif
		return -1;
	}

#ifdef HAVE_GNUTLS
	if ( ld->ld_options.ldo_tls_require_cert != LDAP_OPT_X_TLS_NEVER ) {
		err = tls_cert_verify( ssl );
		if ( err && ld->ld_options.ldo_tls_require_cert != LDAP_OPT_X_TLS_ALLOW )
			return err;
	}
#endif

	return 0;
}

/*
 * Call this to do a TLS accept on a sockbuf.
 * Everything else is the same as with tls_connect.
 */
int
ldap_pvt_tls_accept( Sockbuf *sb, void *ctx_arg )
{
	int	err;
	tls_session	*ssl;

	if ( HAS_TLS( sb ) ) {
		ber_sockbuf_ctrl( sb, LBER_SB_OPT_GET_SSL, (void *)&ssl );

	} else {
		ssl = alloc_handle( ctx_arg, 1 );
		if ( ssl == NULL ) return -1;

#ifdef LDAP_DEBUG
		ber_sockbuf_add_io( sb, &ber_sockbuf_io_debug,
			LBER_SBIOD_LEVEL_TRANSPORT, (void *)"tls_" );
#endif
		ber_sockbuf_add_io( sb, &sb_tls_sbio,
			LBER_SBIOD_LEVEL_TRANSPORT, (void *)ssl );
	}

	err = tls_session_accept( ssl );

#ifdef HAVE_WINSOCK
	errno = WSAGetLastError();
#endif

#ifdef HAVE_GNUTLS
	if ( err < 0 )
#else
	if ( err <= 0 )
#endif
	{
		if ( update_flags( sb, ssl, err )) return 1;

#ifdef HAVE_GNUTLS
		Debug( LDAP_DEBUG_ANY,"TLS: can't accept: %s.\n",
			gnutls_strerror( err ),0,0 );
#else
		Debug( LDAP_DEBUG_ANY,"TLS: can't accept.\n",0,0,0 );
		tls_report_error();
#endif
		ber_sockbuf_remove_io( sb, &sb_tls_sbio,
			LBER_SBIOD_LEVEL_TRANSPORT );
#ifdef LDAP_DEBUG
		ber_sockbuf_remove_io( sb, &ber_sockbuf_io_debug,
			LBER_SBIOD_LEVEL_TRANSPORT );
#endif
		return -1;
	}

#ifdef HAVE_GNUTLS
	if ( ssl->ctx->lo->ldo_tls_require_cert != LDAP_OPT_X_TLS_NEVER ) {
		err = tls_cert_verify( ssl );
		if ( err && ssl->ctx->lo->ldo_tls_require_cert != LDAP_OPT_X_TLS_ALLOW )
			return err;
	}
#endif
	return 0;
}

int
ldap_pvt_tls_inplace ( Sockbuf *sb )
{
	return HAS_TLS( sb ) ? 1 : 0;
}

int
ldap_tls_inplace( LDAP *ld )
{
	Sockbuf		*sb = NULL;

	if ( ld->ld_defconn && ld->ld_defconn->lconn_sb ) {
		sb = ld->ld_defconn->lconn_sb;

	} else if ( ld->ld_sb ) {
		sb = ld->ld_sb;

	} else {
		return 0;
	}

	return ldap_pvt_tls_inplace( sb );
}

#ifdef HAVE_GNUTLS
static void
x509_cert_get_dn( struct berval *cert, struct berval *dn, int get_subject )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t i;

	ber_init2( ber, cert, LBER_USE_DER );
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	tag = ber_skip_tag( ber, &len );	/* Context + Constructed (version) */
	if ( tag == 0xa0 )	/* Version is optional */
		tag = ber_get_int( ber, &i );	/* Int: Version */
	tag = ber_get_int( ber, &i );	/* Int: Serial */
	tag = ber_skip_tag( ber, &len );	/* Sequence: Signature */
	ber_skip_data( ber, len );
	if ( !get_subject ) {
		tag = ber_peek_tag( ber, &len );	/* Sequence: Issuer DN */
	} else {
		tag = ber_skip_tag( ber, &len );
		ber_skip_data( ber, len );
		tag = ber_skip_tag( ber, &len );	/* Sequence: Validity */
		ber_skip_data( ber, len );
		tag = ber_peek_tag( ber, &len );	/* Sequence: Subject DN */
	}
	len = ber_ptrlen( ber );
	dn->bv_val = cert->bv_val + len;
	dn->bv_len = cert->bv_len - len;
}

static int
tls_get_cert_dn( tls_session *session, struct berval *dnbv )
{
	if ( !session->peer_der_dn.bv_val ) {
		const gnutls_datum_t *peer_cert_list;
		int list_size;
		struct berval bv;

		peer_cert_list = gnutls_certificate_get_peers( session->session, 
							&list_size );
		if ( !peer_cert_list ) return LDAP_INVALID_CREDENTIALS;

		bv.bv_len = peer_cert_list->size;
		bv.bv_val = peer_cert_list->data;

		x509_cert_get_dn( &bv, &session->peer_der_dn, 1 );
		*dnbv = session->peer_der_dn;
	}
	return 0;
}
#else /* !HAVE_GNUTLS */
static X509 *
tls_get_cert( SSL *s )
{
	/* If peer cert was bad, treat as if no cert was given */
	if (SSL_get_verify_result(s)) {
		/* If we can send an alert, do so */
		if (SSL_version(s) != SSL2_VERSION) {
			ssl3_send_alert(s,SSL3_AL_WARNING,SSL3_AD_BAD_CERTIFICATE);
		}
		return NULL;
	}
	return SSL_get_peer_certificate(s);
}

static int
tls_get_cert_dn( tls_session *session, struct berval *dnbv )
{
	X509_NAME *xn;
	X509 *x = tls_get_cert( session );

	if ( !x )
		return LDAP_INVALID_CREDENTIALS;

	xn = X509_get_subject_name(x);
	dnbv->bv_len = i2d_X509_NAME( xn, NULL );
	dnbv->bv_val = xn->bytes->data;
	return 0;
}
#endif /* HAVE_GNUTLS */

int
ldap_pvt_tls_get_peer_dn( void *s, struct berval *dn,
	LDAPDN_rewrite_dummy *func, unsigned flags )
{
	tls_session *session = s;
	struct berval bvdn;
	int rc;

	rc = tls_get_cert_dn( session, &bvdn );
	if ( rc ) return rc;

	rc = ldap_X509dn2bv( &bvdn, dn, 
			    (LDAPDN_rewrite_func *)func, flags);
	return rc;
}

/* what kind of hostname were we given? */
#define	IS_DNS	0
#define	IS_IP4	1
#define	IS_IP6	2

#ifdef HAVE_GNUTLS

int
ldap_pvt_tls_check_hostname( LDAP *ld, void *s, const char *name_in )
{
	tls_session *session = s;
	int i, ret;
	const gnutls_datum_t *peer_cert_list;
	int list_size;
	struct berval bv;
	char altname[NI_MAXHOST];
	size_t altnamesize;

	gnutls_x509_crt_t cert;
	gnutls_datum_t *x;
	const char *name;
	char *ptr;
	char *domain = NULL;
#ifdef LDAP_PF_INET6
	struct in6_addr addr;
#else
	struct in_addr addr;
#endif
	int n, len1 = 0, len2 = 0;
	int ntype = IS_DNS;
	time_t now = time(0);

	if( ldap_int_hostname &&
		( !name_in || !strcasecmp( name_in, "localhost" ) ) )
	{
		name = ldap_int_hostname;
	} else {
		name = name_in;
	}

	peer_cert_list = gnutls_certificate_get_peers( session->session, 
						&list_size );
	if ( !peer_cert_list ) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: unable to get peer certificate.\n",
			0, 0, 0 );
		/* If this was a fatal condition, things would have
		 * aborted long before now.
		 */
		return LDAP_SUCCESS;
	}
	ret = gnutls_x509_crt_init( &cert );
	if ( ret < 0 )
		return LDAP_LOCAL_ERROR;
	ret = gnutls_x509_crt_import( cert, peer_cert_list, GNUTLS_X509_FMT_DER );
	if ( ret ) {
		gnutls_x509_crt_deinit( cert );
		return LDAP_LOCAL_ERROR;
	}

#ifdef LDAP_PF_INET6
	if (name[0] == '[' && strchr(name, ']')) {
		char *n2 = ldap_strdup(name+1);
		*strchr(n2, ']') = 2;
		if (inet_pton(AF_INET6, n2, &addr))
			ntype = IS_IP6;
		LDAP_FREE(n2);
	} else 
#endif
	if ((ptr = strrchr(name, '.')) && isdigit((unsigned char)ptr[1])) {
		if (inet_aton(name, (struct in_addr *)&addr)) ntype = IS_IP4;
	}
	
	if (ntype == IS_DNS) {
		len1 = strlen(name);
		domain = strchr(name, '.');
		if (domain) {
			len2 = len1 - (domain-name);
		}
	}

	for ( i=0, ret=0; ret >= 0; i++ ) {
		altnamesize = sizeof(altname);
		ret = gnutls_x509_crt_get_subject_alt_name( cert, i, 
			altname, &altnamesize, NULL );
		if ( ret < 0 ) break;

		/* ignore empty */
		if ( altnamesize == 0 ) continue;

		if ( ret == GNUTLS_SAN_DNSNAME ) {
			if (ntype != IS_DNS) continue;
	
			/* Is this an exact match? */
			if ((len1 == altnamesize) && !strncasecmp(name, altname, len1)) {
				break;
			}

			/* Is this a wildcard match? */
			if (domain && (altname[0] == '*') && (altname[1] == '.') &&
				(len2 == altnamesize-1) && !strncasecmp(domain, &altname[1], len2))
			{
				break;
			}
		} else if ( ret == GNUTLS_SAN_IPADDRESS ) {
			if (ntype == IS_DNS) continue;

#ifdef LDAP_PF_INET6
			if (ntype == IS_IP6 && altnamesize != sizeof(struct in6_addr)) {
				continue;
			} else
#endif
			if (ntype == IS_IP4 && altnamesize != sizeof(struct in_addr)) {
				continue;
			}
			if (!memcmp(altname, &addr, altnamesize)) {
				break;
			}
		}
	}
	if ( ret >= 0 ) {
		ret = LDAP_SUCCESS;
	} else {
		altnamesize = sizeof(altname);
		ret = gnutls_x509_crt_get_dn_by_oid( cert, CN_OID,
			0, 0, altname, &altnamesize );
		if ( ret < 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"TLS: unable to get common name from peer certificate.\n",
				0, 0, 0 );
			ret = LDAP_CONNECT_ERROR;
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP(
				_("TLS: unable to get CN from peer certificate"));

		} else {
			ret = LDAP_LOCAL_ERROR;
			if ( len1 == altnamesize && strncasecmp(name, altname, altnamesize) == 0 ) {
				ret = LDAP_SUCCESS;

			} else if (( altname[0] == '*' ) && ( altname[1] == '.' )) {
					/* Is this a wildcard match? */
				if( domain &&
					(len2 == altnamesize-1) && !strncasecmp(domain, &altname[1], len2)) {
					ret = LDAP_SUCCESS;
				}
			}
		}

		if( ret == LDAP_LOCAL_ERROR ) {
			altname[altnamesize] = '\0';
			Debug( LDAP_DEBUG_ANY, "TLS: hostname (%s) does not match "
				"common name in certificate (%s).\n", 
				name, altname, 0 );
			ret = LDAP_CONNECT_ERROR;
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP(
				_("TLS: hostname does not match CN in peer certificate"));
		}
	}
	gnutls_x509_crt_deinit( cert );
	return ret;
}

#else /* !HAVE_GNUTLS */

int
ldap_pvt_tls_check_hostname( LDAP *ld, void *s, const char *name_in )
{
	int i, ret = LDAP_LOCAL_ERROR;
	X509 *x;
	const char *name;
	char *ptr;
	int ntype = IS_DNS;
#ifdef LDAP_PF_INET6
	struct in6_addr addr;
#else
	struct in_addr addr;
#endif

	if( ldap_int_hostname &&
		( !name_in || !strcasecmp( name_in, "localhost" ) ) )
	{
		name = ldap_int_hostname;
	} else {
		name = name_in;
	}

	x = tls_get_cert((SSL *)s);
	if (!x) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: unable to get peer certificate.\n",
			0, 0, 0 );
		/* If this was a fatal condition, things would have
		 * aborted long before now.
		 */
		return LDAP_SUCCESS;
	}

#ifdef LDAP_PF_INET6
	if (name[0] == '[' && strchr(name, ']')) {
		char *n2 = ldap_strdup(name+1);
		*strchr(n2, ']') = 2;
		if (inet_pton(AF_INET6, n2, &addr))
			ntype = IS_IP6;
		LDAP_FREE(n2);
	} else 
#endif
	if ((ptr = strrchr(name, '.')) && isdigit((unsigned char)ptr[1])) {
		if (inet_aton(name, (struct in_addr *)&addr)) ntype = IS_IP4;
	}
	
	i = X509_get_ext_by_NID(x, NID_subject_alt_name, -1);
	if (i >= 0) {
		X509_EXTENSION *ex;
		STACK_OF(GENERAL_NAME) *alt;

		ex = X509_get_ext(x, i);
		alt = X509V3_EXT_d2i(ex);
		if (alt) {
			int n, len1 = 0, len2 = 0;
			char *domain = NULL;
			GENERAL_NAME *gn;

			if (ntype == IS_DNS) {
				len1 = strlen(name);
				domain = strchr(name, '.');
				if (domain) {
					len2 = len1 - (domain-name);
				}
			}
			n = sk_GENERAL_NAME_num(alt);
			for (i=0; i<n; i++) {
				char *sn;
				int sl;
				gn = sk_GENERAL_NAME_value(alt, i);
				if (gn->type == GEN_DNS) {
					if (ntype != IS_DNS) continue;

					sn = (char *) ASN1_STRING_data(gn->d.ia5);
					sl = ASN1_STRING_length(gn->d.ia5);

					/* ignore empty */
					if (sl == 0) continue;

					/* Is this an exact match? */
					if ((len1 == sl) && !strncasecmp(name, sn, len1)) {
						break;
					}

					/* Is this a wildcard match? */
					if (domain && (sn[0] == '*') && (sn[1] == '.') &&
						(len2 == sl-1) && !strncasecmp(domain, &sn[1], len2))
					{
						break;
					}

				} else if (gn->type == GEN_IPADD) {
					if (ntype == IS_DNS) continue;

					sn = (char *) ASN1_STRING_data(gn->d.ia5);
					sl = ASN1_STRING_length(gn->d.ia5);

#ifdef LDAP_PF_INET6
					if (ntype == IS_IP6 && sl != sizeof(struct in6_addr)) {
						continue;
					} else
#endif
					if (ntype == IS_IP4 && sl != sizeof(struct in_addr)) {
						continue;
					}
					if (!memcmp(sn, &addr, sl)) {
						break;
					}
				}
			}

			GENERAL_NAMES_free(alt);
			if (i < n) {	/* Found a match */
				ret = LDAP_SUCCESS;
			}
		}
	}

	if (ret != LDAP_SUCCESS) {
		X509_NAME *xn;
		char buf[2048];
		buf[0] = '\0';

		xn = X509_get_subject_name(x);
		if( X509_NAME_get_text_by_NID( xn, NID_commonName,
			buf, sizeof(buf)) == -1)
		{
			Debug( LDAP_DEBUG_ANY,
				"TLS: unable to get common name from peer certificate.\n",
				0, 0, 0 );
			ret = LDAP_CONNECT_ERROR;
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP(
				_("TLS: unable to get CN from peer certificate"));

		} else if (strcasecmp(name, buf) == 0 ) {
			ret = LDAP_SUCCESS;

		} else if (( buf[0] == '*' ) && ( buf[1] == '.' )) {
			char *domain = strchr(name, '.');
			if( domain ) {
				size_t dlen = 0;
				size_t sl;

				sl = strlen(name);
				dlen = sl - (domain-name);
				sl = strlen(buf);

				/* Is this a wildcard match? */
				if ((dlen == sl-1) && !strncasecmp(domain, &buf[1], dlen)) {
					ret = LDAP_SUCCESS;
				}
			}
		}

		if( ret == LDAP_LOCAL_ERROR ) {
			Debug( LDAP_DEBUG_ANY, "TLS: hostname (%s) does not match "
				"common name in certificate (%s).\n", 
				name, buf, 0 );
			ret = LDAP_CONNECT_ERROR;
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP(
				_("TLS: hostname does not match CN in peer certificate"));
		}
	}
	X509_free(x);
	return ret;
}
#endif

int
ldap_int_tls_config( LDAP *ld, int option, const char *arg )
{
	int i;

	switch( option ) {
	case LDAP_OPT_X_TLS_CACERTFILE:
	case LDAP_OPT_X_TLS_CACERTDIR:
	case LDAP_OPT_X_TLS_CERTFILE:
	case LDAP_OPT_X_TLS_KEYFILE:
	case LDAP_OPT_X_TLS_RANDOM_FILE:
	case LDAP_OPT_X_TLS_CIPHER_SUITE:
	case LDAP_OPT_X_TLS_DHFILE:
#ifdef HAVE_GNUTLS
	case LDAP_OPT_X_TLS_CRLFILE:
#endif
		return ldap_pvt_tls_set_option( ld, option, (void *) arg );

	case LDAP_OPT_X_TLS_REQUIRE_CERT:
	case LDAP_OPT_X_TLS:
		i = -1;
		if ( strcasecmp( arg, "never" ) == 0 ) {
			i = LDAP_OPT_X_TLS_NEVER ;

		} else if ( strcasecmp( arg, "demand" ) == 0 ) {
			i = LDAP_OPT_X_TLS_DEMAND ;

		} else if ( strcasecmp( arg, "allow" ) == 0 ) {
			i = LDAP_OPT_X_TLS_ALLOW ;

		} else if ( strcasecmp( arg, "try" ) == 0 ) {
			i = LDAP_OPT_X_TLS_TRY ;

		} else if ( ( strcasecmp( arg, "hard" ) == 0 ) ||
			( strcasecmp( arg, "on" ) == 0 ) ||
			( strcasecmp( arg, "yes" ) == 0) ||
			( strcasecmp( arg, "true" ) == 0 ) )
		{
			i = LDAP_OPT_X_TLS_HARD ;
		}

		if (i >= 0) {
			return ldap_pvt_tls_set_option( ld, option, &i );
		}
		return -1;
#ifdef HAVE_OPENSSL_CRL
	case LDAP_OPT_X_TLS_CRLCHECK:
		i = -1;
		if ( strcasecmp( arg, "none" ) == 0 ) {
			i = LDAP_OPT_X_TLS_CRL_NONE ;
		} else if ( strcasecmp( arg, "peer" ) == 0 ) {
			i = LDAP_OPT_X_TLS_CRL_PEER ;
		} else if ( strcasecmp( arg, "all" ) == 0 ) {
			i = LDAP_OPT_X_TLS_CRL_ALL ;
		}
		if (i >= 0) {
			return ldap_pvt_tls_set_option( ld, option, &i );
		}
		return -1;
#endif
	}
	return -1;
}

int
ldap_pvt_tls_get_option( LDAP *ld, int option, void *arg )
{
	struct ldapoptions *lo;

	if( ld != NULL ) {
		assert( LDAP_VALID( ld ) );

		if( !LDAP_VALID( ld ) ) {
			return LDAP_OPT_ERROR;
		}

		lo = &ld->ld_options;

	} else {
		/* Get pointer to global option structure */
		lo = LDAP_INT_GLOBAL_OPT();   
		if ( lo == NULL ) {
			return LDAP_NO_MEMORY;
		}
	}

	switch( option ) {
	case LDAP_OPT_X_TLS:
		*(int *)arg = lo->ldo_tls_mode;
		break;
	case LDAP_OPT_X_TLS_CTX:
		*(void **)arg = lo->ldo_tls_ctx;
		if ( lo->ldo_tls_ctx ) {
			tls_ctx_ref( lo->ldo_tls_ctx );
		}
		break;
	case LDAP_OPT_X_TLS_CACERTFILE:
		*(char **)arg = lo->ldo_tls_cacertfile ?
			LDAP_STRDUP( lo->ldo_tls_cacertfile ) : NULL;
		break;
	case LDAP_OPT_X_TLS_CACERTDIR:
		*(char **)arg = lo->ldo_tls_cacertdir ?
			LDAP_STRDUP( lo->ldo_tls_cacertdir ) : NULL;
		break;
	case LDAP_OPT_X_TLS_CERTFILE:
		*(char **)arg = lo->ldo_tls_certfile ?
			LDAP_STRDUP( lo->ldo_tls_certfile ) : NULL;
		break;
	case LDAP_OPT_X_TLS_KEYFILE:
		*(char **)arg = lo->ldo_tls_keyfile ?
			LDAP_STRDUP( lo->ldo_tls_keyfile ) : NULL;
		break;
	case LDAP_OPT_X_TLS_DHFILE:
		*(char **)arg = lo->ldo_tls_dhfile ?
			LDAP_STRDUP( lo->ldo_tls_dhfile ) : NULL;
		break;
#ifdef HAVE_GNUTLS
	case LDAP_OPT_X_TLS_CRLFILE:
		*(char **)arg = lo->ldo_tls_crlfile ?
			LDAP_STRDUP( lo->ldo_tls_crlfile ) : NULL;
		break;
#endif
	case LDAP_OPT_X_TLS_REQUIRE_CERT:
		*(int *)arg = lo->ldo_tls_require_cert;
		break;
#ifdef HAVE_OPENSSL_CRL
	case LDAP_OPT_X_TLS_CRLCHECK:
		*(int *)arg = lo->ldo_tls_crlcheck;
		break;
#endif
	case LDAP_OPT_X_TLS_CIPHER_SUITE:
		*(char **)arg = lo->ldo_tls_ciphersuite ?
			LDAP_STRDUP( lo->ldo_tls_ciphersuite ) : NULL;
		break;
	case LDAP_OPT_X_TLS_RANDOM_FILE:
#ifdef HAVE_OPENSSL
		*(char **)arg = tls_opt_randfile ?
			LDAP_STRDUP( tls_opt_randfile ) : NULL;
#else
		*(char **)arg = NULL;
#endif
		break;
	case LDAP_OPT_X_TLS_SSL_CTX: {
		void *retval = 0;
		if ( ld != NULL ) {
			LDAPConn *conn = ld->ld_defconn;
			if ( conn != NULL ) {
				Sockbuf *sb = conn->lconn_sb;
				retval = ldap_pvt_tls_sb_ctx( sb );
			}
		}
		*(void **)arg = retval;
		break;
	}
	case LDAP_OPT_X_TLS_CONNECT_CB:
		*(LDAP_TLS_CONNECT_CB **)arg = lo->ldo_tls_connect_cb;
		break;
	case LDAP_OPT_X_TLS_CONNECT_ARG:
		*(void **)arg = lo->ldo_tls_connect_arg;
		break;
	default:
		return -1;
	}
	return 0;
}

int
ldap_pvt_tls_set_option( LDAP *ld, int option, void *arg )
{
	struct ldapoptions *lo;

	if( ld != NULL ) {
		assert( LDAP_VALID( ld ) );

		if( !LDAP_VALID( ld ) ) {
			return LDAP_OPT_ERROR;
		}

		lo = &ld->ld_options;

	} else {
		/* Get pointer to global option structure */
		lo = LDAP_INT_GLOBAL_OPT();   
		if ( lo == NULL ) {
			return LDAP_NO_MEMORY;
		}
	}

	switch( option ) {
	case LDAP_OPT_X_TLS:
		if ( !arg ) return -1;

		switch( *(int *) arg ) {
		case LDAP_OPT_X_TLS_NEVER:
		case LDAP_OPT_X_TLS_DEMAND:
		case LDAP_OPT_X_TLS_ALLOW:
		case LDAP_OPT_X_TLS_TRY:
		case LDAP_OPT_X_TLS_HARD:
			if (lo != NULL) {
				lo->ldo_tls_mode = *(int *)arg;
			}

			return 0;
		}
		return -1;

	case LDAP_OPT_X_TLS_CTX:
		if ( lo->ldo_tls_ctx )
			ldap_pvt_tls_ctx_free( lo->ldo_tls_ctx );
		lo->ldo_tls_ctx = arg;
		tls_ctx_ref( lo->ldo_tls_ctx );
		return 0;
	case LDAP_OPT_X_TLS_CONNECT_CB:
		lo->ldo_tls_connect_cb = (LDAP_TLS_CONNECT_CB *)arg;
		return 0;
	case LDAP_OPT_X_TLS_CONNECT_ARG:
		lo->ldo_tls_connect_arg = arg;
		return 0;
	case LDAP_OPT_X_TLS_CACERTFILE:
		if ( lo->ldo_tls_cacertfile ) LDAP_FREE( lo->ldo_tls_cacertfile );
		lo->ldo_tls_cacertfile = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
		return 0;
	case LDAP_OPT_X_TLS_CACERTDIR:
		if ( lo->ldo_tls_cacertdir ) LDAP_FREE( lo->ldo_tls_cacertdir );
		lo->ldo_tls_cacertdir = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
		return 0;
	case LDAP_OPT_X_TLS_CERTFILE:
		if ( lo->ldo_tls_certfile ) LDAP_FREE( lo->ldo_tls_certfile );
		lo->ldo_tls_certfile = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
		return 0;
	case LDAP_OPT_X_TLS_KEYFILE:
		if ( lo->ldo_tls_keyfile ) LDAP_FREE( lo->ldo_tls_keyfile );
		lo->ldo_tls_keyfile = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
		return 0;
	case LDAP_OPT_X_TLS_DHFILE:
		if ( lo->ldo_tls_dhfile ) LDAP_FREE( lo->ldo_tls_dhfile );
		lo->ldo_tls_dhfile = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
		return 0;
#ifdef HAVE_GNUTLS
	case LDAP_OPT_X_TLS_CRLFILE:
		if ( lo->ldo_tls_crlfile ) LDAP_FREE( lo->ldo_tls_crlfile );
		lo->ldo_tls_crlfile = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
		return 0;
#endif
	case LDAP_OPT_X_TLS_REQUIRE_CERT:
		if ( !arg ) return -1;
		switch( *(int *) arg ) {
		case LDAP_OPT_X_TLS_NEVER:
		case LDAP_OPT_X_TLS_DEMAND:
		case LDAP_OPT_X_TLS_ALLOW:
		case LDAP_OPT_X_TLS_TRY:
		case LDAP_OPT_X_TLS_HARD:
			lo->ldo_tls_require_cert = * (int *) arg;
			return 0;
		}
		return -1;
#ifdef HAVE_OPENSSL_CRL
	case LDAP_OPT_X_TLS_CRLCHECK:
		if ( !arg ) return -1;
		switch( *(int *) arg ) {
		case LDAP_OPT_X_TLS_CRL_NONE:
		case LDAP_OPT_X_TLS_CRL_PEER:
		case LDAP_OPT_X_TLS_CRL_ALL:
			lo->ldo_tls_crlcheck = * (int *) arg;
			return 0;
		}
		return -1;
#endif
	case LDAP_OPT_X_TLS_CIPHER_SUITE:
		if ( lo->ldo_tls_ciphersuite ) LDAP_FREE( lo->ldo_tls_ciphersuite );
		lo->ldo_tls_ciphersuite = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
		return 0;

	case LDAP_OPT_X_TLS_RANDOM_FILE:
		if ( ld != NULL )
			return -1;
#ifdef HAVE_OPENSSL
		if (tls_opt_randfile ) LDAP_FREE (tls_opt_randfile );
		tls_opt_randfile = arg ? LDAP_STRDUP( (char *) arg ) : NULL;
#endif
		break;

	case LDAP_OPT_X_TLS_NEWCTX:
		if ( !arg ) return -1;
		if ( lo->ldo_tls_ctx )
			ldap_pvt_tls_ctx_free( lo->ldo_tls_ctx );
		lo->ldo_tls_ctx = NULL;
		return ldap_int_tls_init_ctx( lo, *(int *)arg );
	default:
		return -1;
	}
	return 0;
}

int
ldap_int_tls_start ( LDAP *ld, LDAPConn *conn, LDAPURLDesc *srv )
{
	Sockbuf *sb = conn->lconn_sb;
	char *host;
	void *ssl;

	if( srv ) {
		host = srv->lud_host;
	} else {
 		host = conn->lconn_server->lud_host;
	}

	/* avoid NULL host */
	if( host == NULL ) {
		host = "localhost";
	}

	(void) ldap_pvt_tls_init();

	/*
	 * Fortunately, the lib uses blocking io...
	 */
	if ( ldap_int_tls_connect( ld, conn ) < 0 ) {
		ld->ld_errno = LDAP_CONNECT_ERROR;
		return (ld->ld_errno);
	}

	ssl = ldap_pvt_tls_sb_ctx( sb );
	assert( ssl != NULL );

	/* 
	 * compare host with name(s) in certificate
	 */
	if (ld->ld_options.ldo_tls_require_cert != LDAP_OPT_X_TLS_NEVER) {
		ld->ld_errno = ldap_pvt_tls_check_hostname( ld, ssl, host );
		if (ld->ld_errno != LDAP_SUCCESS) {
			return ld->ld_errno;
		}
	}

	return LDAP_SUCCESS;
}

#ifdef HAVE_OPENSSL
/* Derived from openssl/apps/s_cb.c */
static void
tls_info_cb( const SSL *ssl, int where, int ret )
{
	int w;
	char *op;
	char *state = (char *) SSL_state_string_long( (SSL *)ssl );

	w = where & ~SSL_ST_MASK;
	if ( w & SSL_ST_CONNECT ) {
		op = "SSL_connect";
	} else if ( w & SSL_ST_ACCEPT ) {
		op = "SSL_accept";
	} else {
		op = "undefined";
	}

#ifdef HAVE_EBCDIC
	if ( state ) {
		state = LDAP_STRDUP( state );
		__etoa( state );
	}
#endif
	if ( where & SSL_CB_LOOP ) {
		Debug( LDAP_DEBUG_TRACE,
			   "TLS trace: %s:%s\n",
			   op, state, 0 );

	} else if ( where & SSL_CB_ALERT ) {
		char *atype = (char *) SSL_alert_type_string_long( ret );
		char *adesc = (char *) SSL_alert_desc_string_long( ret );
		op = ( where & SSL_CB_READ ) ? "read" : "write";
#ifdef HAVE_EBCDIC
		if ( atype ) {
			atype = LDAP_STRDUP( atype );
			__etoa( atype );
		}
		if ( adesc ) {
			adesc = LDAP_STRDUP( adesc );
			__etoa( adesc );
		}
#endif
		Debug( LDAP_DEBUG_TRACE,
			   "TLS trace: SSL3 alert %s:%s:%s\n",
			   op, atype, adesc );
#ifdef HAVE_EBCDIC
		if ( atype ) LDAP_FREE( atype );
		if ( adesc ) LDAP_FREE( adesc );
#endif
	} else if ( where & SSL_CB_EXIT ) {
		if ( ret == 0 ) {
			Debug( LDAP_DEBUG_TRACE,
				   "TLS trace: %s:failed in %s\n",
				   op, state, 0 );
		} else if ( ret < 0 ) {
			Debug( LDAP_DEBUG_TRACE,
				   "TLS trace: %s:error in %s\n",
				   op, state, 0 );
		}
	}
#ifdef HAVE_EBCDIC
	if ( state ) LDAP_FREE( state );
#endif
}

static int
tls_verify_cb( int ok, X509_STORE_CTX *ctx )
{
	X509 *cert;
	int errnum;
	int errdepth;
	X509_NAME *subject;
	X509_NAME *issuer;
	char *sname;
	char *iname;
	char *certerr = NULL;

	cert = X509_STORE_CTX_get_current_cert( ctx );
	errnum = X509_STORE_CTX_get_error( ctx );
	errdepth = X509_STORE_CTX_get_error_depth( ctx );

	/*
	 * X509_get_*_name return pointers to the internal copies of
	 * those things requested.  So do not free them.
	 */
	subject = X509_get_subject_name( cert );
	issuer = X509_get_issuer_name( cert );
	/* X509_NAME_oneline, if passed a NULL buf, allocate memomry */
	sname = X509_NAME_oneline( subject, NULL, 0 );
	iname = X509_NAME_oneline( issuer, NULL, 0 );
	if ( !ok ) certerr = (char *)X509_verify_cert_error_string( errnum );
#ifdef HAVE_EBCDIC
	if ( sname ) __etoa( sname );
	if ( iname ) __etoa( iname );
	if ( certerr ) {
		certerr = LDAP_STRDUP( certerr );
		__etoa( certerr );
	}
#endif
	Debug( LDAP_DEBUG_TRACE,
		   "TLS certificate verification: depth: %d, err: %d, subject: %s,",
		   errdepth, errnum,
		   sname ? sname : "-unknown-" );
	Debug( LDAP_DEBUG_TRACE, " issuer: %s\n", iname ? iname : "-unknown-", 0, 0 );
	if ( !ok ) {
		Debug( LDAP_DEBUG_ANY,
			"TLS certificate verification: Error, %s\n",
			certerr, 0, 0 );
	}
	if ( sname )
		CRYPTO_free ( sname );
	if ( iname )
		CRYPTO_free ( iname );
#ifdef HAVE_EBCDIC
	if ( certerr ) LDAP_FREE( certerr );
#endif
	return ok;
}

static int
tls_verify_ok( int ok, X509_STORE_CTX *ctx )
{
	(void) tls_verify_cb( ok, ctx );
	return 1;
}

/* Inspired by ERR_print_errors in OpenSSL */
static void
tls_report_error( void )
{
	unsigned long l;
	char buf[200];
	const char *file;
	int line;

	while ( ( l = ERR_get_error_line( &file, &line ) ) != 0 ) {
		ERR_error_string_n( l, buf, sizeof( buf ) );
#ifdef HAVE_EBCDIC
		if ( file ) {
			file = LDAP_STRDUP( file );
			__etoa( (char *)file );
		}
		__etoa( buf );
#endif
		Debug( LDAP_DEBUG_ANY, "TLS: %s %s:%d\n",
			buf, file, line );
#ifdef HAVE_EBCDIC
		if ( file ) LDAP_FREE( (void *)file );
#endif
	}
}

static RSA *
tls_tmp_rsa_cb( SSL *ssl, int is_export, int key_length )
{
	RSA *tmp_rsa;

	/* FIXME:  Pregenerate the key on startup */
	/* FIXME:  Who frees the key? */
	tmp_rsa = RSA_generate_key( key_length, RSA_F4, NULL, NULL );

	if ( !tmp_rsa ) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: Failed to generate temporary %d-bit %s RSA key\n",
			key_length, is_export ? "export" : "domestic", 0 );
		return NULL;
	}
	return tmp_rsa;
}

static int
tls_seed_PRNG( const char *randfile )
{
#ifndef URANDOM_DEVICE
	/* no /dev/urandom (or equiv) */
	long total=0;
	char buffer[MAXPATHLEN];

	if (randfile == NULL) {
		/* The seed file is $RANDFILE if defined, otherwise $HOME/.rnd.
		 * If $HOME is not set or buffer too small to hold the pathname,
		 * an error occurs.	- From RAND_file_name() man page.
		 * The fact is that when $HOME is NULL, .rnd is used.
		 */
		randfile = RAND_file_name( buffer, sizeof( buffer ) );

	} else if (RAND_egd(randfile) > 0) {
		/* EGD socket */
		return 0;
	}

	if (randfile == NULL) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: Use configuration file or $RANDFILE to define seed PRNG\n",
			0, 0, 0);
		return -1;
	}

	total = RAND_load_file(randfile, -1);

	if (RAND_status() == 0) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: PRNG not been seeded with enough data\n",
			0, 0, 0);
		return -1;
	}

	/* assume if there was enough bits to seed that it's okay
	 * to write derived bits to the file
	 */
	RAND_write_file(randfile);

#endif

	return 0;
}

struct dhinfo {
	int keylength;
	const char *pem;
	size_t size;
};


/* From the OpenSSL 0.9.7 distro */
static const char dhpem512[] =
"-----BEGIN DH PARAMETERS-----\n\
MEYCQQDaWDwW2YUiidDkr3VvTMqS3UvlM7gE+w/tlO+cikQD7VdGUNNpmdsp13Yn\n\
a6LT1BLiGPTdHghM9tgAPnxHdOgzAgEC\n\
-----END DH PARAMETERS-----\n";

static const char dhpem1024[] =
"-----BEGIN DH PARAMETERS-----\n\
MIGHAoGBAJf2QmHKtQXdKCjhPx1ottPb0PMTBH9A6FbaWMsTuKG/K3g6TG1Z1fkq\n\
/Gz/PWk/eLI9TzFgqVAuPvr3q14a1aZeVUMTgo2oO5/y2UHe6VaJ+trqCTat3xlx\n\
/mNbIK9HA2RgPC3gWfVLZQrY+gz3ASHHR5nXWHEyvpuZm7m3h+irAgEC\n\
-----END DH PARAMETERS-----\n";

static const char dhpem2048[] =
"-----BEGIN DH PARAMETERS-----\n\
MIIBCAKCAQEA7ZKJNYJFVcs7+6J2WmkEYb8h86tT0s0h2v94GRFS8Q7B4lW9aG9o\n\
AFO5Imov5Jo0H2XMWTKKvbHbSe3fpxJmw/0hBHAY8H/W91hRGXKCeyKpNBgdL8sh\n\
z22SrkO2qCnHJ6PLAMXy5fsKpFmFor2tRfCzrfnggTXu2YOzzK7q62bmqVdmufEo\n\
pT8igNcLpvZxk5uBDvhakObMym9mX3rAEBoe8PwttggMYiiw7NuJKO4MqD1llGkW\n\
aVM8U2ATsCun1IKHrRxynkE1/MJ86VHeYYX8GZt2YA8z+GuzylIOKcMH6JAWzMwA\n\
Gbatw6QwizOhr9iMjZ0B26TE3X8LvW84wwIBAg==\n\
-----END DH PARAMETERS-----\n";

static const char dhpem4096[] =
"-----BEGIN DH PARAMETERS-----\n\
MIICCAKCAgEA/urRnb6vkPYc/KEGXWnbCIOaKitq7ySIq9dTH7s+Ri59zs77zty7\n\
vfVlSe6VFTBWgYjD2XKUFmtqq6CqXMhVX5ElUDoYDpAyTH85xqNFLzFC7nKrff/H\n\
TFKNttp22cZE9V0IPpzedPfnQkE7aUdmF9JnDyv21Z/818O93u1B4r0szdnmEvEF\n\
bKuIxEHX+bp0ZR7RqE1AeifXGJX3d6tsd2PMAObxwwsv55RGkn50vHO4QxtTARr1\n\
rRUV5j3B3oPMgC7Offxx+98Xn45B1/G0Prp11anDsR1PGwtaCYipqsvMwQUSJtyE\n\
EOQWk+yFkeMe4vWv367eEi0Sd/wnC+TSXBE3pYvpYerJ8n1MceI5GQTdarJ77OW9\n\
bGTHmxRsLSCM1jpLdPja5jjb4siAa6EHc4qN9c/iFKS3PQPJEnX7pXKBRs5f7AF3\n\
W3RIGt+G9IVNZfXaS7Z/iCpgzgvKCs0VeqN38QsJGtC1aIkwOeyjPNy2G6jJ4yqH\n\
ovXYt/0mc00vCWeSNS1wren0pR2EiLxX0ypjjgsU1mk/Z3b/+zVf7fZSIB+nDLjb\n\
NPtUlJCVGnAeBK1J1nG3TQicqowOXoM6ISkdaXj5GPJdXHab2+S7cqhKGv5qC7rR\n\
jT6sx7RUr0CNTxzLI7muV2/a4tGmj0PSdXQdsZ7tw7gbXlaWT1+MM2MCAQI=\n\
-----END DH PARAMETERS-----\n";

static const struct dhinfo dhpem[] = {
	{ 512, dhpem512, sizeof(dhpem512) },
	{ 1024, dhpem1024, sizeof(dhpem1024) },
	{ 2048, dhpem2048, sizeof(dhpem2048) },
	{ 4096, dhpem4096, sizeof(dhpem4096) },
	{ 0, NULL, 0 }
};

static DH *
tls_tmp_dh_cb( SSL *ssl, int is_export, int key_length )
{
	struct dhplist *p = NULL;
	BIO *b = NULL;
	DH *dh = NULL;
	int i;

	/* Do we have params of this length already? */
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &tls_def_ctx_mutex );
#endif
	for ( p = dhparams; p; p=p->next ) {
		if ( p->keylength == key_length ) {
#ifdef LDAP_R_COMPILE
			ldap_pvt_thread_mutex_unlock( &tls_def_ctx_mutex );
#endif
			return p->param;
		}
	}

	/* No - check for hardcoded params */

	for (i=0; dhpem[i].keylength; i++) {
		if ( dhpem[i].keylength == key_length ) {
			b = BIO_new_mem_buf( (char *)dhpem[i].pem, dhpem[i].size );
			break;
		}
	}

	if ( b ) {
		dh = PEM_read_bio_DHparams( b, NULL, NULL, NULL );
		BIO_free( b );
	}

	/* Generating on the fly is expensive/slow... */
	if ( !dh ) {
		dh = DH_generate_parameters( key_length, DH_GENERATOR_2, NULL, NULL );
	}
	if ( dh ) {
		p = LDAP_MALLOC( sizeof(struct dhplist) );
		if ( p != NULL ) {
			p->keylength = key_length;
			p->param = dh;
			p->next = dhparams;
			dhparams = p;
		}
	}

#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &tls_def_ctx_mutex );
#endif
	return dh;
}
#endif

#endif /* HAVE_OPENSSL */

void *
ldap_pvt_tls_sb_ctx( Sockbuf *sb )
{
#ifdef HAVE_TLS
	void			*p;
	
	if (HAS_TLS( sb )) {
		ber_sockbuf_ctrl( sb, LBER_SB_OPT_GET_SSL, (void *)&p );
		return p;
	}
#endif

	return NULL;
}

int
ldap_pvt_tls_get_strength( void *s )
{
#ifdef HAVE_OPENSSL
	const SSL_CIPHER *c;

	c = SSL_get_current_cipher((const SSL *)s);
	return SSL_CIPHER_get_bits(c, NULL);
#elif defined(HAVE_GNUTLS)
	tls_session *session = s;
	gnutls_cipher_algorithm_t c;

	c = gnutls_cipher_get( session->session );
	return gnutls_cipher_get_key_size( c ) * 8;
#else
	return 0;
#endif
}


int
ldap_pvt_tls_get_my_dn( void *s, struct berval *dn, LDAPDN_rewrite_dummy *func, unsigned flags )
{
#ifdef HAVE_TLS
	struct berval der_dn;
	int rc;
#ifdef HAVE_OPENSSL
	X509 *x;
	X509_NAME *xn;

	x = SSL_get_certificate((SSL *)s);

	if (!x) return LDAP_INVALID_CREDENTIALS;
	
	xn = X509_get_subject_name(x);
	der_dn.bv_len = i2d_X509_NAME( xn, NULL );
	der_dn.bv_val = xn->bytes->data;
#elif defined(HAVE_GNUTLS)
	tls_session *session = s;
	const gnutls_datum_t *x;
	struct berval bv;

	x = gnutls_certificate_get_ours( session->session );

	if (!x) return LDAP_INVALID_CREDENTIALS;
	
	bv.bv_val = x->data;
	bv.bv_len = x->size;

	x509_cert_get_dn( &bv, &der_dn, 1 );
#endif
	rc = ldap_X509dn2bv(&der_dn, dn, (LDAPDN_rewrite_func *)func, flags );
	return rc;
#else /* !HAVE_TLS */
	return LDAP_NOT_SUPPORTED;
#endif
}

int
ldap_start_tls( LDAP *ld,
	LDAPControl **serverctrls,
	LDAPControl **clientctrls,
	int *msgidp )
{
	return ldap_extended_operation( ld, LDAP_EXOP_START_TLS,
		NULL, serverctrls, clientctrls, msgidp );
}

int
ldap_install_tls( LDAP *ld )
{
#ifndef HAVE_TLS
	return LDAP_NOT_SUPPORTED;
#else
	if ( ldap_tls_inplace( ld ) ) {
		return LDAP_LOCAL_ERROR;
	}

	return ldap_int_tls_start( ld, ld->ld_defconn, NULL );
#endif
}

int
ldap_start_tls_s ( LDAP *ld,
	LDAPControl **serverctrls,
	LDAPControl **clientctrls )
{
#ifndef HAVE_TLS
	return LDAP_NOT_SUPPORTED;
#else
	int rc;
	char *rspoid = NULL;
	struct berval *rspdata = NULL;

	/* XXYYZ: this initiates operation only on default connection! */

	if ( ldap_tls_inplace( ld ) ) {
		return LDAP_LOCAL_ERROR;
	}

	rc = ldap_extended_operation_s( ld, LDAP_EXOP_START_TLS,
		NULL, serverctrls, clientctrls, &rspoid, &rspdata );

	if ( rspoid != NULL ) {
		LDAP_FREE(rspoid);
	}

	if ( rspdata != NULL ) {
		ber_bvfree( rspdata );
	}

	if ( rc == LDAP_SUCCESS ) {
		rc = ldap_int_tls_start( ld, ld->ld_defconn, NULL );
	}

	return rc;
#endif
}

/* These tags probably all belong in lber.h, but they're
 * not normally encountered when processing LDAP, so maybe
 * they belong somewhere else instead.
 */

#define LBER_TAG_OID		((ber_tag_t) 0x06UL)

/* Tags for string types used in a DirectoryString.
 *
 * Note that IA5string is not one of the defined choices for
 * DirectoryString in X.520, but it gets used for email AVAs.
 */
#define	LBER_TAG_UTF8		((ber_tag_t) 0x0cUL)
#define	LBER_TAG_PRINTABLE	((ber_tag_t) 0x13UL)
#define	LBER_TAG_TELETEX	((ber_tag_t) 0x14UL)
#define	LBER_TAG_IA5		((ber_tag_t) 0x16UL)
#define	LBER_TAG_UNIVERSAL	((ber_tag_t) 0x1cUL)
#define	LBER_TAG_BMP		((ber_tag_t) 0x1eUL)

static oid_name *
find_oid( struct berval *oid )
{
	int i;

	for ( i=0; !BER_BVISNULL( &oids[i].oid ); i++ ) {
		if ( oids[i].oid.bv_len != oid->bv_len ) continue;
		if ( !strcmp( oids[i].oid.bv_val, oid->bv_val ))
			return &oids[i];
	}
	return NULL;
}

/* Convert a structured DN from an X.509 certificate into an LDAPV3 DN.
 * x509_name must be raw DER. If func is non-NULL, the
 * constructed DN will use numeric OIDs to identify attributeTypes,
 * and the func() will be invoked to rewrite the DN with the given
 * flags.
 *
 * Otherwise the DN will use shortNames from a hardcoded table.
 */
int
ldap_X509dn2bv( void *x509_name, struct berval *bv, LDAPDN_rewrite_func *func,
	unsigned flags )
{
	LDAPDN	newDN;
	LDAPRDN	newRDN;
	LDAPAVA *newAVA, *baseAVA;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	char oids[8192], *oidptr = oids, *oidbuf = NULL;
	void *ptrs[2048];
	char *dn_end, *rdn_end;
	int i, navas, nrdns, rc = LDAP_SUCCESS;
	size_t dnsize, oidrem = sizeof(oids), oidsize = 0;
	int csize;
	ber_tag_t tag;
	ber_len_t len;
	oid_name *oidname;

	struct berval	Oid, Val, oid2, *in = x509_name;

	assert( bv != NULL );

	bv->bv_len = 0;
	bv->bv_val = NULL;

	navas = 0;
	nrdns = 0;

	/* A DN is a SEQUENCE of RDNs. An RDN is a SET of AVAs.
	 * An AVA is a SEQUENCE of attr and value.
	 * Count the number of AVAs and RDNs
	 */
	ber_init2( ber, in, LBER_USE_DER );
	tag = ber_peek_tag( ber, &len );
	if ( tag != LBER_SEQUENCE )
		return LDAP_DECODING_ERROR;

	for ( tag = ber_first_element( ber, &len, &dn_end );
		tag == LBER_SET;
		tag = ber_next_element( ber, &len, dn_end )) {
		nrdns++;
		for ( tag = ber_first_element( ber, &len, &rdn_end );
			tag == LBER_SEQUENCE;
			tag = ber_next_element( ber, &len, rdn_end )) {
			tag = ber_skip_tag( ber, &len );
			ber_skip_data( ber, len );
			navas++;
		}
	}

	/* Allocate the DN/RDN/AVA stuff as a single block */    
	dnsize = sizeof(LDAPRDN) * (nrdns+1);
	dnsize += sizeof(LDAPAVA *) * (navas+nrdns);
	dnsize += sizeof(LDAPAVA) * navas;
	if (dnsize > sizeof(ptrs)) {
		newDN = (LDAPDN)LDAP_MALLOC( dnsize );
		if ( newDN == NULL )
			return LDAP_NO_MEMORY;
	} else {
		newDN = (LDAPDN)(char *)ptrs;
	}
	
	newDN[nrdns] = NULL;
	newRDN = (LDAPRDN)(newDN + nrdns+1);
	newAVA = (LDAPAVA *)(newRDN + navas + nrdns);
	baseAVA = newAVA;

	/* Rewind and start extracting */
	ber_rewind( ber );

	tag = ber_first_element( ber, &len, &dn_end );
	for ( i = nrdns - 1; i >= 0; i-- ) {
		newDN[i] = newRDN;

		for ( tag = ber_first_element( ber, &len, &rdn_end );
			tag == LBER_SEQUENCE;
			tag = ber_next_element( ber, &len, rdn_end )) {

			*newRDN++ = newAVA;
			tag = ber_skip_tag( ber, &len );
			tag = ber_get_stringbv( ber, &Oid, LBER_BV_NOTERM );
			if ( tag != LBER_TAG_OID ) {
				rc = LDAP_DECODING_ERROR;
				goto nomem;
			}

			oid2.bv_val = oidptr;
			oid2.bv_len = oidrem;
			if ( ber_decode_oid( &Oid, &oid2 ) < 0 ) {
				rc = LDAP_DECODING_ERROR;
				goto nomem;
			}
			oidname = find_oid( &oid2 );
			if ( !oidname ) {
				newAVA->la_attr = oid2;
				oidptr += oid2.bv_len + 1;
				oidrem -= oid2.bv_len + 1;

				/* Running out of OID buffer space? */
				if (oidrem < 128) {
					if ( oidsize == 0 ) {
						oidsize = sizeof(oids) * 2;
						oidrem = oidsize;
						oidbuf = LDAP_MALLOC( oidsize );
						if ( oidbuf == NULL ) goto nomem;
						oidptr = oidbuf;
					} else {
						char *old = oidbuf;
						oidbuf = LDAP_REALLOC( oidbuf, oidsize*2 );
						if ( oidbuf == NULL ) goto nomem;
						/* Buffer moved! Fix AVA pointers */
						if ( old != oidbuf ) {
							LDAPAVA *a;
							long dif = oidbuf - old;

							for (a=baseAVA; a<=newAVA; a++){
								if (a->la_attr.bv_val >= old &&
									a->la_attr.bv_val <= (old + oidsize))
									a->la_attr.bv_val += dif;
							}
						}
						oidptr = oidbuf + oidsize - oidrem;
						oidrem += oidsize;
						oidsize *= 2;
					}
				}
			} else {
				if ( func ) {
					newAVA->la_attr = oidname->oid;
				} else {
					newAVA->la_attr = oidname->name;
				}
			}
			tag = ber_get_stringbv( ber, &Val, LBER_BV_NOTERM );
			switch(tag) {
			case LBER_TAG_UNIVERSAL:
				/* This uses 32-bit ISO 10646-1 */
				csize = 4; goto to_utf8;
			case LBER_TAG_BMP:
				/* This uses 16-bit ISO 10646-1 */
				csize = 2; goto to_utf8;
			case LBER_TAG_TELETEX:
				/* This uses 8-bit, assume ISO 8859-1 */
				csize = 1;
to_utf8:		rc = ldap_ucs_to_utf8s( &Val, csize, &newAVA->la_value );
				newAVA->la_flags |= LDAP_AVA_FREE_VALUE;
				if (rc != LDAP_SUCCESS) goto nomem;
				newAVA->la_flags = LDAP_AVA_NONPRINTABLE;
				break;
			case LBER_TAG_UTF8:
				newAVA->la_flags = LDAP_AVA_NONPRINTABLE;
				/* This is already in UTF-8 encoding */
			case LBER_TAG_IA5:
			case LBER_TAG_PRINTABLE:
				/* These are always 7-bit strings */
				newAVA->la_value = Val;
			default:
				;
			}
			newAVA->la_private = NULL;
			newAVA->la_flags = LDAP_AVA_STRING;
			newAVA++;
		}
		*newRDN++ = NULL;
		tag = ber_next_element( ber, &len, dn_end );
	}
		
	if ( func ) {
		rc = func( newDN, flags, NULL );
		if ( rc != LDAP_SUCCESS )
			goto nomem;
	}

	rc = ldap_dn2bv_x( newDN, bv, LDAP_DN_FORMAT_LDAPV3, NULL );

nomem:
	for (;baseAVA < newAVA; baseAVA++) {
		if (baseAVA->la_flags & LDAP_AVA_FREE_ATTR)
			LDAP_FREE( baseAVA->la_attr.bv_val );
		if (baseAVA->la_flags & LDAP_AVA_FREE_VALUE)
			LDAP_FREE( baseAVA->la_value.bv_val );
	}

	if ( oidsize != 0 )
		LDAP_FREE( oidbuf );
	if ( newDN != (LDAPDN)(char *) ptrs )
		LDAP_FREE( newDN );
	return rc;
}

