/*	$NetBSD: tls_m.c,v 1.1.1.2 2010/12/12 15:21:39 adam Exp $	*/

/* tls_m.c - Handle tls/ssl using Mozilla NSS. */
/* OpenLDAP: pkg/ldap/libraries/libldap/tls_m.c,v 1.3.2.11 2010/04/15 21:26:00 quanah Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2010 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS: Initial version written by Howard Chu. 
 * Additional support by Rich Megginson.
 */

#include "portable.h"

#ifdef HAVE_MOZNSS

#include "ldap_config.h"

#include <stdio.h>

#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif

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
#include "ldap-tls.h"

#ifdef LDAP_R_COMPILE
#include <ldap_pvt_thread.h>
#endif

#define READ_PASSWORD_FROM_STDIN
#define READ_PASSWORD_FROM_FILE

#ifdef READ_PASSWORD_FROM_STDIN
#include <termios.h> /* for echo on/off */
#endif

#include <nspr/nspr.h>
#include <nspr/private/pprio.h>
#include <nss/nss.h>
#include <nss/ssl.h>
#include <nss/sslerr.h>
#include <nss/sslproto.h>
#include <nss/pk11pub.h>
#include <nss/secerr.h>
#include <nss/keyhi.h>
#include <nss/secmod.h>

/* NSS 3.12.5 and later have NSS_InitContext */
#if NSS_VMAJOR <= 3 && NSS_VMINOR <= 12 && NSS_VPATCH < 5
/* do nothing */
#else
#define HAVE_NSS_INITCONTEXT 1
#endif

/* InitContext does not currently work in server mode */
/* #define INITCONTEXT_HACK 1 */

typedef struct tlsm_ctx {
	PRFileDesc *tc_model;
	int tc_refcnt;
	PRBool tc_verify_cert;
	CERTCertDBHandle *tc_certdb;
	char *tc_certname;
	char *tc_pin_file;
	struct ldaptls *tc_config;
	int tc_is_server;
	int tc_require_cert;
	PRCallOnceType tc_callonce;
	PRBool tc_using_pem;
	char *tc_slotname; /* if using pem */
#ifdef HAVE_NSS_INITCONTEXT
	NSSInitContext *tc_initctx; /* the NSS context */
#endif
	PK11GenericObject **tc_pem_objs; /* array of objects to free */
	int tc_n_pem_objs; /* number of objects */
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_t tc_refmutex;
#endif
} tlsm_ctx;

typedef PRFileDesc tlsm_session;

static PRDescIdentity	tlsm_layer_id;

static const PRIOMethods tlsm_PR_methods;

#define PEM_LIBRARY	"nsspem"
#define PEM_MODULE	"PEM"

static SECMODModule *pem_module;

#define DEFAULT_TOKEN_NAME "default"
/* sprintf format used to create token name */
#define TLSM_PEM_TOKEN_FMT "PEM Token #%ld"

static int tlsm_slot_count;

#define PK11_SETATTRS(x,id,v,l) (x)->type = (id); \
                (x)->pValue=(v); (x)->ulValueLen = (l);

/* forward declaration */
static int tlsm_init( void );

#ifdef LDAP_R_COMPILE

static void
tlsm_thr_init( void )
{
}

#endif /* LDAP_R_COMPILE */

static const char *
tlsm_dump_cipher_info(PRFileDesc *fd)
{
	PRUint16 ii;

	for (ii = 0; ii < SSL_NumImplementedCiphers; ++ii) {
		PRInt32 cipher = (PRInt32)SSL_ImplementedCiphers[ii];
		PRBool enabled = PR_FALSE;
		PRInt32 policy = 0;
		SSLCipherSuiteInfo info;

		if (fd) {
			SSL_CipherPrefGet(fd, cipher, &enabled);
		} else {
			SSL_CipherPrefGetDefault(cipher, &enabled);
		}
		SSL_CipherPolicyGet(cipher, &policy);
		SSL_GetCipherSuiteInfo(cipher, &info, (PRUintn)sizeof(info));
		Debug( LDAP_DEBUG_TRACE,
			   "TLS: cipher: %d - %s, enabled: %d, ",
			   info.cipherSuite, info.cipherSuiteName, enabled );
		Debug( LDAP_DEBUG_TRACE,
			   "policy: %d\n", policy, 0, 0 );
	}

	return "";
}

/* Cipher definitions */
typedef struct {
	char *ossl_name;    /* The OpenSSL cipher name */
	int num;            /* The cipher id */
	int attr;           /* cipher attributes: algorithms, etc */
	int version;        /* protocol version valid for this cipher */
	int bits;           /* bits of strength */
	int alg_bits;       /* bits of the algorithm */
	int strength;       /* LOW, MEDIUM, HIGH */
	int enabled;        /* Enabled by default? */
} cipher_properties;

/* cipher attributes  */
#define SSL_kRSA  0x00000001L
#define SSL_aRSA  0x00000002L
#define SSL_aDSS  0x00000004L
#define SSL_DSS   SSL_aDSS
#define SSL_eNULL 0x00000008L
#define SSL_DES   0x00000010L
#define SSL_3DES  0x00000020L
#define SSL_RC4   0x00000040L
#define SSL_RC2   0x00000080L
#define SSL_AES   0x00000100L
#define SSL_MD5   0x00000200L
#define SSL_SHA1  0x00000400L
#define SSL_SHA   SSL_SHA1
#define SSL_RSA   (SSL_kRSA|SSL_aRSA)

/* cipher strength */
#define SSL_NULL      0x00000001L
#define SSL_EXPORT40  0x00000002L
#define SSL_EXPORT56  0x00000004L
#define SSL_LOW       0x00000008L
#define SSL_MEDIUM    0x00000010L
#define SSL_HIGH      0x00000020L

#define SSL2  0x00000001L
#define SSL3  0x00000002L
/* OpenSSL treats SSL3 and TLSv1 the same */
#define TLS1  SSL3

/* Cipher translation */
static cipher_properties ciphers_def[] = {
	/* SSL 2 ciphers */
	{"DES-CBC3-MD5", SSL_EN_DES_192_EDE3_CBC_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_3DES|SSL_MD5, SSL2, 168, 168, SSL_HIGH, SSL_ALLOWED},
	{"RC2-CBC-MD5", SSL_EN_RC2_128_CBC_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC2|SSL_MD5, SSL2, 128, 128, SSL_MEDIUM, SSL_ALLOWED},
	{"RC4-MD5", SSL_EN_RC4_128_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL2, 128, 128, SSL_MEDIUM, SSL_ALLOWED},
	{"DES-CBC-MD5", SSL_EN_DES_64_CBC_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_DES|SSL_MD5, SSL2, 56, 56, SSL_LOW, SSL_ALLOWED},
	{"EXP-RC2-CBC-MD5", SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC2|SSL_MD5, SSL2, 40, 128, SSL_EXPORT40, SSL_ALLOWED},
	{"EXP-RC4-MD5", SSL_EN_RC4_128_EXPORT40_WITH_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL2, 40, 128, SSL_EXPORT40, SSL_ALLOWED},

	/* SSL3 ciphers */
	{"RC4-MD5", SSL_RSA_WITH_RC4_128_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL3, 128, 128, SSL_MEDIUM, SSL_ALLOWED},
	{"RC4-SHA", SSL_RSA_WITH_RC4_128_SHA, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_SHA1, SSL3, 128, 128, SSL_MEDIUM, SSL_NOT_ALLOWED},
	{"DES-CBC3-SHA", SSL_RSA_WITH_3DES_EDE_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_3DES|SSL_SHA1, SSL3, 168, 168, SSL_HIGH, SSL_ALLOWED},
	{"DES-CBC-SHA", SSL_RSA_WITH_DES_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_DES|SSL_SHA1, SSL3, 56, 56, SSL_LOW, SSL_ALLOWED},
	{"EXP-RC4-MD5", SSL_RSA_EXPORT_WITH_RC4_40_MD5, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_MD5, SSL3, 40, 128, SSL_EXPORT40, SSL_ALLOWED},
	{"EXP-RC2-CBC-MD5", SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5, SSL_kRSA|SSL_aRSA|SSL_RC2|SSL_MD5, SSL3, 0, 0, SSL_EXPORT40, SSL_ALLOWED},
	{"NULL-MD5", SSL_RSA_WITH_NULL_MD5, SSL_kRSA|SSL_aRSA|SSL_eNULL|SSL_MD5, SSL3, 0, 0, SSL_NULL, SSL_NOT_ALLOWED},
	{"NULL-SHA", SSL_RSA_WITH_NULL_SHA, SSL_kRSA|SSL_aRSA|SSL_eNULL|SSL_SHA1, SSL3, 0, 0, SSL_NULL, SSL_NOT_ALLOWED},

	/* TLSv1 ciphers */
	{"EXP1024-DES-CBC-SHA", TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_DES|SSL_SHA, TLS1, 56, 56, SSL_EXPORT56, SSL_ALLOWED},
	{"EXP1024-RC4-SHA", TLS_RSA_EXPORT1024_WITH_RC4_56_SHA, SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_SHA, TLS1, 56, 56, SSL_EXPORT56, SSL_ALLOWED},
	{"AES128-SHA", TLS_RSA_WITH_AES_128_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_AES|SSL_SHA, TLS1, 128, 128, SSL_HIGH, SSL_NOT_ALLOWED},
	{"AES256-SHA", TLS_RSA_WITH_AES_256_CBC_SHA, SSL_kRSA|SSL_aRSA|SSL_AES|SSL_SHA, TLS1, 256, 256, SSL_HIGH, SSL_NOT_ALLOWED},
};

#define ciphernum (sizeof(ciphers_def)/sizeof(cipher_properties))

/* given err which is the current errno, calls PR_SetError with
   the corresponding NSPR error code */
