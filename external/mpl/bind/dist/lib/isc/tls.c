/*	$NetBSD: tls.c,v 1.3.2.1 2024/02/25 15:47:19 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <inttypes.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#if HAVE_LIBNGHTTP2
#include <nghttp2/nghttp2.h>
#endif /* HAVE_LIBNGHTTP2 */
#include <arpa/inet.h>

#include <openssl/bn.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/dh.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>

#include <isc/atomic.h>
#include <isc/ht.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mutex.h>
#include <isc/mutexblock.h>
#include <isc/once.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/sockaddr.h>
#include <isc/thread.h>
#include <isc/tls.h>
#include <isc/util.h>

#include "openssl_shim.h"
#include "tls_p.h"

#define COMMON_SSL_OPTIONS \
	(SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION)

static isc_once_t init_once = ISC_ONCE_INIT;
static isc_once_t shut_once = ISC_ONCE_INIT;
static atomic_bool init_done = false;
static atomic_bool shut_done = false;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static isc_mutex_t *locks = NULL;
static int nlocks;

static void
isc__tls_lock_callback(int mode, int type, const char *file, int line) {
	UNUSED(file);
	UNUSED(line);
	if ((mode & CRYPTO_LOCK) != 0) {
		LOCK(&locks[type]);
	} else {
		UNLOCK(&locks[type]);
	}
}

static void
isc__tls_set_thread_id(CRYPTO_THREADID *id) {
	CRYPTO_THREADID_set_numeric(id, (unsigned long)isc_thread_self());
}
#endif

static void
tls_initialize(void) {
	REQUIRE(!atomic_load(&init_done));

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	RUNTIME_CHECK(OPENSSL_init_ssl(OPENSSL_INIT_ENGINE_ALL_BUILTIN |
					       OPENSSL_INIT_LOAD_CONFIG,
				       NULL) == 1);
#else
	nlocks = CRYPTO_num_locks();
	/*
	 * We can't use isc_mem API here, because it's called too
	 * early and when the isc_mem_debugging flags are changed
	 * later.
	 *
	 * Actually, since this is a single allocation at library load
	 * and deallocation at library unload, using the standard
	 * allocator without the tracking is fine for this purpose.
	 */
	locks = calloc(nlocks, sizeof(locks[0]));
	isc_mutexblock_init(locks, nlocks);
	CRYPTO_set_locking_callback(isc__tls_lock_callback);
	CRYPTO_THREADID_set_callback(isc__tls_set_thread_id);

	CRYPTO_malloc_init();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	SSL_library_init();

#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
	ENGINE_load_builtin_engines();
#endif
	OpenSSL_add_all_algorithms();
	OPENSSL_load_builtin_modules();

	CONF_modules_load_file(NULL, NULL,
			       CONF_MFLAGS_DEFAULT_SECTION |
				       CONF_MFLAGS_IGNORE_MISSING_FILE);
#endif

	/* Protect ourselves against unseeded PRNG */
	if (RAND_status() != 1) {
		FATAL_ERROR("OpenSSL pseudorandom number generator "
			    "cannot be initialized (see the `PRNG not "
			    "seeded' message in the OpenSSL FAQ)");
	}

	atomic_compare_exchange_enforced(&init_done, &(bool){ false }, true);
}

void
isc__tls_initialize(void) {
	isc_result_t result = isc_once_do(&init_once, tls_initialize);
	REQUIRE(result == ISC_R_SUCCESS);
	REQUIRE(atomic_load(&init_done));
}

static void
tls_shutdown(void) {
	REQUIRE(atomic_load(&init_done));
	REQUIRE(!atomic_load(&shut_done));

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	OPENSSL_cleanup();
#else
	CONF_modules_unload(1);
	OBJ_cleanup();
	EVP_cleanup();
#if !defined(OPENSSL_NO_ENGINE) && OPENSSL_API_LEVEL < 30000
	ENGINE_cleanup();
#endif
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_thread_state(NULL);
	RAND_cleanup();
	ERR_free_strings();

	CRYPTO_set_locking_callback(NULL);

	if (locks != NULL) {
		isc_mutexblock_destroy(locks, nlocks);
		free(locks);
		locks = NULL;
	}
#endif

	atomic_compare_exchange_enforced(&shut_done, &(bool){ false }, true);
}

void
isc__tls_shutdown(void) {
	isc_result_t result = isc_once_do(&shut_once, tls_shutdown);
	REQUIRE(result == ISC_R_SUCCESS);
	REQUIRE(atomic_load(&shut_done));
}

void
isc_tlsctx_free(isc_tlsctx_t **ctxp) {
	SSL_CTX *ctx = NULL;
	REQUIRE(ctxp != NULL && *ctxp != NULL);

	ctx = *ctxp;
	*ctxp = NULL;

	SSL_CTX_free(ctx);
}

void
isc_tlsctx_attach(isc_tlsctx_t *src, isc_tlsctx_t **ptarget) {
	REQUIRE(src != NULL);
	REQUIRE(ptarget != NULL && *ptarget == NULL);

	RUNTIME_CHECK(SSL_CTX_up_ref(src) == 1);

	*ptarget = src;
}

#if HAVE_SSL_CTX_SET_KEYLOG_CALLBACK
/*
 * Callback invoked by the SSL library whenever a new TLS pre-master secret
 * needs to be logged.
 */
static void
sslkeylogfile_append(const SSL *ssl, const char *line) {
	UNUSED(ssl);

	isc_log_write(isc_lctx, ISC_LOGCATEGORY_SSLKEYLOG, ISC_LOGMODULE_NETMGR,
		      ISC_LOG_INFO, "%s", line);
}

/*
 * Enable TLS pre-master secret logging if the SSLKEYLOGFILE environment
 * variable is set.  This needs to be done on a per-context basis as that is
 * how SSL_CTX_set_keylog_callback() works.
 */
static void
sslkeylogfile_init(isc_tlsctx_t *ctx) {
	if (getenv("SSLKEYLOGFILE") != NULL) {
		SSL_CTX_set_keylog_callback(ctx, sslkeylogfile_append);
	}
}
#else /* HAVE_SSL_CTX_SET_KEYLOG_CALLBACK */
#define sslkeylogfile_init(ctx)
#endif /* HAVE_SSL_CTX_SET_KEYLOG_CALLBACK */

isc_result_t
isc_tlsctx_createclient(isc_tlsctx_t **ctxp) {
	unsigned long err;
	char errbuf[256];
	SSL_CTX *ctx = NULL;
	const SSL_METHOD *method = NULL;

	REQUIRE(ctxp != NULL && *ctxp == NULL);

	method = TLS_client_method();
	if (method == NULL) {
		goto ssl_error;
	}
	ctx = SSL_CTX_new(method);
	if (ctx == NULL) {
		goto ssl_error;
	}

	SSL_CTX_set_options(ctx, COMMON_SSL_OPTIONS);

#if HAVE_SSL_CTX_SET_MIN_PROTO_VERSION
	SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
#else
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
					 SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#endif

	sslkeylogfile_init(ctx);

	*ctxp = ctx;

	return (ISC_R_SUCCESS);

ssl_error:
	err = ERR_get_error();
	ERR_error_string_n(err, errbuf, sizeof(errbuf));
	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      ISC_LOG_ERROR, "Error initializing TLS context: %s",
		      errbuf);

	return (ISC_R_TLSERROR);
}

