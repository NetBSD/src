/*	$NetBSD: tls.c,v 1.1.1.1 2021/04/29 16:46:32 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <openssl/bn.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include <isc/atomic.h>
#include <isc/log.h>
#include <isc/mutex.h>
#include <isc/mutexblock.h>
#include <isc/once.h>
#include <isc/thread.h>
#include <isc/tls.h>
#include <isc/util.h>

#include "openssl_shim.h"
#include "tls_p.h"

static isc_once_t init_once = ISC_ONCE_INIT;
static isc_once_t shut_once = ISC_ONCE_INIT;
static atomic_bool init_done = ATOMIC_VAR_INIT(false);
static atomic_bool shut_done = ATOMIC_VAR_INIT(false);

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
	 * later and ISC_MEM_DEBUGSIZE or ISC_MEM_DEBUGCTX flags are
	 * added, neither isc_mem_put() nor isc_mem_free() can be used
	 * to free up the memory allocated here because the flags were
	 * not set when calling isc_mem_get() or isc_mem_allocate()
	 * here.
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

#if !defined(OPENSSL_NO_ENGINE)
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
		FATAL_ERROR(__FILE__, __LINE__,
			    "OpenSSL pseudorandom number generator "
			    "cannot be initialized (see the `PRNG not "
			    "seeded' message in the OpenSSL FAQ)");
	}

	REQUIRE(atomic_compare_exchange_strong(&init_done, &(bool){ false },
					       true));
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

#if OPENSSL_VERSION_NUMBER < 0x10100000L

	CONF_modules_unload(1);
	OBJ_cleanup();
	EVP_cleanup();
#if !defined(OPENSSL_NO_ENGINE)
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

	REQUIRE(atomic_compare_exchange_strong(&shut_done, &(bool){ false },
					       true));
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

#if HAVE_SSL_CTX_SET_MIN_PROTO_VERSION
	SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
#else
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
					 SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#endif

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
isc_tlsctx_createserver(const char *keyfile, const char *certfile,
			isc_tlsctx_t **ctxp) {
	int rv;
	unsigned long err;
	bool ephemeral = (keyfile == NULL && certfile == NULL);
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;
	BIGNUM *bn = NULL;
	SSL_CTX *ctx = NULL;
	RSA *rsa = NULL;
	char errbuf[256];
	const SSL_METHOD *method = NULL;

	REQUIRE(ctxp != NULL && *ctxp == NULL);

	if (ephemeral) {
		INSIST(keyfile == NULL);
		INSIST(certfile == NULL);
	} else {
		INSIST(keyfile != NULL);
		INSIST(certfile != NULL);
	}

	method = TLS_server_method();
	if (method == NULL) {
		goto ssl_error;
	}
	ctx = SSL_CTX_new(method);
	if (ctx == NULL) {
		goto ssl_error;
	}
	RUNTIME_CHECK(ctx != NULL);

#if HAVE_SSL_CTX_SET_MIN_PROTO_VERSION
	SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
#else
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
					 SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#endif

	if (ephemeral) {
		rsa = RSA_new();
		if (rsa == NULL) {
			goto ssl_error;
		}
		bn = BN_new();
		if (bn == NULL) {
			goto ssl_error;
		}
		BN_set_word(bn, RSA_F4);
		rv = RSA_generate_key_ex(rsa, 4096, bn, NULL);
		if (rv != 1) {
			goto ssl_error;
		}
		cert = X509_new();
		if (cert == NULL) {
			goto ssl_error;
		}
		pkey = EVP_PKEY_new();
		if (pkey == NULL) {
			goto ssl_error;
		}

		/*
		 * EVP_PKEY_assign_*() set the referenced key to key
		 * however these use the supplied key internally and so
		 * key will be freed when the parent pkey is freed.
		 */
		EVP_PKEY_assign(pkey, EVP_PKEY_RSA, rsa);
		rsa = NULL;
		ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);

#if OPENSSL_VERSION_NUMBER < 0x10101000L
		X509_gmtime_adj(X509_get_notBefore(cert), 0);
#else
		X509_gmtime_adj(X509_getm_notBefore(cert), 0);
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
		BN_free(bn);
	} else {
		rv = SSL_CTX_use_certificate_file(ctx, certfile,
						  SSL_FILETYPE_PEM);
		if (rv != 1) {
			goto ssl_error;
		}
		rv = SSL_CTX_use_PrivateKey_file(ctx, keyfile,
						 SSL_FILETYPE_PEM);
		if (rv != 1) {
			goto ssl_error;
		}
	}

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
	if (bn != NULL) {
		BN_free(bn);
	}
	if (rsa != NULL) {
		RSA_free(rsa);
	}

	return (ISC_R_TLSERROR);
}