static void 
tlsm_map_error(int err)
{
	PRErrorCode prError;

	switch ( err ) {
	case EACCES:
		prError = PR_NO_ACCESS_RIGHTS_ERROR;
		break;
	case EADDRINUSE:
		prError = PR_ADDRESS_IN_USE_ERROR;
		break;
	case EADDRNOTAVAIL:
		prError = PR_ADDRESS_NOT_AVAILABLE_ERROR;
		break;
	case EAFNOSUPPORT:
		prError = PR_ADDRESS_NOT_SUPPORTED_ERROR;
		break;
	case EAGAIN:
		prError = PR_WOULD_BLOCK_ERROR;
		break;
	/*
	 * On QNX and Neutrino, EALREADY is defined as EBUSY.
	 */
#if EALREADY != EBUSY
	case EALREADY:
		prError = PR_ALREADY_INITIATED_ERROR;
		break;
#endif
	case EBADF:
		prError = PR_BAD_DESCRIPTOR_ERROR;
		break;
#ifdef EBADMSG
	case EBADMSG:
		prError = PR_IO_ERROR;
		break;
#endif
	case EBUSY:
		prError = PR_FILESYSTEM_MOUNTED_ERROR;
		break;
	case ECONNABORTED:
		prError = PR_CONNECT_ABORTED_ERROR;
		break;
	case ECONNREFUSED:
		prError = PR_CONNECT_REFUSED_ERROR;
		break;
	case ECONNRESET:
		prError = PR_CONNECT_RESET_ERROR;
		break;
	case EDEADLK:
		prError = PR_DEADLOCK_ERROR;
		break;
#ifdef EDIRCORRUPTED
	case EDIRCORRUPTED:
		prError = PR_DIRECTORY_CORRUPTED_ERROR;
		break;
#endif
#ifdef EDQUOT
	case EDQUOT:
		prError = PR_NO_DEVICE_SPACE_ERROR;
		break;
#endif
	case EEXIST:
		prError = PR_FILE_EXISTS_ERROR;
		break;
	case EFAULT:
		prError = PR_ACCESS_FAULT_ERROR;
		break;
	case EFBIG:
		prError = PR_FILE_TOO_BIG_ERROR;
		break;
	case EHOSTUNREACH:
		prError = PR_HOST_UNREACHABLE_ERROR;
		break;
	case EINPROGRESS:
		prError = PR_IN_PROGRESS_ERROR;
		break;
	case EINTR:
		prError = PR_PENDING_INTERRUPT_ERROR;
		break;
	case EINVAL:
		prError = PR_INVALID_ARGUMENT_ERROR;
		break;
	case EIO:
		prError = PR_IO_ERROR;
		break;
	case EISCONN:
		prError = PR_IS_CONNECTED_ERROR;
		break;
	case EISDIR:
		prError = PR_IS_DIRECTORY_ERROR;
		break;
	case ELOOP:
		prError = PR_LOOP_ERROR;
		break;
	case EMFILE:
		prError = PR_PROC_DESC_TABLE_FULL_ERROR;
		break;
	case EMLINK:
		prError = PR_MAX_DIRECTORY_ENTRIES_ERROR;
		break;
	case EMSGSIZE:
		prError = PR_INVALID_ARGUMENT_ERROR;
		break;
#ifdef EMULTIHOP
	case EMULTIHOP:
		prError = PR_REMOTE_FILE_ERROR;
		break;
#endif
	case ENAMETOOLONG:
		prError = PR_NAME_TOO_LONG_ERROR;
		break;
	case ENETUNREACH:
		prError = PR_NETWORK_UNREACHABLE_ERROR;
		break;
	case ENFILE:
		prError = PR_SYS_DESC_TABLE_FULL_ERROR;
		break;
	/*
	 * On SCO OpenServer 5, ENOBUFS is defined as ENOSR.
	 */
#if defined(ENOBUFS) && (ENOBUFS != ENOSR)
	case ENOBUFS:
		prError = PR_INSUFFICIENT_RESOURCES_ERROR;
		break;
#endif
	case ENODEV:
		prError = PR_FILE_NOT_FOUND_ERROR;
		break;
	case ENOENT:
		prError = PR_FILE_NOT_FOUND_ERROR;
		break;
	case ENOLCK:
		prError = PR_FILE_IS_LOCKED_ERROR;
		break;
#ifdef ENOLINK 
	case ENOLINK:
		prError = PR_REMOTE_FILE_ERROR;
		break;
#endif
	case ENOMEM:
		prError = PR_OUT_OF_MEMORY_ERROR;
		break;
	case ENOPROTOOPT:
		prError = PR_INVALID_ARGUMENT_ERROR;
		break;
	case ENOSPC:
		prError = PR_NO_DEVICE_SPACE_ERROR;
		break;
#ifdef ENOSR
	case ENOSR:
		prError = PR_INSUFFICIENT_RESOURCES_ERROR;
		break;
#endif
	case ENOTCONN:
		prError = PR_NOT_CONNECTED_ERROR;
		break;
	case ENOTDIR:
		prError = PR_NOT_DIRECTORY_ERROR;
		break;
	case ENOTSOCK:
		prError = PR_NOT_SOCKET_ERROR;
		break;
	case ENXIO:
		prError = PR_FILE_NOT_FOUND_ERROR;
		break;
	case EOPNOTSUPP:
		prError = PR_NOT_TCP_SOCKET_ERROR;
		break;
#ifdef EOVERFLOW
	case EOVERFLOW:
		prError = PR_BUFFER_OVERFLOW_ERROR;
		break;
#endif
	case EPERM:
		prError = PR_NO_ACCESS_RIGHTS_ERROR;
		break;
	case EPIPE:
		prError = PR_CONNECT_RESET_ERROR;
		break;
#ifdef EPROTO
	case EPROTO:
		prError = PR_IO_ERROR;
		break;
#endif
	case EPROTONOSUPPORT:
		prError = PR_PROTOCOL_NOT_SUPPORTED_ERROR;
		break;
	case EPROTOTYPE:
		prError = PR_ADDRESS_NOT_SUPPORTED_ERROR;
		break;
	case ERANGE:
		prError = PR_INVALID_METHOD_ERROR;
		break;
	case EROFS:
		prError = PR_READ_ONLY_FILESYSTEM_ERROR;
		break;
	case ESPIPE:
		prError = PR_INVALID_METHOD_ERROR;
		break;
	case ETIMEDOUT:
		prError = PR_IO_TIMEOUT_ERROR;
		break;
#if EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK:
		prError = PR_WOULD_BLOCK_ERROR;
		break;
#endif
	case EXDEV:
		prError = PR_NOT_SAME_DEVICE_ERROR;
		break;
	default:
		prError = PR_UNKNOWN_ERROR;
		break;
	}
	PR_SetError( prError, err );
}

/*
 * cipher_list is an integer array with the following values:
 *   -1: never enable this cipher
 *    0: cipher disabled
 *    1: cipher enabled
 */
static int
nss_parse_ciphers(const char *cipherstr, int cipher_list[ciphernum])
{
	int i;
	char *cipher;
	char *ciphers;
	char *ciphertip;
	int action;
	int rv;

	/* All disabled to start */
	for (i=0; i<ciphernum; i++)
		cipher_list[i] = 0;

	ciphertip = strdup(cipherstr);
	cipher = ciphers = ciphertip;

	while (ciphers && (strlen(ciphers))) {
		while ((*cipher) && (isspace(*cipher)))
			++cipher;

		action = 1;
		switch(*cipher) {
		case '+': /* Add something */
			action = 1;
			cipher++;
			break;
		case '-': /* Subtract something */
			action = 0;
			cipher++;
			break;
		case '!':  /* Disable something */
			action = -1;
			cipher++;
			break;
		default:
			/* do nothing */
			break;
		}

		if ((ciphers = strchr(cipher, ':'))) {
			*ciphers++ = '\0';
		}

		/* Do the easy one first */
		if (!strcmp(cipher, "ALL")) {
			for (i=0; i<ciphernum; i++) {
				if (!(ciphers_def[i].attr & SSL_eNULL))
					cipher_list[i] = action;
			}
		} else if (!strcmp(cipher, "COMPLEMENTOFALL")) {
			for (i=0; i<ciphernum; i++) {
				if ((ciphers_def[i].attr & SSL_eNULL))
					cipher_list[i] = action;
			}
		} else if (!strcmp(cipher, "DEFAULT")) {
			for (i=0; i<ciphernum; i++) {
				cipher_list[i] = ciphers_def[i].enabled == SSL_ALLOWED ? 1 : 0;
			}
		} else {
			int mask = 0;
			int strength = 0;
			int protocol = 0;
			char *c;

			c = cipher;
			while (c && (strlen(c))) {

				if ((c = strchr(cipher, '+'))) {
					*c++ = '\0';
				}

				if (!strcmp(cipher, "RSA")) {
					mask |= SSL_RSA;
				} else if ((!strcmp(cipher, "NULL")) || (!strcmp(cipher, "eNULL"))) {
					mask |= SSL_eNULL;
				} else if (!strcmp(cipher, "AES")) {
					mask |= SSL_AES;
				} else if (!strcmp(cipher, "3DES")) {
					mask |= SSL_3DES;
				} else if (!strcmp(cipher, "DES")) {
					mask |= SSL_DES;
				} else if (!strcmp(cipher, "RC4")) {
					mask |= SSL_RC4;
				} else if (!strcmp(cipher, "RC2")) {
					mask |= SSL_RC2;
				} else if (!strcmp(cipher, "MD5")) {
					mask |= SSL_MD5;
				} else if ((!strcmp(cipher, "SHA")) || (!strcmp(cipher, "SHA1"))) {
					mask |= SSL_SHA1;
				} else if (!strcmp(cipher, "SSLv2")) {
					protocol |= SSL2;
				} else if (!strcmp(cipher, "SSLv3")) {
					protocol |= SSL3;
				} else if (!strcmp(cipher, "TLSv1")) {
					protocol |= TLS1;
				} else if (!strcmp(cipher, "HIGH")) {
					strength |= SSL_HIGH;
				} else if (!strcmp(cipher, "MEDIUM")) {
					strength |= SSL_MEDIUM;
				} else if (!strcmp(cipher, "LOW")) {
					strength |= SSL_LOW;
				} else if ((!strcmp(cipher, "EXPORT")) || (!strcmp(cipher, "EXP"))) {
					strength |= SSL_EXPORT40|SSL_EXPORT56;
				} else if (!strcmp(cipher, "EXPORT40")) {
					strength |= SSL_EXPORT40;
				} else if (!strcmp(cipher, "EXPORT56")) {
					strength |= SSL_EXPORT56;
				}

				if (c)
					cipher = c;

			} /* while */

			/* If we have a mask, apply it. If not then perhaps they provided
			 * a specific cipher to enable.
			 */
			if (mask || strength || protocol) {
				for (i=0; i<ciphernum; i++) {
					if (((ciphers_def[i].attr & mask) ||
						 (ciphers_def[i].strength & strength) ||
						 (ciphers_def[i].version & protocol)) &&
						(cipher_list[i] != -1)) {
						/* Enable the NULL ciphers only if explicity
						 * requested */
						if (ciphers_def[i].attr & SSL_eNULL) {
							if (mask & SSL_eNULL)
								cipher_list[i] = action;
						} else
							cipher_list[i] = action;
					}
				}
			} else {
				for (i=0; i<ciphernum; i++) {
					if (!strcmp(ciphers_def[i].ossl_name, cipher) &&
						cipher_list[1] != -1)
						cipher_list[i] = action;
				}
			}
		}

		if (ciphers)
			cipher = ciphers;
	}

	/* See if any ciphers were enabled */
	rv = 0;
	for (i=0; i<ciphernum; i++) {
		if (cipher_list[i] == 1)
			rv = 1;
	}

	free(ciphertip);

	return rv;
}

static int
tlsm_parse_ciphers(tlsm_ctx *ctx, const char *str)
{
	int cipher_state[ciphernum];
	int rv, i;

	if (!ctx)
		return 0;

	rv = nss_parse_ciphers(str, cipher_state);

	if (rv) {
		/* First disable everything */
		for (i = 0; i < SSL_NumImplementedCiphers; i++)
			SSL_CipherPrefSet(ctx->tc_model, SSL_ImplementedCiphers[i], SSL_NOT_ALLOWED);

		/* Now enable what was requested */
		for (i=0; i<ciphernum; i++) {
			SSLCipherSuiteInfo suite;
			PRBool enabled;

			if (SSL_GetCipherSuiteInfo(ciphers_def[i].num, &suite, sizeof suite)
				== SECSuccess) {
				enabled = cipher_state[i] < 0 ? 0 : cipher_state[i];
				if (enabled == SSL_ALLOWED) {
					if (PK11_IsFIPS() && !suite.isFIPS)    
						enabled = SSL_NOT_ALLOWED;
				}
				SSL_CipherPrefSet(ctx->tc_model, ciphers_def[i].num, enabled);
			}
		}
	}

	return rv == 1 ? 0 : -1;
}