isc_result_t
isc_tlsctx_load_certificate(isc_tlsctx_t *ctx, const char *keyfile,
			    const char *certfile) {
	int rv;
	REQUIRE(ctx != NULL);
	REQUIRE(keyfile != NULL);
	REQUIRE(certfile != NULL);

	rv = SSL_CTX_use_certificate_chain_file(ctx, certfile);
	if (rv != 1) {
		return (ISC_R_TLSERROR);
	}
	rv = SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM);
	if (rv != 1) {
		return (ISC_R_TLSERROR);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_tlsctx_createserver(const char *keyfile, const char *certfile,
			isc_tlsctx_t **ctxp) {
	int rv;
	unsigned long err;
	bool ephemeral = (keyfile == NULL && certfile == NULL);
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;
	SSL_CTX *ctx = NULL;
#if OPENSSL_VERSION_NUMBER < 0x30000000L
	EC_KEY *eckey = NULL;
#else
	EVP_PKEY_CTX *pkey_ctx = NULL;
	EVP_PKEY *params_pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L */
	char errbuf[256];
	const SSL_METHOD *method = NULL;

	REQUIRE(ctxp != NULL && *ctxp == NULL);
	REQUIRE((keyfile == NULL) == (certfile == NULL));

	method = TLS_server_method();
	if (method == NULL) {
		goto ssl_error;
	}
	ctx = SSL_CTX_new(method);
	if (ctx == NULL) {
		goto ssl_error;
	}
	RUNTIME_CHECK(ctx != NULL);

	SSL_CTX_set_options(ctx, COMMON_SSL_OPTIONS);

#if HAVE_SSL_CTX_SET_MIN_PROTO_VERSION
	SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
#else
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
					 SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#endif

	if (ephemeral) {
		const int group_nid = NID_X9_62_prime256v1;

#if OPENSSL_VERSION_NUMBER < 0x30000000L
		eckey = EC_KEY_new_by_curve_name(group_nid);
		if (eckey == NULL) {
			goto ssl_error;
		}

		/* Generate the key. */
		rv = EC_KEY_generate_key(eckey);
		if (rv != 1) {
			goto ssl_error;
		}
		pkey = EVP_PKEY_new();
		if (pkey == NULL) {
			goto ssl_error;
		}
		rv = EVP_PKEY_set1_EC_KEY(pkey, eckey);
		if (rv != 1) {
			goto ssl_error;
		}

		/* Use a named curve and uncompressed point conversion form. */
#if HAVE_EVP_PKEY_GET0_EC_KEY
		EC_KEY_set_asn1_flag(EVP_PKEY_get0_EC_KEY(pkey),
				     OPENSSL_EC_NAMED_CURVE);
		EC_KEY_set_conv_form(EVP_PKEY_get0_EC_KEY(pkey),
				     POINT_CONVERSION_UNCOMPRESSED);
#else
		EC_KEY_set_asn1_flag(pkey->pkey.ec, OPENSSL_EC_NAMED_CURVE);
		EC_KEY_set_conv_form(pkey->pkey.ec,
				     POINT_CONVERSION_UNCOMPRESSED);
#endif /* HAVE_EVP_PKEY_GET0_EC_KEY */

#if defined(SSL_CTX_set_ecdh_auto)
		/*
		 * Using this macro is required for older versions of OpenSSL to
		 * automatically enable ECDH support.
		 *
		 * On later versions this function is no longer needed and is
		 * deprecated.
		 */
		(void)SSL_CTX_set_ecdh_auto(ctx, 1);
#endif /* defined(SSL_CTX_set_ecdh_auto) */

		/* Cleanup */
		EC_KEY_free(eckey);
		eckey = NULL;
#else
		/* Generate the key's parameters. */
		pkey_ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL);
		if (pkey_ctx == NULL) {
			goto ssl_error;
		}
		rv = EVP_PKEY_paramgen_init(pkey_ctx);
		if (rv != 1) {
			goto ssl_error;
		}
		rv = EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pkey_ctx,
							    group_nid);
		if (rv != 1) {
			goto ssl_error;
		}
		rv = EVP_PKEY_paramgen(pkey_ctx, &params_pkey);
		if (rv != 1 || params_pkey == NULL) {
			goto ssl_error;
		}
		EVP_PKEY_CTX_free(pkey_ctx);

		/* Generate the key. */
		pkey_ctx = EVP_PKEY_CTX_new(params_pkey, NULL);
		if (pkey_ctx == NULL) {
			goto ssl_error;
		}
		rv = EVP_PKEY_keygen_init(pkey_ctx);
		if (rv != 1) {
			goto ssl_error;
		}
		rv = EVP_PKEY_keygen(pkey_ctx, &pkey);
		if (rv != 1 || pkey == NULL) {
			goto ssl_error;
		}

		/* Cleanup */
		EVP_PKEY_free(params_pkey);
		params_pkey = NULL;
		EVP_PKEY_CTX_free(pkey_ctx);
		pkey_ctx = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L */

		cert = X509_new();
		if (cert == NULL) {
			goto ssl_error;
		}

		ASN1_INTEGER_set(X509_get_serialNumber(cert),
				 (long)isc_random32());

		/*
		 * Set the "not before" property 5 minutes into the past to
		 * accommodate with some possible clock skew across systems.
		 */
#if OPENSSL_VERSION_NUMBER < 0x10101000L
		X509_gmtime_adj(X509_get_notBefore(cert), -300);
#else
		X509_gmtime_adj(X509_getm_notBefore(cert), -300);
#endif

		/*
		 * We set the vailidy for 10 years.
		 */
#if OPENSSL_VERSION_NUMBER < 0x10101000L
		X509_gmtime_adj(X509_get_notAfter(cert), 3650 * 24 * 3600);
#else
		X509_gmtime_adj(X509_getm_notAfter(cert), 3650 * 24 * 3600);
#endif

		X509_set_pubkey(cert, pkey);

		X509_NAME *name = X509_get_subject_name(cert);

		X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC,
					   (const unsigned char *)"AQ", -1, -1,
					   0);
		X509_NAME_add_entry_by_txt(
			name, "O", MBSTRING_ASC,
			(const unsigned char *)"BIND9 ephemeral "
					       "certificate",
			-1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
					   (const unsigned char *)"bind9.local",
					   -1, -1, 0);

		X509_set_issuer_name(cert, name);
		X509_sign(cert, pkey, EVP_sha256());
		rv = SSL_CTX_use_certificate(ctx, cert);
		if (rv != 1) {
			goto ssl_error;
		}
		rv = SSL_CTX_use_PrivateKey(ctx, pkey);
		if (rv != 1) {
			goto ssl_error;
		}

		X509_free(cert);
		EVP_PKEY_free(pkey);
	} else {
		isc_result_t result;
		result = isc_tlsctx_load_certificate(ctx, keyfile, certfile);
		if (result != ISC_R_SUCCESS) {
			goto ssl_error;
		}
	}

	sslkeylogfile_init(ctx);

	*ctxp = ctx;
	return (ISC_R_SUCCESS);

