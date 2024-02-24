/*	$NetBSD: tls.c,v 1.1.2.2 2024/02/24 13:07:22 martin Exp $	*/

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

#include <openssl/bn.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>
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
#include <isc/util.h>

#include "openssl_shim.h"
#include "tls_p.h"

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

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	OPENSSL_cleanup();
#else
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