static SECStatus
tlsm_bad_cert_handler(void *arg, PRFileDesc *ssl)
{
	SECStatus success = SECSuccess;
	PRErrorCode err;
	tlsm_ctx *ctx = (tlsm_ctx *)arg;

	if (!ssl || !ctx) {
		return SECFailure;
	}

	err = PORT_GetError();

	switch (err) {
	case SEC_ERROR_UNTRUSTED_ISSUER:
	case SEC_ERROR_UNKNOWN_ISSUER:
	case SEC_ERROR_EXPIRED_CERTIFICATE:
		if (ctx->tc_verify_cert) {
			success = SECFailure;
		}
		break;
	/* we bypass NSS's hostname checks and do our own */
	case SSL_ERROR_BAD_CERT_DOMAIN:
		break;
	default:
		success = SECFailure;
		break;
	}

	return success;
}

static const char *
tlsm_dump_security_status(PRFileDesc *fd)
{
	char * cp;	/* bulk cipher name */
	char * ip;	/* cert issuer DN */
	char * sp;	/* cert subject DN */
	int    op;	/* High, Low, Off */
	int    kp0;	/* total key bits */
	int    kp1;	/* secret key bits */
	SSL3Statistics * ssl3stats = SSL_GetStatistics();

	SSL_SecurityStatus( fd, &op, &cp, &kp0, &kp1, &ip, &sp );
	Debug( LDAP_DEBUG_TRACE,
		   "TLS certificate verification: subject: %s, issuer: %s, cipher: %s, ",
		   sp ? sp : "-unknown-", ip ? ip : "-unknown-", cp ? cp : "-unknown-" );
	PR_Free(cp);
	PR_Free(ip);
	PR_Free(sp);
	Debug( LDAP_DEBUG_TRACE,
		   "security level: %s, secret key bits: %d, total key bits: %d, ",
		   ((op == SSL_SECURITY_STATUS_ON_HIGH) ? "high" :
			((op == SSL_SECURITY_STATUS_ON_LOW) ? "low" : "off")),
		   kp1, kp0 );

	Debug( LDAP_DEBUG_TRACE,
		   "cache hits: %ld, cache misses: %ld, cache not reusable: %ld\n",
		   ssl3stats->hch_sid_cache_hits, ssl3stats->hch_sid_cache_misses,
		   ssl3stats->hch_sid_cache_not_ok );

	return "";
}