ssl_error:
	err = ERR_get_error();
	ERR_error_string_n(err, errbuf, sizeof(errbuf));
	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      ISC_LOG_ERROR, "Error initializing TLS context: %s",
		      errbuf);

	if (ctx != NULL) {
		SSL_CTX_free(ctx);
	}
	if (cert != NULL) {
		X509_free(cert);
	}
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
#if OPENSSL_VERSION_NUMBER < 0x30000000L
	if (eckey != NULL) {
		EC_KEY_free(eckey);
	}
#else
	if (params_pkey != NULL) {
		EVP_PKEY_free(params_pkey);
	}
	if (pkey_ctx != NULL) {
		EVP_PKEY_CTX_free(pkey_ctx);
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L */

	return (ISC_R_TLSERROR);
}

static long
get_tls_version_disable_bit(const isc_tls_protocol_version_t tls_ver) {
	long bit = 0;

	switch (tls_ver) {
	case ISC_TLS_PROTO_VER_1_2:
#ifdef SSL_OP_NO_TLSv1_2
		bit = SSL_OP_NO_TLSv1_2;
#else
		bit = 0;
#endif
		break;
	case ISC_TLS_PROTO_VER_1_3:
#ifdef SSL_OP_NO_TLSv1_3
		bit = SSL_OP_NO_TLSv1_3;
#else
		bit = 0;
#endif
		break;
	default:
		UNREACHABLE();
		break;
	};

	return (bit);
}

bool
isc_tls_protocol_supported(const isc_tls_protocol_version_t tls_ver) {
	return (get_tls_version_disable_bit(tls_ver) != 0);
}

isc_tls_protocol_version_t
isc_tls_protocol_name_to_version(const char *name) {
	REQUIRE(name != NULL);

	if (strcasecmp(name, "TLSv1.2") == 0) {
		return (ISC_TLS_PROTO_VER_1_2);
	} else if (strcasecmp(name, "TLSv1.3") == 0) {
		return (ISC_TLS_PROTO_VER_1_3);
	}

	return (ISC_TLS_PROTO_VER_UNDEFINED);
}

void
isc_tlsctx_set_protocols(isc_tlsctx_t *ctx, const uint32_t tls_versions) {
	REQUIRE(ctx != NULL);
	REQUIRE(tls_versions != 0);
	long set_options = 0;
	long clear_options = 0;
	uint32_t versions = tls_versions;

	/*
	 * The code below might be initially hard to follow because of the
	 * double negation that OpenSSL enforces.
	 *
	 * Taking into account that OpenSSL provides bits to *disable*
	 * specific protocol versions, like SSL_OP_NO_TLSv1_2,
	 * SSL_OP_NO_TLSv1_3, etc., the code has the following logic:
	 *
	 * If a protocol version is not specified in the bitmask, get the
	 * bit that disables it and add it to the set of TLS options to
	 * set ('set_options'). Otherwise, if a protocol version is set,
	 * add the bit to the set of options to clear ('clear_options').
	 */

	/* TLS protocol versions are defined as powers of two. */
	for (uint32_t tls_ver = ISC_TLS_PROTO_VER_1_2;
	     tls_ver < ISC_TLS_PROTO_VER_UNDEFINED; tls_ver <<= 1)
	{
		if ((tls_versions & tls_ver) == 0) {
			set_options |= get_tls_version_disable_bit(tls_ver);
		} else {
			/*
			 * Only supported versions should ever be passed to the
			 * function SSL_CTX_clear_options. For example, in order
			 * to enable TLS v1.2, we have to clear
			 * SSL_OP_NO_TLSv1_2. Insist that the configuration file
			 * was verified properly, so we are not trying to enable
			 * an unsupported TLS version.
			 */
			INSIST(isc_tls_protocol_supported(tls_ver));
			clear_options |= get_tls_version_disable_bit(tls_ver);
		}
		versions &= ~(tls_ver);
	}

	/* All versions should be processed at this point, thus the value
	 * must equal zero. If it is not, then some garbage has been
	 * passed to the function; this situation is worth
	 * investigation. */
	INSIST(versions == 0);

	(void)SSL_CTX_set_options(ctx, set_options);
	(void)SSL_CTX_clear_options(ctx, clear_options);
}

bool
isc_tlsctx_load_dhparams(isc_tlsctx_t *ctx, const char *dhparams_file) {
	REQUIRE(ctx != NULL);
	REQUIRE(dhparams_file != NULL);
	REQUIRE(*dhparams_file != '\0');

#if OPENSSL_VERSION_NUMBER < 0x30000000L
	/* OpenSSL < 3.0 */
	DH *dh = NULL;
	FILE *paramfile;

	paramfile = fopen(dhparams_file, "r");

	if (paramfile) {
		int check = 0;
		dh = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
		fclose(paramfile);

		if (dh == NULL) {
			return (false);
		} else if (DH_check(dh, &check) != 1 || check != 0) {
			DH_free(dh);
			return (false);
		}
	} else {
		return (false);
	}

	if (SSL_CTX_set_tmp_dh(ctx, dh) != 1) {
		DH_free(dh);
		return (false);
	}

	DH_free(dh);
#else
	/* OpenSSL >= 3.0: low level DH APIs are deprecated in OpenSSL 3.0 */
	EVP_PKEY *dh = NULL;
	BIO *bio = NULL;

	bio = BIO_new_file(dhparams_file, "r");
	if (bio == NULL) {
		return (false);
	}

	dh = PEM_read_bio_Parameters(bio, NULL);
	if (dh == NULL) {
		BIO_free(bio);
		return (false);
	}

	if (SSL_CTX_set0_tmp_dh_pkey(ctx, dh) != 1) {
		BIO_free(bio);
		EVP_PKEY_free(dh);
		return (false);
	}

	/* No need to call EVP_PKEY_free(dh) as the "dh" is owned by the
	 * SSL context at this point. */

	BIO_free(bio);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L */

	return (true);
}

bool
isc_tls_cipherlist_valid(const char *cipherlist) {
	isc_tlsctx_t *tmp_ctx = NULL;
	const SSL_METHOD *method = NULL;
	bool result;
	REQUIRE(cipherlist != NULL);

	if (*cipherlist == '\0') {
		return (false);
	}

	method = TLS_server_method();
	if (method == NULL) {
		return (false);
	}
	tmp_ctx = SSL_CTX_new(method);
	if (tmp_ctx == NULL) {
		return (false);
	}

	result = SSL_CTX_set_cipher_list(tmp_ctx, cipherlist) == 1;

	isc_tlsctx_free(&tmp_ctx);

	return (result);
}