#ifdef READ_PASSWORD_FROM_FILE
static char *
tlsm_get_pin_from_file(const char *token_name, tlsm_ctx *ctx)
{
	char *pwdstr = NULL;
	char *contents = NULL;
	char *lasts = NULL;
	char *line = NULL;
	char *candidate = NULL;
	PRFileInfo file_info;
	PRFileDesc *pwd_fileptr = PR_Open( ctx->tc_pin_file, PR_RDONLY, 00400 );

	/* open the password file */
	if ( !pwd_fileptr ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not open security pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* get the file size */
	if ( PR_SUCCESS != PR_GetFileInfo( ctx->tc_pin_file, &file_info ) ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not get file info from pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* create a buffer to hold the file contents */
	if ( !( contents = PR_MALLOC( file_info.size + 1 ) ) ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not alloc a buffer for contents of pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* read file into the buffer */
	if( PR_Read( pwd_fileptr, contents, file_info.size ) <= 0 ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not read the file contents from pin file %s - error %d:%s.\n",
		       ctx->tc_pin_file, errcode,
		       PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	/* format is [tokenname:]password EOL [tokenname:]password EOL ... */
	/* if you want to use a password containing a colon character, use
	   the special tokenname "default" */
	for ( line = PL_strtok_r( contents, "\r\n", &lasts ); line;
	      line = PL_strtok_r( NULL, "\r\n", &lasts ) ) {
		char *colon;

		if ( !*line ) {
			continue; /* skip blank lines */
		}
		colon = PL_strchr( line, ':' );
		if ( colon ) {
			if ( *(colon + 1) && token_name &&
			     !PL_strncmp( token_name, line, colon-line ) ) {
				candidate = colon + 1; /* found a definite match */
				break;
			} else if ( !PL_strncmp( DEFAULT_TOKEN_NAME, line, colon-line ) ) {
				candidate = colon + 1; /* found possible match */
			}
		} else { /* no token name */
			candidate = line;
		}
	}
done:
	if ( pwd_fileptr ) {
		PR_Close( pwd_fileptr );
	}
	if ( candidate ) {
		pwdstr = PL_strdup( candidate );
	}
	PL_strfree( contents );

	return pwdstr;
}
#endif /* READ_PASSWORD_FROM_FILE */

#ifdef READ_PASSWORD_FROM_STDIN
/*
 * Turn the echoing off on a tty.
 */
static void
echoOff(int fd)
{
	if ( isatty( fd ) ) {
		struct termios tio;
		tcgetattr( fd, &tio );
		tio.c_lflag &= ~ECHO;
		tcsetattr( fd, TCSAFLUSH, &tio );
	}
}

/*
 * Turn the echoing on on a tty.
 */
static void
echoOn(int fd)
{
	if ( isatty( fd ) ) {
		struct termios tio;
		tcgetattr( fd, &tio );
		tio.c_lflag |= ECHO;
		tcsetattr( fd, TCSAFLUSH, &tio );
		tcsetattr( fd, TCSAFLUSH, &tio );
	}
}
#endif /* READ_PASSWORD_FROM_STDIN */

/*
 * This does the actual work of reading the pin/password/pass phrase
 */
static char *
tlsm_get_pin(PK11SlotInfo *slot, PRBool retry, tlsm_ctx *ctx)
{
	char *token_name = NULL;
	char *pwdstr = NULL;

	token_name = PK11_GetTokenName( slot );
#ifdef READ_PASSWORD_FROM_FILE
	/* Try to get the passwords from the password file if it exists.
	 * THIS IS UNSAFE and is provided for convenience only. Without this
	 * capability the server would have to be started in foreground mode
	 * if using an encrypted key.
	 */
	if ( ctx->tc_pin_file ) {
		pwdstr = tlsm_get_pin_from_file( token_name, ctx );
	}
#endif /* RETRIEVE_PASSWORD_FROM_FILE */
#ifdef READ_PASSWORD_FROM_STDIN
	if ( !pwdstr ) {
		int infd = PR_FileDesc2NativeHandle( PR_STDIN );
		int isTTY = isatty( infd );
		unsigned char phrase[200];
		/* Prompt for password */
		if ( isTTY ) {
			fprintf( stdout,
				 "Please enter pin, password, or pass phrase for security token '%s': ",
				 token_name ? token_name : DEFAULT_TOKEN_NAME );
			echoOff( infd );
		}
		fgets( (char*)phrase, sizeof(phrase), stdin );
		if ( isTTY ) {
			fprintf( stdout, "\n" );
			echoOn( infd );
		}
		/* stomp on newline */
		phrase[strlen((char*)phrase)-1] = 0;

		pwdstr = PL_strdup( (char*)phrase );
	}

#endif /* READ_PASSWORD_FROM_STDIN */
	return pwdstr;
}

/*
 * PKCS11 devices (including the internal softokn cert/key database)
 * may be protected by a pin or password or even pass phrase
 * MozNSS needs a way for the user to provide that
 */
static char *
tlsm_pin_prompt(PK11SlotInfo *slot, PRBool retry, void *arg)
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;

	return tlsm_get_pin( slot, retry, ctx );
}

static SECStatus
tlsm_auth_cert_handler(void *arg, PRFileDesc *fd,
                       PRBool checksig, PRBool isServer)
{
	SECStatus ret = SSL_AuthCertificate(arg, fd, checksig, isServer);

	tlsm_dump_security_status( fd );
	Debug( LDAP_DEBUG_TRACE,
		   "TLS certificate verification: %s\n",
		   ret == SECSuccess ? "ok" : "bad", 0, 0 );

	if ( ret != SECSuccess ) {
		PRErrorCode errcode = PORT_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS certificate verification: Error, %d: %s\n",
			   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 ) ;
	}

	return ret;
}

static int
tlsm_authenticate_to_slot( tlsm_ctx *ctx, PK11SlotInfo *slot )
{
	int rc = -1;

	if ( SECSuccess != PK11_Authenticate( slot, PR_FALSE, ctx ) ) {
		char *token_name = PK11_GetTokenName( slot );
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not authenticate to the security token %s - error %d:%s.\n",
			   token_name ? token_name : DEFAULT_TOKEN_NAME, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
	} else {
		rc = 0; /* success */
	}

	return rc;
}

static int
tlsm_init_tokens( tlsm_ctx *ctx )
{
	PK11SlotList *slotList;
	PK11SlotListElement *listEntry;
	int rc = 0;

	slotList = PK11_GetAllTokens( CKM_INVALID_MECHANISM, PR_FALSE, PR_TRUE, NULL );

	for ( listEntry = PK11_GetFirstSafe( slotList ); !rc && listEntry;
		  listEntry = PK11_GetNextSafe( slotList, listEntry, PR_FALSE ) ) {
		PK11SlotInfo *slot = listEntry->slot;
		rc = tlsm_authenticate_to_slot( ctx, slot );
	}

	PK11_FreeSlotList( slotList );

	return rc;
}

static SECStatus
tlsm_nss_shutdown_cb( void *appData, void *nssData )
{
	SECStatus rc = SECSuccess;

	SSL_ShutdownServerSessionIDCache();
	SSL_ClearSessionCache();

	if ( pem_module ) {
		SECMOD_UnloadUserModule( pem_module );
		SECMOD_DestroyModule( pem_module );
		pem_module = NULL;
	}
	return rc;
}

static int
tlsm_init_pem_module( void )
{
	int rc = 0;
	char *fullname = NULL;
	char *configstring = NULL;

	if ( pem_module ) {
		return rc;
	}

	/* not loaded - load it */
	/* get the system dependent library name */
	fullname = PR_GetLibraryName( NULL, PEM_LIBRARY );
	/* Load our PKCS#11 module */
	configstring = PR_smprintf( "library=%s name=" PEM_MODULE " parameters=\"\"", fullname );
	PL_strfree( fullname );

	pem_module = SECMOD_LoadUserModule( configstring, NULL, PR_FALSE );
	PR_smprintf_free( configstring );

	if ( !pem_module || !pem_module->loaded ) {
		if ( pem_module ) {
			SECMOD_DestroyModule( pem_module );
			pem_module = NULL;
		}
		rc = -1;
	}

	return rc;
}

static void
tlsm_add_pem_obj( tlsm_ctx *ctx, PK11GenericObject *obj )
{
	int idx = ctx->tc_n_pem_objs;
	ctx->tc_n_pem_objs++;
	ctx->tc_pem_objs = (PK11GenericObject **)
		PORT_Realloc( ctx->tc_pem_objs, ctx->tc_n_pem_objs * sizeof( PK11GenericObject * ) );
	ctx->tc_pem_objs[idx] = obj;														  
}

static void
tlsm_free_pem_objs( tlsm_ctx *ctx )
{
	/* free in reverse order of allocation */
	while ( ctx->tc_n_pem_objs-- ) {
		PK11_DestroyGenericObject( ctx->tc_pem_objs[ctx->tc_n_pem_objs] );
		ctx->tc_pem_objs[ctx->tc_n_pem_objs] = NULL;
	}
	PORT_Free(ctx->tc_pem_objs);
	ctx->tc_pem_objs = NULL;
	ctx->tc_n_pem_objs = 0;
}

static int
tlsm_add_cert_from_file( tlsm_ctx *ctx, const char *filename, PRBool isca )
{
	CK_SLOT_ID slotID;
	PK11SlotInfo *slot = NULL;
	PK11GenericObject *rv;
	CK_ATTRIBUTE *attrs;
	CK_ATTRIBUTE theTemplate[20];
	CK_BBOOL cktrue = CK_TRUE;
	CK_BBOOL ckfalse = CK_FALSE;
	CK_OBJECT_CLASS objClass = CKO_CERTIFICATE;
	char tmpslotname[64];
	char *slotname = NULL;
	const char *ptr = NULL;
	char sep = PR_GetDirectorySeparator();

	attrs = theTemplate;

	if ( isca ) {
		slotID = 0; /* CA and trust objects use slot 0 */
		PR_snprintf( tmpslotname, sizeof(tmpslotname), TLSM_PEM_TOKEN_FMT, slotID );
		slotname = tmpslotname;
	} else {
		if ( ctx->tc_slotname == NULL ) { /* need new slot */
			slotID = ++tlsm_slot_count;
			ctx->tc_slotname = PR_smprintf( TLSM_PEM_TOKEN_FMT, slotID );
		}
		slotname = ctx->tc_slotname;

		if ( ( ptr = PL_strrchr( filename, sep ) ) ) {
			PL_strfree( ctx->tc_certname );
			++ptr;
			ctx->tc_certname = PR_smprintf( "%s:%s", slotname, ptr );
		}
	}

	slot = PK11_FindSlotByName( slotname );

	if ( !slot ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not find the slot for certificate %s - error %d:%s.\n",
			   ctx->tc_certname, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	PK11_SETATTRS( attrs, CKA_CLASS, &objClass, sizeof(objClass) ); attrs++;
	PK11_SETATTRS( attrs, CKA_TOKEN, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	PK11_SETATTRS( attrs, CKA_LABEL, (unsigned char *)filename, strlen(filename)+1 ); attrs++;
	if ( isca ) {
		PK11_SETATTRS( attrs, CKA_TRUST, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	} else {
		PK11_SETATTRS( attrs, CKA_TRUST, &ckfalse, sizeof(CK_BBOOL) ); attrs++;
	}
	/* This loads the certificate in our PEM module into the appropriate
	 * slot.
	 */
	rv = PK11_CreateGenericObject( slot, theTemplate, 4, PR_FALSE /* isPerm */ );

	PK11_FreeSlot( slot );

	if ( !rv ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not add the certificate %s - error %d:%s.\n",
			   ctx->tc_certname, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	tlsm_add_pem_obj( ctx, rv );

	return 0;
}

static int
tlsm_add_key_from_file( tlsm_ctx *ctx, const char *filename )
{
	CK_SLOT_ID slotID;
	PK11SlotInfo * slot = NULL;
	PK11GenericObject *rv;
	CK_ATTRIBUTE *attrs;
	CK_ATTRIBUTE theTemplate[20];
	CK_BBOOL cktrue = CK_TRUE;
	CK_OBJECT_CLASS objClass = CKO_PRIVATE_KEY;
	int retcode = 0;

	attrs = theTemplate;

	if ( ctx->tc_slotname == NULL ) { /* need new slot */
		slotID = ++tlsm_slot_count;
		ctx->tc_slotname = PR_smprintf( TLSM_PEM_TOKEN_FMT, slotID );
	}
	slot = PK11_FindSlotByName( ctx->tc_slotname );

	if ( !slot ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not find the slot %s for the private key - error %d:%s.\n",
			   ctx->tc_slotname, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		return -1;
	}

	PK11_SETATTRS( attrs, CKA_CLASS, &objClass, sizeof(objClass) ); attrs++;
	PK11_SETATTRS( attrs, CKA_TOKEN, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	PK11_SETATTRS( attrs, CKA_LABEL, (unsigned char *)filename, strlen(filename)+1 ); attrs++;
	rv = PK11_CreateGenericObject( slot, theTemplate, 3, PR_FALSE /* isPerm */ );

	if ( !rv ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not add the certificate %s - error %d:%s.\n",
			   ctx->tc_certname, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		retcode = -1;
	} else {
		/* When adding an encrypted key the PKCS#11 will be set as removed */
		/* This will force the token to be seen as re-inserted */
		SECMOD_WaitForAnyTokenEvent( pem_module, 0, 0 );
		PK11_IsPresent( slot );
		retcode = 0;
	}

	PK11_FreeSlot( slot );

	if ( !retcode ) {
		tlsm_add_pem_obj( ctx, rv );
	}
	return retcode;
}

static int
tlsm_init_ca_certs( tlsm_ctx *ctx, const char *cacertfile, const char *cacertdir )
{
	PRBool isca = PR_TRUE;

	if ( cacertfile ) {
		int rc = tlsm_add_cert_from_file( ctx, cacertfile, isca );
		if ( rc ) {
			return rc;
		}
	}

	if ( cacertdir ) {
		PRFileInfo fi;
		PRStatus status;
		PRDir *dir;
		PRDirEntry *entry;

		memset( &fi, 0, sizeof(fi) );
		status = PR_GetFileInfo( cacertdir, &fi );
		if ( PR_SUCCESS != status) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: could not get info about the CA certificate directory %s - error %d:%s.\n",
				   cacertdir, errcode,
				   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			return -1;
		}

		if ( fi.type != PR_FILE_DIRECTORY ) {
			Debug( LDAP_DEBUG_ANY,
				   "TLS: error: the CA certificate directory %s is not a directory.\n",
				   cacertdir, 0 ,0 );
			return -1;
		}

		dir = PR_OpenDir( cacertdir );
		if ( NULL == dir ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: could not open the CA certificate directory %s - error %d:%s.\n",
				   cacertdir, errcode,
				   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			return -1;
		}

		status = -1;
		do {
			entry = PR_ReadDir( dir, PR_SKIP_BOTH | PR_SKIP_HIDDEN );
			if ( NULL != entry ) {
				char *fullpath = PR_smprintf( "%s/%s", cacertdir, entry->name );
				if ( !tlsm_add_cert_from_file( ctx, fullpath, isca ) ) {
					status = 0; /* found at least 1 valid CA file in the dir */
				}
				PR_smprintf_free( fullpath );
			}
		} while ( NULL != entry );
		PR_CloseDir( dir );

		if ( status ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: did not find any valid CA certificate files in the CA certificate directory %s - error %d:%s.\n",
				   cacertdir, errcode,
				   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			return -1;
		}
	}

	return 0;
}

/*
 * This is the part of the init we defer until we get the
 * actual security configuration information.  This is
 * only called once, protected by a PRCallOnce
 * NOTE: This must be done before the first call to SSL_ImportFD,
 * especially the setting of the policy
 * NOTE: This must be called after fork()
 */
static int
tlsm_deferred_init( void *arg )
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;
	struct ldaptls *lt = ctx->tc_config;
	const char *securitydirs[3];
	int ii;
	int nn;
	PRErrorCode errcode = 1;
#ifdef HAVE_NSS_INITCONTEXT
	NSSInitParameters initParams;
	NSSInitContext *initctx = NULL;
#endif
	SECStatus rc;

#ifdef HAVE_NSS_INITCONTEXT
	memset( &initParams, 0, sizeof( initParams ) );
	initParams.length = sizeof( initParams );
#endif /* HAVE_NSS_INITCONTEXT */

#ifndef HAVE_NSS_INITCONTEXT
	if ( !NSS_IsInitialized() ) {
#endif /* HAVE_NSS_INITCONTEXT */
		/*
		  MOZNSS_DIR will override everything else - you can
		  always set MOZNSS_DIR to force the use of this
		  directory
		  If using MOZNSS, specify the location of the moznss db dir
		  in the cacertdir directive of the OpenLDAP configuration.
		  DEFAULT_MOZNSS_DIR will only be used if the code cannot
		  find a security dir to use based on the current
		  settings
		*/
		nn = 0;
		securitydirs[nn++] = PR_GetEnv( "MOZNSS_DIR" );
		securitydirs[nn++] = lt->lt_cacertdir;
		securitydirs[nn++] = PR_GetEnv( "DEFAULT_MOZNSS_DIR" );
		for ( ii = 0; ii < nn; ++ii ) {
			const char *securitydir = securitydirs[ii];
			if ( NULL == securitydir ) {
				continue;
			}
#ifdef HAVE_NSS_INITCONTEXT
#ifdef INITCONTEXT_HACK
			if ( !NSS_IsInitialized() && ctx->tc_is_server ) {
				rc = NSS_Initialize( securitydir, "", "", SECMOD_DB, NSS_INIT_READONLY );
			} else {
				initctx = NSS_InitContext( securitydir, "", "", SECMOD_DB,
										   &initParams, NSS_INIT_READONLY );
				rc = (initctx == NULL) ? SECFailure : SECSuccess;
			}
#else
			initctx = NSS_InitContext( securitydir, "", "", SECMOD_DB,
									   &initParams, NSS_INIT_READONLY );
			rc = (initctx == NULL) ? SECFailure : SECSuccess;
#endif
#else
			rc = NSS_Initialize( securitydir, "", "", SECMOD_DB, NSS_INIT_READONLY );
#endif

			if ( rc != SECSuccess ) {
				errcode = PORT_GetError();
				Debug( LDAP_DEBUG_TRACE,
					   "TLS: could not initialize moznss using security dir %s - error %d:%s.\n",
					   securitydir, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			} else {
				/* success */
				Debug( LDAP_DEBUG_TRACE, "TLS: using moznss security dir %s.\n",
					   securitydir, 0, 0 );
				errcode = 0;
				break;
			}
		}

		if ( errcode ) { /* no moznss db found, or not using moznss db */
#ifdef HAVE_NSS_INITCONTEXT
			int flags = NSS_INIT_READONLY|NSS_INIT_NOCERTDB|NSS_INIT_NOMODDB;
#ifdef INITCONTEXT_HACK
			if ( !NSS_IsInitialized() && ctx->tc_is_server ) {
				rc = NSS_NoDB_Init( NULL );
			} else {
				initctx = NSS_InitContext( "", "", "", SECMOD_DB,
										   &initParams, flags );
				rc = (initctx == NULL) ? SECFailure : SECSuccess;
			}
#else
			initctx = NSS_InitContext( "", "", "", SECMOD_DB,
									   &initParams, flags );
			rc = (initctx == NULL) ? SECFailure : SECSuccess;
#endif
#else
			rc = NSS_NoDB_Init( NULL );
#endif
			if ( rc != SECSuccess ) {
				errcode = PORT_GetError();
				Debug( LDAP_DEBUG_ANY,
					   "TLS: could not initialize moznss - error %d:%s.\n",
					   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 );
				return -1;
			}

#ifdef HAVE_NSS_INITCONTEXT
			ctx->tc_initctx = initctx;
#endif

			/* initialize the PEM module */
			if ( tlsm_init_pem_module() ) {
				errcode = PORT_GetError();
				Debug( LDAP_DEBUG_ANY,
					   "TLS: could not initialize moznss PEM module - error %d:%s.\n",
					   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), 0 );
				return -1;
			}

			if ( tlsm_init_ca_certs( ctx, lt->lt_cacertfile, lt->lt_cacertdir ) ) {
				return -1;
			}

			ctx->tc_using_pem = PR_TRUE;
		}

#ifdef HAVE_NSS_INITCONTEXT
		if ( !ctx->tc_initctx ) {
			ctx->tc_initctx = initctx;
		}
#endif

		NSS_SetDomesticPolicy();

		PK11_SetPasswordFunc( tlsm_pin_prompt );

		if ( tlsm_init_tokens( ctx ) ) {
			return -1;
		}

		/* register cleanup function */
		/* delete the old one, if any */
		NSS_UnregisterShutdown( tlsm_nss_shutdown_cb, NULL );
		NSS_RegisterShutdown( tlsm_nss_shutdown_cb, NULL );

#ifndef HAVE_NSS_INITCONTEXT
	}
#endif /* HAVE_NSS_INITCONTEXT */

	return 0;
}

static int
tlsm_authenticate( tlsm_ctx *ctx, const char *certname, const char *pininfo )
{
	const char *colon = NULL;
	char *token_name = NULL;
	PK11SlotInfo *slot = NULL;
	int rc = -1;

	if ( !certname || !*certname ) {
		return 0;
	}

	if ( ( colon = PL_strchr( certname, ':' ) ) ) {
		token_name = PL_strndup( certname, colon-certname );
	}

	if ( token_name ) {
		slot = PK11_FindSlotByName( token_name );
	} else {
		slot = PK11_GetInternalKeySlot();
	}

	if ( !slot ) {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could not find the slot for security token %s - error %d:%s.\n",
			   token_name ? token_name : DEFAULT_TOKEN_NAME, errcode,
			   PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		goto done;
	}

	rc = tlsm_authenticate_to_slot( ctx, slot );

done:
	PL_strfree( token_name );
	if ( slot ) {
		PK11_FreeSlot( slot );
	}

	return rc;
}

/*
 * Find and verify the certificate.
 * Either fd is given, in which case the cert will be taken from it via SSL_PeerCertificate
 * or certname is given, and it will be searched for by name
 */
static int
tlsm_find_and_verify_cert_key(tlsm_ctx *ctx, PRFileDesc *fd, const char *certname, int isServer, CERTCertificate **pRetCert, SECKEYPrivateKey **pRetKey)
{
	CERTCertificate *cert = NULL;
	int rc = -1;
	void *pin_arg = NULL;
	SECKEYPrivateKey *key = NULL;

	pin_arg = SSL_RevealPinArg( fd );
	if ( certname ) {
		cert = PK11_FindCertFromNickname( certname, pin_arg );
		if ( !cert ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: error: the certificate %s could not be found in the database - error %d:%s\n",
				   certname, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
			return -1;
		}
	} else {
		/* we are verifying the peer cert
		   we also need to swap the isServer meaning */
		cert = SSL_PeerCertificate( fd );
		if ( !cert ) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: error: could not get the certificate from the peer connection - error %d:%s\n",
				   errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ), NULL );
			return -1;
		}
		isServer = !isServer; /* verify the peer's cert instead */
	}

	if ( ctx->tc_slotname ) {
		PK11SlotInfo *slot = PK11_FindSlotByName( ctx->tc_slotname );
		key = PK11_FindPrivateKeyFromCert( slot, cert, NULL );
		PK11_FreeSlot( slot );
	} else {
		key = PK11_FindKeyByAnyCert( cert, pin_arg );
	}

	if (key) {
		SECCertificateUsage certUsage;
		PRBool checkSig = PR_TRUE;
		SECStatus status;

		if ( pRetKey ) {
			*pRetKey = key; /* caller will deal with this */
		} else {
			SECKEY_DestroyPrivateKey( key );
		}
		if ( isServer ) {
			certUsage = certificateUsageSSLServer;
		} else {
			certUsage = certificateUsageSSLClient;
		}
		if ( ctx->tc_verify_cert ) {
			checkSig = PR_TRUE;
		} else {
			checkSig = PR_FALSE;
		}
		status = CERT_VerifyCertificateNow( ctx->tc_certdb, cert,
											checkSig, certUsage,
											pin_arg, NULL );
		if (status != SECSuccess) {
			PRErrorCode errcode = PR_GetError();
			Debug( LDAP_DEBUG_ANY,
				   "TLS: error: the certificate %s is not valid - error %d:%s\n",
				   certname, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
		} else {
			rc = 0; /* success */
		}
	} else {
		PRErrorCode errcode = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: error: could not find the private key for certificate %s - error %d:%s\n",
			   certname, errcode, PR_ErrorToString( errcode, PR_LANGUAGE_I_DEFAULT ) );
	}

	if ( pRetCert ) {
		*pRetCert = cert; /* caller will deal with this */
	} else {
		CERT_DestroyCertificate( cert );
	}

    return rc;
}

static int
tlsm_get_client_auth_data( void *arg, PRFileDesc *fd,
						   CERTDistNames *caNames, CERTCertificate **pRetCert,
						   SECKEYPrivateKey **pRetKey )
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;
	int rc;

	/* don't need caNames - this function will call CERT_VerifyCertificateNow
	   which will verify the cert against the known CAs */
	rc = tlsm_find_and_verify_cert_key( ctx, fd, ctx->tc_certname, 0, pRetCert, pRetKey );
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "TLS: error: unable to perform client certificate authentication for "
			   "certificate named %s\n", ctx->tc_certname, 0, 0 );
		return SECFailure;
	}

	return SECSuccess;
}

/*
 * ctx must have a tc_model that is valid
 * certname is in the form [<tokenname>:]<certnickname>
 * where <tokenname> is the name of the PKCS11 token
 * and <certnickname> is the nickname of the cert/key in
 * the database
*/
static int
tlsm_clientauth_init( tlsm_ctx *ctx )
{
	SECStatus status = SECFailure;
	int rc;

	rc = tlsm_find_and_verify_cert_key( ctx, ctx->tc_model, ctx->tc_certname, 0, NULL, NULL );
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   "TLS: error: unable to set up client certificate authentication for "
			   "certificate named %s\n", ctx->tc_certname, 0, 0 );
		return -1;
	}

	status = SSL_GetClientAuthDataHook( ctx->tc_model,
										tlsm_get_client_auth_data,
										(void *)ctx );

	return ( status == SECSuccess ? 0 : -1 );
}

/*
 * Tear down the TLS subsystem. Should only be called once.
 */
static void
tlsm_destroy( void )
{
}

static tls_ctx *
tlsm_ctx_new ( struct ldapoptions *lo )
{
	tlsm_ctx *ctx;

	ctx = LDAP_MALLOC( sizeof (*ctx) );
	if ( ctx ) {
		ctx->tc_refcnt = 1;
#ifdef LDAP_R_COMPILE
		ldap_pvt_thread_mutex_init( &ctx->tc_refmutex );
#endif
		ctx->tc_config = &lo->ldo_tls_info; /* pointer into lo structure - must have global scope and must not go away before we can do real init */
		ctx->tc_certdb = NULL;
		ctx->tc_certname = NULL;
		ctx->tc_pin_file = NULL;
		ctx->tc_model = NULL;
		memset(&ctx->tc_callonce, 0, sizeof(ctx->tc_callonce));
		ctx->tc_require_cert = lo->ldo_tls_require_cert;
		ctx->tc_verify_cert = PR_FALSE;
		ctx->tc_using_pem = PR_FALSE;
		ctx->tc_slotname = NULL;
#ifdef HAVE_NSS_INITCONTEXT
		ctx->tc_initctx = NULL;
#endif /* HAVE_NSS_INITCONTEXT */
		ctx->tc_pem_objs = NULL;
		ctx->tc_n_pem_objs = 0;
	}
	return (tls_ctx *)ctx;
}

static void
tlsm_ctx_ref( tls_ctx *ctx )
{
	tlsm_ctx *c = (tlsm_ctx *)ctx;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &c->tc_refmutex );
#endif
	c->tc_refcnt++;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &c->tc_refmutex );
#endif
}

static void
tlsm_ctx_free ( tls_ctx *ctx )
{
	tlsm_ctx *c = (tlsm_ctx *)ctx;
	int refcount;

	if ( !c ) return;

#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &c->tc_refmutex );
#endif
	refcount = --c->tc_refcnt;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &c->tc_refmutex );
#endif
	if ( refcount )
		return;
	if ( c->tc_model )
		PR_Close( c->tc_model );
	c->tc_certdb = NULL; /* if not the default, may have to clean up */
	PL_strfree( c->tc_certname );
	c->tc_certname = NULL;
	PL_strfree( c->tc_pin_file );
	c->tc_pin_file = NULL;
	PL_strfree( c->tc_slotname );		
	tlsm_free_pem_objs( c );
#ifdef HAVE_NSS_INITCONTEXT
	if (c->tc_initctx)
		NSS_ShutdownContext( c->tc_initctx );
	c->tc_initctx = NULL;
#endif /* HAVE_NSS_INITCONTEXT */
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_destroy( &c->tc_refmutex );
#endif
	LDAP_FREE( c );
}

/*
 * initialize a new TLS context
 */
static int
tlsm_ctx_init( struct ldapoptions *lo, struct ldaptls *lt, int is_server )
{
	tlsm_ctx *ctx = (tlsm_ctx *)lo->ldo_tls_ctx;
	ctx->tc_is_server = is_server;

	return 0;
}