void
isc_tlsctx_set_cipherlist(isc_tlsctx_t *ctx, const char *cipherlist) {
	REQUIRE(ctx != NULL);
	REQUIRE(cipherlist != NULL);
	REQUIRE(*cipherlist != '\0');

	RUNTIME_CHECK(SSL_CTX_set_cipher_list(ctx, cipherlist) == 1);
}

void
isc_tlsctx_prefer_server_ciphers(isc_tlsctx_t *ctx, const bool prefer) {
	REQUIRE(ctx != NULL);

	if (prefer) {
		(void)SSL_CTX_set_options(ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
	} else {
		(void)SSL_CTX_clear_options(ctx,
					    SSL_OP_CIPHER_SERVER_PREFERENCE);
	}
}

void
isc_tlsctx_session_tickets(isc_tlsctx_t *ctx, const bool use) {
	REQUIRE(ctx != NULL);

	if (!use) {
		(void)SSL_CTX_set_options(ctx, SSL_OP_NO_TICKET);
	} else {
		(void)SSL_CTX_clear_options(ctx, SSL_OP_NO_TICKET);
	}
}

isc_tls_t *
isc_tls_create(isc_tlsctx_t *ctx) {
	isc_tls_t *newctx = NULL;

	REQUIRE(ctx != NULL);

	newctx = SSL_new(ctx);
	if (newctx == NULL) {
		char errbuf[256];
		unsigned long err = ERR_get_error();

		ERR_error_string_n(err, errbuf, sizeof(errbuf));
		fprintf(stderr, "%s:SSL_new(%p) -> %s\n", __func__, ctx,
			errbuf);
	}

	return (newctx);
}

void
isc_tls_free(isc_tls_t **tlsp) {
	isc_tls_t *tls = NULL;
	REQUIRE(tlsp != NULL && *tlsp != NULL);

	tls = *tlsp;
	*tlsp = NULL;
	SSL_free(tls);
}

const char *
isc_tls_verify_peer_result_string(isc_tls_t *tls) {
	REQUIRE(tls != NULL);

	return (X509_verify_cert_error_string(SSL_get_verify_result(tls)));
}

#if HAVE_LIBNGHTTP2
#ifndef OPENSSL_NO_NEXTPROTONEG
/*
 * NPN TLS extension client callback.
 */
static int
select_next_proto_cb(SSL *ssl, unsigned char **out, unsigned char *outlen,
		     const unsigned char *in, unsigned int inlen, void *arg) {
	UNUSED(ssl);
	UNUSED(arg);

	if (nghttp2_select_next_protocol(out, outlen, in, inlen) <= 0) {
		return (SSL_TLSEXT_ERR_NOACK);
	}
	return (SSL_TLSEXT_ERR_OK);
}
#endif /* !OPENSSL_NO_NEXTPROTONEG */

void
isc_tlsctx_enable_http2client_alpn(isc_tlsctx_t *ctx) {
	REQUIRE(ctx != NULL);

#ifndef OPENSSL_NO_NEXTPROTONEG
	SSL_CTX_set_next_proto_select_cb(ctx, select_next_proto_cb, NULL);
#endif /* !OPENSSL_NO_NEXTPROTONEG */

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	SSL_CTX_set_alpn_protos(ctx, (const unsigned char *)NGHTTP2_PROTO_ALPN,
				NGHTTP2_PROTO_ALPN_LEN);
#endif /* OPENSSL_VERSION_NUMBER >= 0x10002000L */
}

#ifndef OPENSSL_NO_NEXTPROTONEG
static int
next_proto_cb(isc_tls_t *ssl, const unsigned char **data, unsigned int *len,
	      void *arg) {
	UNUSED(ssl);
	UNUSED(arg);

	*data = (const unsigned char *)NGHTTP2_PROTO_ALPN;
	*len = (unsigned int)NGHTTP2_PROTO_ALPN_LEN;
	return (SSL_TLSEXT_ERR_OK);
}
#endif /* !OPENSSL_NO_NEXTPROTONEG */

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
static int
alpn_select_proto_cb(SSL *ssl, const unsigned char **out, unsigned char *outlen,
		     const unsigned char *in, unsigned int inlen, void *arg) {
	int ret;

	UNUSED(ssl);
	UNUSED(arg);

	ret = nghttp2_select_next_protocol((unsigned char **)(uintptr_t)out,
					   outlen, in, inlen);

	if (ret != 1) {
		return (SSL_TLSEXT_ERR_NOACK);
	}

	return (SSL_TLSEXT_ERR_OK);
}
#endif /* OPENSSL_VERSION_NUMBER >= 0x10002000L */

void
isc_tlsctx_enable_http2server_alpn(isc_tlsctx_t *tls) {
	REQUIRE(tls != NULL);

#ifndef OPENSSL_NO_NEXTPROTONEG
	SSL_CTX_set_next_protos_advertised_cb(tls, next_proto_cb, NULL);
#endif // OPENSSL_NO_NEXTPROTONEG
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	SSL_CTX_set_alpn_select_cb(tls, alpn_select_proto_cb, NULL);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
}
#endif /* HAVE_LIBNGHTTP2 */

void
isc_tls_get_selected_alpn(isc_tls_t *tls, const unsigned char **alpn,
			  unsigned int *alpnlen) {
	REQUIRE(tls != NULL);
	REQUIRE(alpn != NULL);
	REQUIRE(alpnlen != NULL);

#ifndef OPENSSL_NO_NEXTPROTONEG
	SSL_get0_next_proto_negotiated(tls, alpn, alpnlen);
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	if (*alpn == NULL) {
		SSL_get0_alpn_selected(tls, alpn, alpnlen);
	}
#endif
}

static bool
protoneg_check_protocol(const uint8_t **pout, uint8_t *pout_len,
			const uint8_t *in, size_t in_len, const uint8_t *key,
			size_t key_len) {
	for (size_t i = 0; i + key_len <= in_len; i += (size_t)(in[i] + 1)) {
		if (memcmp(&in[i], key, key_len) == 0) {
			*pout = (const uint8_t *)(&in[i + 1]);
			*pout_len = in[i];
			return (true);
		}
	}
	return (false);
}

/* dot prepended by its length (3 bytes) */
#define DOT_PROTO_ALPN	   "\x3" ISC_TLS_DOT_PROTO_ALPN_ID
#define DOT_PROTO_ALPN_LEN (sizeof(DOT_PROTO_ALPN) - 1)

static bool
dot_select_next_protocol(const uint8_t **pout, uint8_t *pout_len,
			 const uint8_t *in, size_t in_len) {
	return (protoneg_check_protocol(pout, pout_len, in, in_len,
					(const uint8_t *)DOT_PROTO_ALPN,
					DOT_PROTO_ALPN_LEN));
}

void
isc_tlsctx_enable_dot_client_alpn(isc_tlsctx_t *ctx) {
	REQUIRE(ctx != NULL);

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	SSL_CTX_set_alpn_protos(ctx, (const uint8_t *)DOT_PROTO_ALPN,
				DOT_PROTO_ALPN_LEN);
#endif /* OPENSSL_VERSION_NUMBER >= 0x10002000L */
}

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
static int
dot_alpn_select_proto_cb(SSL *ssl, const unsigned char **out,
			 unsigned char *outlen, const unsigned char *in,
			 unsigned int inlen, void *arg) {
	bool ret;

	UNUSED(ssl);
	UNUSED(arg);

	ret = dot_select_next_protocol(out, outlen, in, inlen);

	if (!ret) {
		return (SSL_TLSEXT_ERR_NOACK);
	}

	return (SSL_TLSEXT_ERR_OK);
}
#endif /* OPENSSL_VERSION_NUMBER >= 0x10002000L */

void
isc_tlsctx_enable_dot_server_alpn(isc_tlsctx_t *tls) {
	REQUIRE(tls != NULL);

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	SSL_CTX_set_alpn_select_cb(tls, dot_alpn_select_proto_cb, NULL);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
}

isc_result_t
isc_tlsctx_enable_peer_verification(isc_tlsctx_t *tlsctx, const bool is_server,
				    isc_tls_cert_store_t *store,
				    const char *hostname,
				    bool hostname_ignore_subject) {
	int ret = 0;
	REQUIRE(tlsctx != NULL);
	REQUIRE(store != NULL);

	/* Set the hostname/IP address. */
	if (!is_server && hostname != NULL && *hostname != '\0') {
		struct in6_addr sa6;
		struct in_addr sa;
		X509_VERIFY_PARAM *param = SSL_CTX_get0_param(tlsctx);
		unsigned int hostflags = X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS;

		/* It might be an IP address. */
		if (inet_pton(AF_INET6, hostname, &sa6) == 1 ||
		    inet_pton(AF_INET, hostname, &sa) == 1)
		{
			ret = X509_VERIFY_PARAM_set1_ip_asc(param, hostname);
		} else {
			/* It seems that it is a host name. Let's set it. */
			ret = X509_VERIFY_PARAM_set1_host(param, hostname, 0);
		}
		if (ret != 1) {
			ERR_clear_error();
			return (ISC_R_FAILURE);
		}

#ifdef X509_CHECK_FLAG_NEVER_CHECK_SUBJECT
		/*
		 * According to the RFC 8310, Section 8.1, Subject field MUST
		 * NOT be inspected when verifying a hostname when using
		 * DoT. Only SubjectAltName must be checked instead. That is
		 * not the case for HTTPS, though.
		 *
		 * Unfortunately, some quite old versions of OpenSSL (< 1.1.1)
		 * might lack the functionality to implement that. It should
		 * have very little real-world consequences, as most of the
		 * production-ready certificates issued by real CAs will have
		 * SubjectAltName set. In such a case, the Subject field is
		 * ignored.
		 */
		if (hostname_ignore_subject) {
			hostflags |= X509_CHECK_FLAG_NEVER_CHECK_SUBJECT;
		}
#else
		UNUSED(hostname_ignore_subject);
#endif
		X509_VERIFY_PARAM_set_hostflags(param, hostflags);
	}

	/* "Attach" the cert store to the context */
	SSL_CTX_set1_cert_store(tlsctx, store);

	/* enable verification */
	if (is_server) {
		SSL_CTX_set_verify(tlsctx,
				   SSL_VERIFY_PEER |
					   SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
				   NULL);
	} else {
		SSL_CTX_set_verify(tlsctx, SSL_VERIFY_PEER, NULL);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_tlsctx_load_client_ca_names(isc_tlsctx_t *ctx, const char *ca_bundle_file) {
	STACK_OF(X509_NAME) * cert_names;
	REQUIRE(ctx != NULL);
	REQUIRE(ca_bundle_file != NULL);

	cert_names = SSL_load_client_CA_file(ca_bundle_file);
	if (cert_names == NULL) {
		ERR_clear_error();
		return (ISC_R_FAILURE);
	}

	SSL_CTX_set_client_CA_list(ctx, cert_names);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_tls_cert_store_create(const char *ca_bundle_filename,
			  isc_tls_cert_store_t **pstore) {
	int ret = 0;
	isc_tls_cert_store_t *store = NULL;
	REQUIRE(pstore != NULL && *pstore == NULL);

	store = X509_STORE_new();
	if (store == NULL) {
		goto error;
	}

	/* Let's treat empty string as the default (system wide) store */
	if (ca_bundle_filename != NULL && *ca_bundle_filename == '\0') {
		ca_bundle_filename = NULL;
	}

	if (ca_bundle_filename == NULL) {
		ret = X509_STORE_set_default_paths(store);
	} else {
		ret = X509_STORE_load_locations(store, ca_bundle_filename,
						NULL);
	}

	if (ret == 0) {
		goto error;
	}

	*pstore = store;
	return (ISC_R_SUCCESS);

error:
	ERR_clear_error();
	if (store != NULL) {
		X509_STORE_free(store);
	}
	return (ISC_R_FAILURE);
}

void
isc_tls_cert_store_free(isc_tls_cert_store_t **pstore) {
	isc_tls_cert_store_t *store;
	REQUIRE(pstore != NULL && *pstore != NULL);

	store = *pstore;

	X509_STORE_free(store);

	*pstore = NULL;
}

#define TLSCTX_CACHE_MAGIC    ISC_MAGIC('T', 'l', 'S', 'c')
#define VALID_TLSCTX_CACHE(t) ISC_MAGIC_VALID(t, TLSCTX_CACHE_MAGIC)

#define TLSCTX_CLIENT_SESSION_CACHE_MAGIC ISC_MAGIC('T', 'l', 'C', 'c')
#define VALID_TLSCTX_CLIENT_SESSION_CACHE(t) \
	ISC_MAGIC_VALID(t, TLSCTX_CLIENT_SESSION_CACHE_MAGIC)

typedef struct isc_tlsctx_cache_entry {
	/*
	 * We need a TLS context entry for each transport on both IPv4 and
	 * IPv6 in order to avoid cluttering a context-specific
	 * session-resumption cache.
	 */
	isc_tlsctx_t *ctx[isc_tlsctx_cache_count - 1][2];
	isc_tlsctx_client_session_cache_t
		*client_sess_cache[isc_tlsctx_cache_count - 1][2];
	/*
	 * One certificate store is enough for all the contexts defined
	 * above. We need that for peer validation.
	 */
	isc_tls_cert_store_t *ca_store;
} isc_tlsctx_cache_entry_t;

struct isc_tlsctx_cache {
	uint32_t magic;
	isc_refcount_t references;
	isc_mem_t *mctx;

	isc_rwlock_t rwlock;
	isc_ht_t *data;
};

void
isc_tlsctx_cache_create(isc_mem_t *mctx, isc_tlsctx_cache_t **cachep) {
	isc_tlsctx_cache_t *nc;

	REQUIRE(cachep != NULL && *cachep == NULL);
	nc = isc_mem_get(mctx, sizeof(*nc));

	*nc = (isc_tlsctx_cache_t){ .magic = TLSCTX_CACHE_MAGIC };
	isc_refcount_init(&nc->references, 1);
	isc_mem_attach(mctx, &nc->mctx);

	isc_ht_init(&nc->data, mctx, 5, ISC_HT_CASE_SENSITIVE);
	isc_rwlock_init(&nc->rwlock, 0, 0);

	*cachep = nc;
}

void
isc_tlsctx_cache_attach(isc_tlsctx_cache_t *source,
			isc_tlsctx_cache_t **targetp) {
	REQUIRE(VALID_TLSCTX_CACHE(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static void
tlsctx_cache_entry_destroy(isc_mem_t *mctx, isc_tlsctx_cache_entry_t *entry) {
	size_t i, k;

	for (i = 0; i < (isc_tlsctx_cache_count - 1); i++) {
		for (k = 0; k < 2; k++) {
			if (entry->ctx[i][k] != NULL) {
				isc_tlsctx_free(&entry->ctx[i][k]);
			}

			if (entry->client_sess_cache[i][k] != NULL) {
				isc_tlsctx_client_session_cache_detach(
					&entry->client_sess_cache[i][k]);
			}
		}
	}
	if (entry->ca_store != NULL) {
		isc_tls_cert_store_free(&entry->ca_store);
	}
	isc_mem_put(mctx, entry, sizeof(*entry));
}

static void
tlsctx_cache_destroy(isc_tlsctx_cache_t *cache) {
	isc_ht_iter_t *it = NULL;
	isc_result_t result;

	cache->magic = 0;

	isc_refcount_destroy(&cache->references);

	isc_ht_iter_create(cache->data, &it);
	for (result = isc_ht_iter_first(it); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_delcurrent_next(it))
	{
		isc_tlsctx_cache_entry_t *entry = NULL;
		isc_ht_iter_current(it, (void **)&entry);
		tlsctx_cache_entry_destroy(cache->mctx, entry);
	}

	isc_ht_iter_destroy(&it);
	isc_ht_destroy(&cache->data);
	isc_rwlock_destroy(&cache->rwlock);
	isc_mem_putanddetach(&cache->mctx, cache, sizeof(*cache));
}

void
isc_tlsctx_cache_detach(isc_tlsctx_cache_t **cachep) {
	isc_tlsctx_cache_t *cache = NULL;

	REQUIRE(cachep != NULL);

	cache = *cachep;
	*cachep = NULL;

	REQUIRE(VALID_TLSCTX_CACHE(cache));

	if (isc_refcount_decrement(&cache->references) == 1) {
		tlsctx_cache_destroy(cache);
	}
}

isc_result_t
isc_tlsctx_cache_add(
	isc_tlsctx_cache_t *cache, const char *name,
	const isc_tlsctx_cache_transport_t transport, const uint16_t family,
	isc_tlsctx_t *ctx, isc_tls_cert_store_t *store,
	isc_tlsctx_client_session_cache_t *client_sess_cache,
	isc_tlsctx_t **pfound, isc_tls_cert_store_t **pfound_store,
	isc_tlsctx_client_session_cache_t **pfound_client_sess_cache) {
	isc_result_t result = ISC_R_FAILURE;
	size_t name_len, tr_offset;
	isc_tlsctx_cache_entry_t *entry = NULL;
	bool ipv6;

	REQUIRE(VALID_TLSCTX_CACHE(cache));
	REQUIRE(client_sess_cache == NULL ||
		VALID_TLSCTX_CLIENT_SESSION_CACHE(client_sess_cache));
	REQUIRE(name != NULL && *name != '\0');
	REQUIRE(transport > isc_tlsctx_cache_none &&
		transport < isc_tlsctx_cache_count);
	REQUIRE(family == AF_INET || family == AF_INET6);
	REQUIRE(ctx != NULL);

	tr_offset = (transport - 1);
	ipv6 = (family == AF_INET6);

	RWLOCK(&cache->rwlock, isc_rwlocktype_write);

	name_len = strlen(name);
	result = isc_ht_find(cache->data, (const uint8_t *)name, name_len,
			     (void **)&entry);
	if (result == ISC_R_SUCCESS && entry->ctx[tr_offset][ipv6] != NULL) {
		isc_tlsctx_client_session_cache_t *found_client_sess_cache;
		/* The entry exists. */
		if (pfound != NULL) {
			INSIST(*pfound == NULL);
			*pfound = entry->ctx[tr_offset][ipv6];
		}

		if (pfound_store != NULL && entry->ca_store != NULL) {
			INSIST(*pfound_store == NULL);
			*pfound_store = entry->ca_store;
		}

		found_client_sess_cache =
			entry->client_sess_cache[tr_offset][ipv6];
		if (pfound_client_sess_cache != NULL &&
		    found_client_sess_cache != NULL)
		{
			INSIST(*pfound_client_sess_cache == NULL);
			*pfound_client_sess_cache = found_client_sess_cache;
		}
		result = ISC_R_EXISTS;
	} else if (result == ISC_R_SUCCESS &&
		   entry->ctx[tr_offset][ipv6] == NULL)
	{
		/*
		 * The hash table entry exists, but is not filled for this
		 * particular transport/IP type combination.
		 */
		entry->ctx[tr_offset][ipv6] = ctx;
		entry->client_sess_cache[tr_offset][ipv6] = client_sess_cache;
		/*
		 * As the passed certificates store object is supposed
		 * to be internally managed by the cache object anyway,
		 * we might destroy the unneeded store object right now.
		 */
		if (store != NULL && store != entry->ca_store) {
			isc_tls_cert_store_free(&store);
		}
		result = ISC_R_SUCCESS;
	} else {
		/*
		 * The hash table entry does not exist, let's create one.
		 */
		INSIST(result != ISC_R_SUCCESS);
		entry = isc_mem_get(cache->mctx, sizeof(*entry));
		/* Oracle/Red Hat Linux, GCC bug #53119 */
		memset(entry, 0, sizeof(*entry));
		entry->ctx[tr_offset][ipv6] = ctx;
		entry->client_sess_cache[tr_offset][ipv6] = client_sess_cache;
		entry->ca_store = store;
		RUNTIME_CHECK(isc_ht_add(cache->data, (const uint8_t *)name,
					 name_len,
					 (void *)entry) == ISC_R_SUCCESS);
		result = ISC_R_SUCCESS;
	}

	RWUNLOCK(&cache->rwlock, isc_rwlocktype_write);

	return (result);
}

isc_result_t
isc_tlsctx_cache_find(
	isc_tlsctx_cache_t *cache, const char *name,
	const isc_tlsctx_cache_transport_t transport, const uint16_t family,
	isc_tlsctx_t **pctx, isc_tls_cert_store_t **pstore,
	isc_tlsctx_client_session_cache_t **pfound_client_sess_cache) {
	isc_result_t result = ISC_R_FAILURE;
	size_t tr_offset;
	isc_tlsctx_cache_entry_t *entry = NULL;
	bool ipv6;

	REQUIRE(VALID_TLSCTX_CACHE(cache));
	REQUIRE(name != NULL && *name != '\0');
	REQUIRE(transport > isc_tlsctx_cache_none &&
		transport < isc_tlsctx_cache_count);
	REQUIRE(family == AF_INET || family == AF_INET6);
	REQUIRE(pctx != NULL && *pctx == NULL);

	tr_offset = (transport - 1);
	ipv6 = (family == AF_INET6);

	RWLOCK(&cache->rwlock, isc_rwlocktype_read);

	result = isc_ht_find(cache->data, (const uint8_t *)name, strlen(name),
			     (void **)&entry);

	if (result == ISC_R_SUCCESS && pstore != NULL &&
	    entry->ca_store != NULL)
	{
		*pstore = entry->ca_store;
	}

	if (result == ISC_R_SUCCESS && entry->ctx[tr_offset][ipv6] != NULL) {
		isc_tlsctx_client_session_cache_t *found_client_sess_cache =
			entry->client_sess_cache[tr_offset][ipv6];

		*pctx = entry->ctx[tr_offset][ipv6];

		if (pfound_client_sess_cache != NULL &&
		    found_client_sess_cache != NULL)
		{
			INSIST(*pfound_client_sess_cache == NULL);
			*pfound_client_sess_cache = found_client_sess_cache;
		}
	} else if (result == ISC_R_SUCCESS &&
		   entry->ctx[tr_offset][ipv6] == NULL)
	{
		result = ISC_R_NOTFOUND;
	} else {
		INSIST(result != ISC_R_SUCCESS);
	}

	RWUNLOCK(&cache->rwlock, isc_rwlocktype_read);

	return (result);
}

typedef struct client_session_cache_entry client_session_cache_entry_t;

typedef struct client_session_cache_bucket {
	char *bucket_key;
	size_t bucket_key_len;
	/* Cache entries within the bucket (from the oldest to the newest). */
	ISC_LIST(client_session_cache_entry_t) entries;
} client_session_cache_bucket_t;

struct client_session_cache_entry {
	SSL_SESSION *session;
	client_session_cache_bucket_t *bucket; /* "Parent" bucket pointer. */
	ISC_LINK(client_session_cache_entry_t) bucket_link;
	ISC_LINK(client_session_cache_entry_t) cache_link;
};

struct isc_tlsctx_client_session_cache {
	uint32_t magic;
	isc_refcount_t references;
	isc_mem_t *mctx;

	/*
	 * We need to keep a reference to the related TLS context in order
	 * to ensure that it remains valid while the TLS client sessions
	 * cache object is valid, as every TLS session object
	 * (SSL_SESSION) is "tied" to a particular context.
	 */
	isc_tlsctx_t *ctx;

	/*
	 * The idea is to have one bucket per remote server. Each bucket,
	 * can maintain multiple TLS sessions to that server, as BIND
	 * might want to establish multiple TLS connections to the remote
	 * server at once.
	 */
	isc_ht_t *buckets;

	/*
	 * The list of all current entries within the cache maintained in
	 * LRU-manner, so that the oldest entry might be efficiently
	 * removed.
	 */
	ISC_LIST(client_session_cache_entry_t) lru_entries;
	/* Number of the entries within the cache. */
	size_t nentries;
	/* Maximum number of the entries within the cache. */
	size_t max_entries;

	isc_mutex_t lock;
};

void
isc_tlsctx_client_session_cache_create(
	isc_mem_t *mctx, isc_tlsctx_t *ctx, const size_t max_entries,
	isc_tlsctx_client_session_cache_t **cachep) {
	isc_tlsctx_client_session_cache_t *nc;

	REQUIRE(ctx != NULL);
	REQUIRE(max_entries > 0);
	REQUIRE(cachep != NULL && *cachep == NULL);

	nc = isc_mem_get(mctx, sizeof(*nc));

	*nc = (isc_tlsctx_client_session_cache_t){ .max_entries = max_entries };
	isc_refcount_init(&nc->references, 1);
	isc_mem_attach(mctx, &nc->mctx);
	isc_tlsctx_attach(ctx, &nc->ctx);

	isc_ht_init(&nc->buckets, mctx, 5, ISC_HT_CASE_SENSITIVE);
	ISC_LIST_INIT(nc->lru_entries);
	isc_mutex_init(&nc->lock);

	nc->magic = TLSCTX_CLIENT_SESSION_CACHE_MAGIC;

	*cachep = nc;
}

void
isc_tlsctx_client_session_cache_attach(
	isc_tlsctx_client_session_cache_t *source,
	isc_tlsctx_client_session_cache_t **targetp) {
	REQUIRE(VALID_TLSCTX_CLIENT_SESSION_CACHE(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static void
client_cache_entry_delete(isc_tlsctx_client_session_cache_t *restrict cache,
			  client_session_cache_entry_t *restrict entry) {
	client_session_cache_bucket_t *restrict bucket = entry->bucket;

	/* Unlink and free the cache entry */
	ISC_LIST_UNLINK(bucket->entries, entry, bucket_link);
	ISC_LIST_UNLINK(cache->lru_entries, entry, cache_link);
	cache->nentries--;
	(void)SSL_SESSION_free(entry->session);
	isc_mem_put(cache->mctx, entry, sizeof(*entry));

	/* The bucket is empty - let's remove it */
	if (ISC_LIST_EMPTY(bucket->entries)) {
		RUNTIME_CHECK(isc_ht_delete(cache->buckets,
					    (const uint8_t *)bucket->bucket_key,
					    bucket->bucket_key_len) ==
			      ISC_R_SUCCESS);

		isc_mem_free(cache->mctx, bucket->bucket_key);
		isc_mem_put(cache->mctx, bucket, sizeof(*bucket));
	}
}

void
isc_tlsctx_client_session_cache_detach(
	isc_tlsctx_client_session_cache_t **cachep) {
	isc_tlsctx_client_session_cache_t *cache = NULL;
	client_session_cache_entry_t *entry = NULL, *next = NULL;

	REQUIRE(cachep != NULL);

	cache = *cachep;
	*cachep = NULL;

	REQUIRE(VALID_TLSCTX_CLIENT_SESSION_CACHE(cache));

	if (isc_refcount_decrement(&cache->references) != 1) {
		return;
	}

	cache->magic = 0;

	isc_refcount_destroy(&cache->references);

	entry = ISC_LIST_HEAD(cache->lru_entries);
	while (entry != NULL) {
		next = ISC_LIST_NEXT(entry, cache_link);
		client_cache_entry_delete(cache, entry);
		entry = next;
	}

	RUNTIME_CHECK(isc_ht_count(cache->buckets) == 0);
	isc_ht_destroy(&cache->buckets);

	isc_mutex_destroy(&cache->lock);
	isc_tlsctx_free(&cache->ctx);
	isc_mem_putanddetach(&cache->mctx, cache, sizeof(*cache));
}

static bool
ssl_session_seems_resumable(const SSL_SESSION *sess) {
#ifdef HAVE_SSL_SESSION_IS_RESUMABLE
	/*
	 * If SSL_SESSION_is_resumable() is available, let's use that. It
	 * is expected to be available on OpenSSL >= 1.1.1 and its modern
	 * siblings.
	 */
	return (SSL_SESSION_is_resumable(sess) != 0);
#elif (OPENSSL_VERSION_NUMBER >= 0x10100000L)
	/*
	 * Taking into consideration that OpenSSL 1.1.0 uses opaque
	 * pointers for SSL_SESSION, we cannot implement a replacement for
	 * SSL_SESSION_is_resumable() manually. Let's use a sensible
	 * approximation for that, then: if there is an associated session
	 * ticket or session ID, then, most likely, the session is
	 * resumable.
	 */
	unsigned int session_id_len = 0;
	(void)SSL_SESSION_get_id(sess, &session_id_len);
	return (SSL_SESSION_has_ticket(sess) || session_id_len > 0);
#else
	return (!sess->not_resumable &&
		(sess->session_id_length > 0 || sess->tlsext_ticklen > 0));
#endif
}

void
isc_tlsctx_client_session_cache_keep(isc_tlsctx_client_session_cache_t *cache,
				     char *remote_peer_name, isc_tls_t *tls) {
	size_t name_len;
	isc_result_t result;
	SSL_SESSION *sess;
	client_session_cache_bucket_t *restrict bucket = NULL;
	client_session_cache_entry_t *restrict entry = NULL;

	REQUIRE(VALID_TLSCTX_CLIENT_SESSION_CACHE(cache));
	REQUIRE(remote_peer_name != NULL && *remote_peer_name != '\0');
	REQUIRE(tls != NULL);

	sess = SSL_get1_session(tls);
	if (sess == NULL) {
		ERR_clear_error();
		return;
	} else if (!ssl_session_seems_resumable(sess)) {
		SSL_SESSION_free(sess);
		return;
	}

	isc_mutex_lock(&cache->lock);

	name_len = strlen(remote_peer_name);
	result = isc_ht_find(cache->buckets, (const uint8_t *)remote_peer_name,
			     name_len, (void **)&bucket);

	if (result != ISC_R_SUCCESS) {
		/* Let's create a new bucket */
		INSIST(bucket == NULL);
		bucket = isc_mem_get(cache->mctx, sizeof(*bucket));
		*bucket = (client_session_cache_bucket_t){
			.bucket_key = isc_mem_strdup(cache->mctx,
						     remote_peer_name),
			.bucket_key_len = name_len
		};
		ISC_LIST_INIT(bucket->entries);
		RUNTIME_CHECK(isc_ht_add(cache->buckets,
					 (const uint8_t *)remote_peer_name,
					 name_len,
					 (void *)bucket) == ISC_R_SUCCESS);
	}

	/* Let's add a new cache entry to the new/found bucket */
	entry = isc_mem_get(cache->mctx, sizeof(*entry));
	*entry = (client_session_cache_entry_t){ .session = sess,
						 .bucket = bucket };
	ISC_LINK_INIT(entry, bucket_link);
	ISC_LINK_INIT(entry, cache_link);

	ISC_LIST_APPEND(bucket->entries, entry, bucket_link);

	ISC_LIST_APPEND(cache->lru_entries, entry, cache_link);
	cache->nentries++;

	if (cache->nentries > cache->max_entries) {
		/*
		 * Cache overrun. We need to remove the oldest entry from the
		 * cache
		 */
		client_session_cache_entry_t *restrict oldest;
		INSIST((cache->nentries - 1) == cache->max_entries);

		oldest = ISC_LIST_HEAD(cache->lru_entries);
		client_cache_entry_delete(cache, oldest);
	}

	isc_mutex_unlock(&cache->lock);
}

void
isc_tlsctx_client_session_cache_reuse(isc_tlsctx_client_session_cache_t *cache,
				      char *remote_peer_name, isc_tls_t *tls) {
	client_session_cache_bucket_t *restrict bucket = NULL;
	client_session_cache_entry_t *restrict entry;
	size_t name_len;
	isc_result_t result;

	REQUIRE(VALID_TLSCTX_CLIENT_SESSION_CACHE(cache));
	REQUIRE(remote_peer_name != NULL && *remote_peer_name != '\0');
	REQUIRE(tls != NULL);

	isc_mutex_lock(&cache->lock);

	/* Let's find the bucket */
	name_len = strlen(remote_peer_name);
	result = isc_ht_find(cache->buckets, (const uint8_t *)remote_peer_name,
			     name_len, (void **)&bucket);

	if (result != ISC_R_SUCCESS) {
		goto exit;
	}

	INSIST(bucket != NULL);

	/*
	 * If the bucket has been found, let's use the newest session from
	 * the bucket, as it has the highest chance to be successfully
	 * resumed.
	 */
	INSIST(!ISC_LIST_EMPTY(bucket->entries));
	entry = ISC_LIST_TAIL(bucket->entries);
	RUNTIME_CHECK(SSL_set_session(tls, entry->session) == 1);
	client_cache_entry_delete(cache, entry);

exit:
	isc_mutex_unlock(&cache->lock);
}

void
isc_tlsctx_client_session_cache_keep_sockaddr(
	isc_tlsctx_client_session_cache_t *cache, isc_sockaddr_t *remote_peer,
	isc_tls_t *tls) {
	char peername[ISC_SOCKADDR_FORMATSIZE] = { 0 };

	REQUIRE(remote_peer != NULL);

	isc_sockaddr_format(remote_peer, peername, sizeof(peername));

	isc_tlsctx_client_session_cache_keep(cache, peername, tls);
}

void
isc_tlsctx_client_session_cache_reuse_sockaddr(
	isc_tlsctx_client_session_cache_t *cache, isc_sockaddr_t *remote_peer,
	isc_tls_t *tls) {
	char peername[ISC_SOCKADDR_FORMATSIZE] = { 0 };

	REQUIRE(remote_peer != NULL);

	isc_sockaddr_format(remote_peer, peername, sizeof(peername));

	isc_tlsctx_client_session_cache_reuse(cache, peername, tls);
}

const isc_tlsctx_t *
isc_tlsctx_client_session_cache_getctx(
	isc_tlsctx_client_session_cache_t *cache) {
	REQUIRE(VALID_TLSCTX_CLIENT_SESSION_CACHE(cache));
	return (cache->ctx);
}

void
isc_tlsctx_set_random_session_id_context(isc_tlsctx_t *ctx) {
	uint8_t session_id_ctx[SSL_MAX_SID_CTX_LENGTH] = { 0 };
	const size_t len = ISC_MIN(20, sizeof(session_id_ctx));

	REQUIRE(ctx != NULL);

	RUNTIME_CHECK(RAND_bytes(session_id_ctx, len) == 1);

	RUNTIME_CHECK(
		SSL_CTX_set_session_id_context(ctx, session_id_ctx, len) == 1);
}