static int
tlsm_deferred_ctx_init( void *arg )
{
	tlsm_ctx *ctx = (tlsm_ctx *)arg;
	PRBool sslv2 = PR_FALSE;
	PRBool sslv3 = PR_TRUE;
	PRBool tlsv1 = PR_TRUE;
	PRBool request_cert = PR_FALSE;
	PRInt32 require_cert = PR_FALSE;
	PRFileDesc *fd;
	struct ldaptls *lt;

	if ( tlsm_deferred_init( ctx ) ) {
	    Debug( LDAP_DEBUG_ANY,
			   "TLS: could perform TLS system initialization.\n",
			   0, 0, 0 );
	    return -1;
	}

	ctx->tc_certdb = CERT_GetDefaultCertDB(); /* If there is ever a per-context db, change this */

	fd = PR_CreateIOLayerStub( tlsm_layer_id, &tlsm_PR_methods );
	if ( fd ) {
		ctx->tc_model = SSL_ImportFD( NULL, fd );
	}

	if ( !ctx->tc_model ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_ANY,
			   "TLS: could perform TLS socket I/O layer initialization - error %d:%s.\n",
			   err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );

		if ( fd ) {
			PR_Close( fd );
		}
		return -1;
	}

	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_SECURITY, PR_TRUE ) ) {
		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set secure mode on.\n",
		       0, 0, 0 );
		return -1;
	}

	lt = ctx->tc_config;

	/* default is sslv3 and tlsv1 */
	if ( lt->lt_protocol_min ) {
		if ( lt->lt_protocol_min > LDAP_OPT_X_TLS_PROTOCOL_SSL3 ) {
			sslv3 = PR_FALSE;
		} else if ( lt->lt_protocol_min <= LDAP_OPT_X_TLS_PROTOCOL_SSL2 ) {
			sslv2 = PR_TRUE;
			Debug( LDAP_DEBUG_ANY,
			       "TLS: warning: minimum TLS protocol level set to "
			       "include SSLv2 - SSLv2 is insecure - do not use\n", 0, 0, 0 );
		}
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_ENABLE_SSL2, sslv2 ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set SSLv2 mode on.\n",
		       0, 0, 0 );
		return -1;
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_ENABLE_SSL3, sslv3 ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set SSLv3 mode on.\n",
		       0, 0, 0 );
		return -1;
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_ENABLE_TLS, tlsv1 ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set TLSv1 mode on.\n",
		       0, 0, 0 );
		return -1;
	}

	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_HANDSHAKE_AS_CLIENT, !ctx->tc_is_server ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set handshake as client.\n",
		       0, 0, 0 );
		return -1;
	}
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_HANDSHAKE_AS_SERVER, ctx->tc_is_server ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set handshake as server.\n",
		       0, 0, 0 );
		return -1;
	}

 	if ( lt->lt_ciphersuite &&
	     tlsm_parse_ciphers( ctx, lt->lt_ciphersuite )) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set cipher list %s.\n",
		       lt->lt_ciphersuite, 0, 0 );
		return -1;
 	}

	if ( ctx->tc_require_cert ) {
		request_cert = PR_TRUE;
		require_cert = SSL_REQUIRE_NO_ERROR;
		if ( ctx->tc_require_cert == LDAP_OPT_X_TLS_DEMAND ||
		     ctx->tc_require_cert == LDAP_OPT_X_TLS_HARD ) {
			require_cert = SSL_REQUIRE_ALWAYS;
		}
		if ( ctx->tc_require_cert != LDAP_OPT_X_TLS_ALLOW )
			ctx->tc_verify_cert = PR_TRUE;
	} else {
		ctx->tc_verify_cert = PR_FALSE;
	}

	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_REQUEST_CERTIFICATE, request_cert ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set request certificate mode.\n",
		       0, 0, 0 );
		return -1;
	}
		
	if ( SECSuccess != SSL_OptionSet( ctx->tc_model, SSL_REQUIRE_CERTIFICATE, require_cert ) ) {
 		Debug( LDAP_DEBUG_ANY,
		       "TLS: could not set require certificate mode.\n",
		       0, 0, 0 );
		return -1;
	}

	/* set up our cert and key, if any */
	if ( lt->lt_certfile ) {
		/* if using the PEM module, load the PEM file specified by lt_certfile */
		/* otherwise, assume this is the name of a cert already in the db */
		if ( ctx->tc_using_pem ) {
			/* this sets ctx->tc_certname to the correct value */
			int rc = tlsm_add_cert_from_file( ctx, lt->lt_certfile, PR_FALSE /* not a ca */ );
			if ( rc ) {
				return rc;
			}
		} else {
			PL_strfree( ctx->tc_certname );
			ctx->tc_certname = PL_strdup( lt->lt_certfile );
		}
	}

	if ( lt->lt_keyfile ) {
		/* if using the PEM module, load the PEM file specified by lt_keyfile */
		/* otherwise, assume this is the pininfo for the key */
		if ( ctx->tc_using_pem ) {
			/* this sets ctx->tc_certname to the correct value */
			int rc = tlsm_add_key_from_file( ctx, lt->lt_keyfile );
			if ( rc ) {
				return rc;
			}
		} else {
			PL_strfree( ctx->tc_pin_file );
			ctx->tc_pin_file = PL_strdup( lt->lt_keyfile );
		}
	}

	/* Set up callbacks for use by clients */
	if ( !ctx->tc_is_server ) {
		if ( SSL_OptionSet( ctx->tc_model, SSL_NO_CACHE, PR_TRUE ) != SECSuccess ) {
			PRErrorCode err = PR_GetError();
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: could not set nocache option for moznss - error %d:%s\n",
			       err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );
			return -1;
		}

		if ( SSL_BadCertHook( ctx->tc_model, tlsm_bad_cert_handler, ctx ) != SECSuccess ) {
			PRErrorCode err = PR_GetError();
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: could not set bad cert handler for moznss - error %d:%s\n",
			       err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );
			return -1;
		}

		/* 
		   since a cert has been specified, assume the client wants to do cert auth
		*/
		if ( ctx->tc_certname ) {
			if ( tlsm_authenticate( ctx, ctx->tc_certname, ctx->tc_pin_file ) ) {
				Debug( LDAP_DEBUG_ANY, 
				       "TLS: error: unable to authenticate to the security device for certificate %s\n",
				       ctx->tc_certname, 0, 0 );
				return -1;
			}
			if ( tlsm_clientauth_init( ctx ) ) {
				Debug( LDAP_DEBUG_ANY, 
				       "TLS: error: unable to set up client certificate authentication using %s\n",
				       ctx->tc_certname, 0, 0 );
				return -1;
			}
		}
	} else { /* set up secure server */
		SSLKEAType certKEA;
		CERTCertificate *serverCert;
		SECKEYPrivateKey *serverKey;
		SECStatus status;

		/* must have a certificate for the server to use */
		if ( !ctx->tc_certname ) {
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: no server certificate: must specify a certificate for the server to use\n",
			       0, 0, 0 );
			return -1;
		}

		/* authenticate to the server's token - this will do nothing
		   if the key/cert db is not password protected */
		if ( tlsm_authenticate( ctx, ctx->tc_certname, ctx->tc_pin_file ) ) {
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: unable to authenticate to the security device for certificate %s\n",
			       ctx->tc_certname, 0, 0 );
			return -1;
		}

		/* get the server's key and cert */
		if ( tlsm_find_and_verify_cert_key( ctx, ctx->tc_model, ctx->tc_certname, ctx->tc_is_server,
						    &serverCert, &serverKey ) ) {
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: unable to find and verify server's cert and key for certificate %s\n",
			       ctx->tc_certname, 0, 0 );
			return -1;
		}

		certKEA = NSS_FindCertKEAType( serverCert );
		/* configure the socket to be a secure server socket */
		status = SSL_ConfigSecureServer( ctx->tc_model, serverCert, serverKey, certKEA );
		/* SSL_ConfigSecureServer copies these */
		CERT_DestroyCertificate( serverCert );
		SECKEY_DestroyPrivateKey( serverKey );

		if ( SECSuccess != status ) {
			PRErrorCode err = PR_GetError();
			Debug( LDAP_DEBUG_ANY, 
			       "TLS: error: unable to configure secure server using certificate %s - error %d:%s\n",
			       ctx->tc_certname, err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) );
			return -1;
		}
	}

	/* Callback for authenticating certificate */
	if ( SSL_AuthCertificateHook( ctx->tc_model, tlsm_auth_cert_handler,
                                  ctx->tc_certdb ) != SECSuccess ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: error: could not set auth cert handler for moznss - error %d:%s\n",
		       err, PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ), NULL );
		return -1;
	}

	return 0;
}

struct tls_data {
	tlsm_session		*session;
	Sockbuf_IO_Desc		*sbiod;
	/* there seems to be no portable way to determine if the
	   sockbuf sd has been set to nonblocking mode - the
	   call to ber_pvt_socket_set_nonblock() takes place
	   before the tls socket is set up, so we cannot
	   intercept that call either.
	   On systems where fcntl is available, we can just
	   F_GETFL and test for O_NONBLOCK.  On other systems,
	   we will just see if the IO op returns EAGAIN or EWOULDBLOCK,
	   and just set this flag */
	PRBool              nonblock;
};

static int
tlsm_is_io_ready( PRFileDesc *fd, PRInt16 in_flags, PRInt16 *out_flags )
{
	struct tls_data		*p;
	PRFileDesc *pollfd = NULL;
	PRFileDesc *myfd;
	PRPollDesc polldesc;
	int rc;

	myfd = PR_GetIdentitiesLayer( fd, tlsm_layer_id );

	if ( !myfd ) {
		return 0;
	}

	p = (struct tls_data *)myfd->secret;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	/* wrap the sockbuf fd with a NSPR FD created especially
	   for use with polling, and only with polling */
	pollfd = PR_CreateSocketPollFd( p->sbiod->sbiod_sb->sb_fd );
	polldesc.fd = pollfd;
	polldesc.in_flags = in_flags;
	polldesc.out_flags = 0;

	/* do the poll - no waiting, no blocking */
	rc = PR_Poll( &polldesc, 1, PR_INTERVAL_NO_WAIT );

	/* unwrap the socket */
	PR_DestroySocketPollFd( pollfd );

	/* rc will be either 1 if IO is ready, 0 if IO is not
	   ready, or -1 if there was some error (and the caller
	   should use PR_GetError() to figure out what */
	if (out_flags) {
		*out_flags = polldesc.out_flags;
	}
	return rc;
}

static tls_session *
tlsm_session_new ( tls_ctx * ctx, int is_server )
{
	tlsm_ctx *c = (tlsm_ctx *)ctx;
	tlsm_session *session;
	PRFileDesc *fd;
	PRStatus status;

	c->tc_is_server = is_server;
	status = PR_CallOnceWithArg( &c->tc_callonce, tlsm_deferred_ctx_init, c );
	if ( PR_SUCCESS != status ) {
		PRErrorCode err = PR_GetError();
		Debug( LDAP_DEBUG_ANY, 
		       "TLS: error: could not initialize moznss security context - error %d:%s\n",
		       err, PR_ErrorToString(err, PR_LANGUAGE_I_DEFAULT), NULL );
		return NULL;
	}

	fd = PR_CreateIOLayerStub( tlsm_layer_id, &tlsm_PR_methods );
	if ( !fd ) {
		return NULL;
	}

	session = SSL_ImportFD( c->tc_model, fd );
	if ( !session ) {
		PR_DELETE( fd );
		return NULL;
	}

	if ( is_server ) {
		/* 0 means use the defaults here */
		SSL_ConfigServerSessionIDCache( 0, 0, 0, NULL );
	}

	return (tls_session *)session;
} 

static int
tlsm_session_accept( tls_session *session )
{
	tlsm_session *s = (tlsm_session *)session;
	int rc;
	PRErrorCode err;
	int waitcounter = 0;

	rc = SSL_ResetHandshake( s, PR_TRUE /* server */ );
	if (rc) {
		err = PR_GetError();
		Debug( LDAP_DEBUG_TRACE, 
			   "TLS: error: accept - reset handshake failure %d - error %d:%s\n",
			   rc, err,
			   err ? PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) : "unknown" );
	}

	do {
		PRInt32 filesReady;
		PRInt16 in_flags;
		PRInt16 out_flags;

		errno = 0;
		rc = SSL_ForceHandshake( s );
		if (rc == SECSuccess) {
			rc = 0;
			break; /* done */
		}
		err = PR_GetError();
		Debug( LDAP_DEBUG_TRACE, 
			   "TLS: error: accept - force handshake failure %d - error %d waitcounter %d\n",
			   errno, err, waitcounter );
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			waitcounter++;
			in_flags = PR_POLL_READ | PR_POLL_EXCEPT;
			out_flags = 0;
			errno = 0;
			filesReady = tlsm_is_io_ready( s, in_flags, &out_flags );
			if ( filesReady < 0 ) {
				err = PR_GetError();
				Debug( LDAP_DEBUG_ANY, 
					   "TLS: error: accept - error waiting for socket to be ready: %d - error %d:%s\n",
					   errno, err,
					   err ? PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) : "unknown" );
				rc = -1;
				break; /* hard error */
			} else if ( out_flags & PR_POLL_NVAL ) {
				PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
				Debug( LDAP_DEBUG_ANY, 
					   "TLS: error: accept failure - invalid socket\n",
					   NULL, NULL, NULL );
				rc = -1;
				break;
			} else if ( out_flags & PR_POLL_EXCEPT ) {
				err = PR_GetError();
				Debug( LDAP_DEBUG_ANY, 
					   "TLS: error: accept - error waiting for socket to be ready: %d - error %d:%s\n",
					   errno, err,
					   err ? PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) : "unknown" );
				rc = -1;
				break; /* hard error */
			}
		} else { /* hard error */
			err = PR_GetError();
			Debug( LDAP_DEBUG_ANY, 
				   "TLS: error: accept - force handshake failure: %d - error %d:%s\n",
				   errno, err,
				   err ? PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) : "unknown" );
			rc = -1;
			break; /* hard error */
		}
	} while (rc == SECFailure);

	Debug( LDAP_DEBUG_TRACE, 
		   "TLS: accept completed after %d waits\n", waitcounter, NULL, NULL );

	return rc;
}

static int
tlsm_session_connect( LDAP *ld, tls_session *session )
{
	tlsm_session *s = (tlsm_session *)session;
	int rc;
	PRErrorCode err;

	rc = SSL_ResetHandshake( s, PR_FALSE /* server */ );
	if (rc) {
		err = PR_GetError();
		Debug( LDAP_DEBUG_TRACE, 
			   "TLS: error: connect - reset handshake failure %d - error %d:%s\n",
			   rc, err,
			   err ? PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) : "unknown" );
	}

	rc = SSL_ForceHandshake( s );
	if (rc) {
		err = PR_GetError();
		Debug( LDAP_DEBUG_TRACE, 
			   "TLS: error: connect - force handshake failure %d - error %d:%s\n",
			   rc, err,
			   err ? PR_ErrorToString( err, PR_LANGUAGE_I_DEFAULT ) : "unknown" );
	}

	return rc;
}

static int
tlsm_session_upflags( Sockbuf *sb, tls_session *session, int rc )
{
	/* Should never happen */
	rc = PR_GetError();

	if ( rc != PR_PENDING_INTERRUPT_ERROR && rc != PR_WOULD_BLOCK_ERROR )
		return 0;
	return 0;
}

static char *
tlsm_session_errmsg( tls_session *sess, int rc, char *buf, size_t len )
{
	int i;

	rc = PR_GetError();
	i = PR_GetErrorTextLength();
	if ( i > len ) {
		char *msg = LDAP_MALLOC( i+1 );
		PR_GetErrorText( msg );
		memcpy( buf, msg, len );
		LDAP_FREE( msg );
	} else if ( i ) {
		PR_GetErrorText( buf );
	}

	return i ? buf : NULL;
}

static int
tlsm_session_my_dn( tls_session *session, struct berval *der_dn )
{
	tlsm_session *s = (tlsm_session *)session;
	CERTCertificate *cert;

	cert = SSL_LocalCertificate( s );
	if (!cert) return LDAP_INVALID_CREDENTIALS;

	der_dn->bv_val = cert->derSubject.data;
	der_dn->bv_len = cert->derSubject.len;
	CERT_DestroyCertificate( cert );
	return 0;
}

static int
tlsm_session_peer_dn( tls_session *session, struct berval *der_dn )
{
	tlsm_session *s = (tlsm_session *)session;
	CERTCertificate *cert;

	cert = SSL_PeerCertificate( s );
	if (!cert) return LDAP_INVALID_CREDENTIALS;
	
	der_dn->bv_val = cert->derSubject.data;
	der_dn->bv_len = cert->derSubject.len;
	CERT_DestroyCertificate( cert );
	return 0;
}

/* what kind of hostname were we given? */
#define	IS_DNS	0
#define	IS_IP4	1
#define	IS_IP6	2

static int
tlsm_session_chkhost( LDAP *ld, tls_session *session, const char *name_in )
{
	tlsm_session *s = (tlsm_session *)session;
	CERTCertificate *cert;
	const char *name, *domain = NULL, *ptr;
	int i, ret, ntype = IS_DNS, nlen, dlen;
#ifdef LDAP_PF_INET6
	struct in6_addr addr;
#else
	struct in_addr addr;
#endif
	SECItem altname;
	SECStatus rv;

	if( ldap_int_hostname &&
		( !name_in || !strcasecmp( name_in, "localhost" ) ) )
	{
		name = ldap_int_hostname;
	} else {
		name = name_in;
	}
	nlen = strlen( name );

	cert = SSL_PeerCertificate( s );
	if (!cert) {
		Debug( LDAP_DEBUG_ANY,
			"TLS: unable to get peer certificate.\n",
			0, 0, 0 );
		/* if this was a fatal condition, things would have
		 * aborted long before now.
		 */
		return LDAP_SUCCESS;
	}

#ifdef LDAP_PF_INET6
	if (name[0] == '[' && strchr(name, ']')) {
		char *n2 = ldap_strdup(name+1);
		*strchr(n2, ']') = 0;
		if (inet_pton(AF_INET6, n2, &addr))
			ntype = IS_IP6;
		LDAP_FREE(n2);
	} else 
#endif
	if ((ptr = strrchr(name, '.')) && isdigit((unsigned char)ptr[1])) {
		if (inet_aton(name, (struct in_addr *)&addr)) ntype = IS_IP4;
	}
	if (ntype == IS_DNS ) {
		domain = strchr( name, '.' );
		if ( domain )
			dlen = nlen - ( domain - name );
	}

	ret = LDAP_LOCAL_ERROR;

	rv = CERT_FindCertExtension( cert, SEC_OID_X509_SUBJECT_ALT_NAME,
		&altname );
	if ( rv == SECSuccess && altname.data ) {
		PRArenaPool *arena;
		CERTGeneralName *names, *cur;

		arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
		if ( !arena ) {
			ret = LDAP_NO_MEMORY;
			goto fail;
		}

		names = cur = CERT_DecodeAltNameExtension(arena, &altname);
		if ( !cur )
			goto altfail;

		do {
			char *host;
			int hlen;

			/* ignore empty */
			if ( !cur->name.other.len ) continue;

			host = cur->name.other.data;
			hlen = cur->name.other.len;

			if ( cur->type == certDNSName ) {
				if ( ntype != IS_DNS )	continue;

				/* is this an exact match? */
				if ( nlen == hlen && !strncasecmp( name, host, nlen )) {
					ret = LDAP_SUCCESS;
					break;
				}

				/* is this a wildcard match? */
				if ( domain && host[0] == '*' && host[1] == '.' &&
					dlen == hlen-1 && !strncasecmp( domain, host+1, dlen )) {
					ret = LDAP_SUCCESS;
					break;
				}
			} else if ( cur->type == certIPAddress ) {
				if ( ntype == IS_DNS )	continue;
				
#ifdef LDAP_PF_INET6
				if (ntype == IS_IP6 && hlen != sizeof(struct in6_addr)) {
					continue;
				} else
#endif
				if (ntype == IS_IP4 && hlen != sizeof(struct in_addr)) {
					continue;
				}
				if (!memcmp(host, &addr, hlen)) {
					ret = LDAP_SUCCESS;
					break;
				}
			}
		} while (( cur = CERT_GetNextGeneralName( cur )) != names );
altfail:
		PORT_FreeArena( arena, PR_FALSE );
		SECITEM_FreeItem( &altname, PR_FALSE );
	}
	/* no altnames matched, try the CN */
	if ( ret != LDAP_SUCCESS ) {
		/* find the last CN */
		CERTRDN *rdn, **rdns;
		CERTAVA *lastava = NULL;
		char buf[2048];

		buf[0] = '\0';
		rdns = cert->subject.rdns;
		while ( rdns && ( rdn = *rdns++ )) {
			CERTAVA *ava, **avas = rdn->avas;
			while ( avas && ( ava = *avas++ )) {
				if ( CERT_GetAVATag( ava ) == SEC_OID_AVA_COMMON_NAME )
					lastava = ava;
			}
		}
		if ( lastava ) {
			SECItem *av = CERT_DecodeAVAValue( &lastava->value );
			if ( av ) {
				if ( av->len == nlen && !strncasecmp( name, av->data, nlen )) {
					ret = LDAP_SUCCESS;
				} else if ( av->data[0] == '*' && av->data[1] == '.' &&
					domain && dlen == av->len - 1 && !strncasecmp( name,
						av->data+1, dlen )) {
					ret = LDAP_SUCCESS;
				} else {
					int len = av->len;
					if ( len >= sizeof(buf) )
						len = sizeof(buf)-1;
					memcpy( buf, av->data, len );
					buf[len] = '\0';
				}
				SECITEM_FreeItem( av, PR_TRUE );
			}
		}
		if ( ret != LDAP_SUCCESS ) {
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

fail:
	CERT_DestroyCertificate( cert );
	return ret;
}

static int
tlsm_session_strength( tls_session *session )
{
	tlsm_session *s = (tlsm_session *)session;
	int rc, keySize;

	rc = SSL_SecurityStatus( s, NULL, NULL, NULL, &keySize,
		NULL, NULL );
	return rc ? 0 : keySize;
}

/*
 * TLS support for LBER Sockbufs
 */

static PRStatus PR_CALLBACK
tlsm_PR_Close(PRFileDesc *fd)
{
	int rc = PR_SUCCESS;

	/* we don't need to actually close anything here, just
	   pop our io layer off the stack */
	fd->secret = NULL; /* must have been freed before calling PR_Close */
	if ( fd->lower ) {
		fd = PR_PopIOLayer( fd, tlsm_layer_id );
		/* if we are not the last layer, pass the close along */
		if ( fd ) {
			if ( fd->dtor ) {
				fd->dtor( fd );
			}
			rc = fd->methods->close( fd );
		}
	} else {
		/* we are the last layer - just call our dtor */
		fd->dtor(fd);
	}

	return rc;
}

static PRStatus PR_CALLBACK
tlsm_PR_Shutdown(PRFileDesc *fd, PRShutdownHow how)
{
	int rc = PR_SUCCESS;

	if ( fd->lower ) {
		rc = PR_Shutdown( fd->lower, how );
	}

	return rc;
}

static int PR_CALLBACK
tlsm_PR_Recv(PRFileDesc *fd, void *buf, PRInt32 len, PRIntn flags,
	 PRIntervalTime timeout)
{
	struct tls_data		*p;
	int rc;

	if ( buf == NULL || len <= 0 ) return 0;

	p = (struct tls_data *)fd->secret;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	rc = LBER_SBIOD_READ_NEXT( p->sbiod, buf, len );
	if (rc <= 0) {
		tlsm_map_error( errno );
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			p->nonblock = PR_TRUE; /* fd is using non-blocking io */
		} else if ( errno ) { /* real error */
			Debug( LDAP_DEBUG_TRACE, 
			       "TLS: error: tlsm_PR_Recv returned %d - error %d:%s\n",
			       rc, errno, STRERROR(errno) );
		}
	}

	return rc;
}

static int PR_CALLBACK
tlsm_PR_Send(PRFileDesc *fd, const void *buf, PRInt32 len, PRIntn flags,
	 PRIntervalTime timeout)
{
	struct tls_data		*p;
	int rc;

	if ( buf == NULL || len <= 0 ) return 0;

	p = (struct tls_data *)fd->secret;

	if ( p == NULL || p->sbiod == NULL ) {
		return 0;
	}

	rc = LBER_SBIOD_WRITE_NEXT( p->sbiod, (char *)buf, len );
	if (rc <= 0) {
		tlsm_map_error( errno );
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			p->nonblock = PR_TRUE;
		} else if ( errno ) { /* real error */
			Debug( LDAP_DEBUG_TRACE, 
			       "TLS: error: tlsm_PR_Send returned %d - error %d:%s\n",
			       rc, errno, STRERROR(errno) );
		}
	}

	return rc;
}

static int PR_CALLBACK
tlsm_PR_Read(PRFileDesc *fd, void *buf, PRInt32 len)
{
	return tlsm_PR_Recv( fd, buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
}

static int PR_CALLBACK
tlsm_PR_Write(PRFileDesc *fd, const void *buf, PRInt32 len)
{
	return tlsm_PR_Send( fd, buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
}

static PRStatus PR_CALLBACK
tlsm_PR_GetPeerName(PRFileDesc *fd, PRNetAddr *addr)
{
	struct tls_data		*p;
	int rc;
	ber_socklen_t len;

	p = (struct tls_data *)fd->secret;

	if ( p == NULL || p->sbiod == NULL ) {
		return PR_FAILURE;
	}
	len = sizeof(PRNetAddr);
	return getpeername( p->sbiod->sbiod_sb->sb_fd, (struct sockaddr *)addr, &len );
}

static PRStatus PR_CALLBACK
tlsm_PR_GetSocketOption(PRFileDesc *fd, PRSocketOptionData *data)
{
	struct tls_data		*p;
	p = (struct tls_data *)fd->secret;

	if ( !data ) {
		return PR_FAILURE;
	}

	/* only the nonblocking option is supported at this time
	   MozNSS SSL code needs it */
	if ( data->option != PR_SockOpt_Nonblocking ) {
		PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
		return PR_FAILURE;
	}
#ifdef HAVE_FCNTL
	int flags = fcntl( p->sbiod->sbiod_sb->sb_fd, F_GETFL );
	data->value.non_blocking = (flags & O_NONBLOCK) ? PR_TRUE : PR_FALSE;		
#else /* punt :P */
	data->value.non_blocking = p->nonblock;
#endif
	return PR_SUCCESS;
}

static PRStatus PR_CALLBACK
tlsm_PR_prs_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

static PRFileDesc * PR_CALLBACK
tlsm_PR_pfd_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

static PRInt16 PR_CALLBACK
tlsm_PR_i16_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static PRInt32 PR_CALLBACK
tlsm_PR_i32_unimp()
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static PRInt64 PR_CALLBACK
tlsm_PR_i64_unimp()
{
    PRInt64 res;

    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    LL_I2L(res, -1L);
    return res;
}

static const PRIOMethods tlsm_PR_methods = {
    PR_DESC_LAYERED,
    tlsm_PR_Close,			/* close        */
    tlsm_PR_Read,			/* read         */
    tlsm_PR_Write,			/* write        */
    tlsm_PR_i32_unimp,		/* available    */
    tlsm_PR_i64_unimp,		/* available64  */
    tlsm_PR_prs_unimp,		/* fsync        */
    tlsm_PR_i32_unimp,		/* seek         */
    tlsm_PR_i64_unimp,		/* seek64       */
    tlsm_PR_prs_unimp,		/* fileInfo     */
    tlsm_PR_prs_unimp,		/* fileInfo64   */
    tlsm_PR_i32_unimp,		/* writev       */
    tlsm_PR_prs_unimp,		/* connect      */
    tlsm_PR_pfd_unimp,		/* accept       */
    tlsm_PR_prs_unimp,		/* bind         */
    tlsm_PR_prs_unimp,		/* listen       */
    (PRShutdownFN)tlsm_PR_Shutdown,			/* shutdown     */
    tlsm_PR_Recv,			/* recv         */
    tlsm_PR_Send,			/* send         */
    tlsm_PR_i32_unimp,		/* recvfrom     */
    tlsm_PR_i32_unimp,		/* sendto       */
    (PRPollFN)tlsm_PR_i16_unimp,	/* poll         */
    tlsm_PR_i32_unimp,		/* acceptread   */
    tlsm_PR_i32_unimp,		/* transmitfile */
    tlsm_PR_prs_unimp,		/* getsockname  */
    tlsm_PR_GetPeerName,	/* getpeername  */
    tlsm_PR_i32_unimp,		/* getsockopt   OBSOLETE */
    tlsm_PR_i32_unimp,		/* setsockopt   OBSOLETE */
    tlsm_PR_GetSocketOption,		/* getsocketoption   */
    tlsm_PR_i32_unimp,		/* setsocketoption   */
    tlsm_PR_i32_unimp,		/* Send a (partial) file with header/trailer*/
    (PRConnectcontinueFN)tlsm_PR_prs_unimp,		/* connectcontinue */
    tlsm_PR_i32_unimp,		/* reserved for future use */
    tlsm_PR_i32_unimp,		/* reserved for future use */
    tlsm_PR_i32_unimp,		/* reserved for future use */
    tlsm_PR_i32_unimp		/* reserved for future use */
};

/*
 * Initialize TLS subsystem. Should be called only once.
 * See tlsm_deferred_init for the bulk of the init process
 */
static int
tlsm_init( void )
{
	PR_Init(0, 0, 0);

	tlsm_layer_id = PR_GetUniqueIdentity( "OpenLDAP" );

	return 0;
}

static int
tlsm_sb_setup( Sockbuf_IO_Desc *sbiod, void *arg )
{
	struct tls_data		*p;
	tlsm_session	*session = arg;
	PRFileDesc *fd;

	assert( sbiod != NULL );

	p = LBER_MALLOC( sizeof( *p ) );
	if ( p == NULL ) {
		return -1;
	}

	fd = PR_GetIdentitiesLayer( session, tlsm_layer_id );
	if ( !fd ) {
		LBER_FREE( p );
		return -1;
	}

	fd->secret = (PRFilePrivate *)p;
	p->session = session;
	p->sbiod = sbiod;
	sbiod->sbiod_pvt = p;
	return 0;
}

static int
tlsm_sb_remove( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	PR_Close( p->session );
	LBER_FREE( sbiod->sbiod_pvt );
	sbiod->sbiod_pvt = NULL;
	return 0;
}

static int
tlsm_sb_close( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	PR_Shutdown( p->session, PR_SHUTDOWN_BOTH );
	return 0;
}

static int
tlsm_sb_ctrl( Sockbuf_IO_Desc *sbiod, int opt, void *arg )
{
	struct tls_data		*p;
	
	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	p = (struct tls_data *)sbiod->sbiod_pvt;
	
	if ( opt == LBER_SB_OPT_GET_SSL ) {
		*((tlsm_session **)arg) = p->session;
		return 1;
		
	} else if ( opt == LBER_SB_OPT_DATA_READY ) {
		if ( tlsm_is_io_ready( p->session, PR_POLL_READ, NULL ) > 0 ) {
			return 1;
		}
		
	}
	
	return LBER_SBIOD_CTRL_NEXT( sbiod, opt, arg );
}

static ber_slen_t
tlsm_sb_read( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = PR_Recv( p->session, buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
	if ( ret < 0 ) {
		err = PR_GetError();
		if ( err == PR_PENDING_INTERRUPT_ERROR || err == PR_WOULD_BLOCK_ERROR ) {
			sbiod->sbiod_sb->sb_trans_needs_read = 1;
			sock_errset(EWOULDBLOCK);
		}
	} else {
		sbiod->sbiod_sb->sb_trans_needs_read = 0;
	}
	return ret;
}

static ber_slen_t
tlsm_sb_write( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data		*p;
	ber_slen_t		ret;
	int			err;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct tls_data *)sbiod->sbiod_pvt;

	ret = PR_Send( p->session, (char *)buf, len, 0, PR_INTERVAL_NO_TIMEOUT );
	if ( ret < 0 ) {
		err = PR_GetError();
		if ( err == PR_PENDING_INTERRUPT_ERROR || err == PR_WOULD_BLOCK_ERROR ) {
			sbiod->sbiod_sb->sb_trans_needs_write = 1;
			sock_errset(EWOULDBLOCK);
			ret = 0;
		}
	} else {
		sbiod->sbiod_sb->sb_trans_needs_write = 0;
	}
	return ret;
}

static Sockbuf_IO tlsm_sbio =
{
	tlsm_sb_setup,		/* sbi_setup */
	tlsm_sb_remove,		/* sbi_remove */
	tlsm_sb_ctrl,		/* sbi_ctrl */
	tlsm_sb_read,		/* sbi_read */
	tlsm_sb_write,		/* sbi_write */
	tlsm_sb_close		/* sbi_close */
};

tls_impl ldap_int_tls_impl = {
	"MozNSS",

	tlsm_init,
	tlsm_destroy,

	tlsm_ctx_new,
	tlsm_ctx_ref,
	tlsm_ctx_free,
	tlsm_ctx_init,

	tlsm_session_new,
	tlsm_session_connect,
	tlsm_session_accept,
	tlsm_session_upflags,
	tlsm_session_errmsg,
	tlsm_session_my_dn,
	tlsm_session_peer_dn,
	tlsm_session_chkhost,
	tlsm_session_strength,

	&tlsm_sbio,

#ifdef LDAP_R_COMPILE
	tlsm_thr_init,
#else
	NULL,
#endif

	0
};

#endif /* HAVE_MOZNSS */
/*
  emacs settings
  Local Variables:
  indent-tabs-mode: t
  tab-width: 4
  End:
*/
