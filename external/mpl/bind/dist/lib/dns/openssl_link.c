/*	$NetBSD: openssl_link.c,v 1.4 2020/05/24 19:46:23 christos Exp $	*/

/*
 * Portions Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 *
 * Portions Copyright (C) Network Associates, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/mutexblock.h>
#include <isc/platform.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/log.h>

#include <dst/result.h>

#include "dst_internal.h"
#include "dst_openssl.h"

static isc_mem_t *dst__mctx = NULL;

#if !defined(OPENSSL_NO_ENGINE)
#include <openssl/engine.h>
#endif /* if !defined(OPENSSL_NO_ENGINE) */

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
static isc_mutex_t *locks = NULL;
static int nlocks;
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */

#if !defined(OPENSSL_NO_ENGINE)
static ENGINE *e = NULL;
#endif /* if !defined(OPENSSL_NO_ENGINE) */

static void
enable_fips_mode(void) {
#ifdef HAVE_FIPS_MODE
	if (FIPS_mode() != 0) {
		/*
		 * FIPS mode is already enabled.
		 */
		return;
	}

	if (FIPS_mode_set(1) == 0) {
		dst__openssl_toresult2("FIPS_mode_set", DST_R_OPENSSLFAILURE);
		exit(1);
	}
#endif /* HAVE_FIPS_MODE */
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
static void
lock_callback(int mode, int type, const char *file, int line) {
	UNUSED(file);
	UNUSED(line);
	if ((mode & CRYPTO_LOCK) != 0) {
		LOCK(&locks[type]);
	} else {
		UNLOCK(&locks[type]);
	}
}
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */

#if defined(LIBRESSL_VERSION_NUMBER)
static unsigned long
id_callback(void) {
	return ((unsigned long)isc_thread_self());
}
#endif /* if defined(LIBRESSL_VERSION_NUMBER) */

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void
_set_thread_id(CRYPTO_THREADID *id) {
	CRYPTO_THREADID_set_numeric(id, (unsigned long)isc_thread_self());
}
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L */

isc_result_t
dst__openssl_init(isc_mem_t *mctx, const char *engine) {
	isc_result_t result;

	REQUIRE(dst__mctx == NULL);
	isc_mem_attach(mctx, &dst__mctx);

#if defined(OPENSSL_NO_ENGINE)
	UNUSED(engine);
#endif /* if defined(OPENSSL_NO_ENGINE) */

	enable_fips_mode();

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	nlocks = CRYPTO_num_locks();
	locks = isc_mem_allocate(dst__mctx, sizeof(isc_mutex_t) * nlocks);
	isc_mutexblock_init(locks, nlocks);
	CRYPTO_set_locking_callback(lock_callback);
#if defined(LIBRESSL_VERSION_NUMBER)
	CRYPTO_set_id_callback(id_callback);
#elif OPENSSL_VERSION_NUMBER < 0x10100000L
	CRYPTO_THREADID_set_callback(_set_thread_id);
#endif /* if defined(LIBRESSL_VERSION_NUMBER) */
	ERR_load_crypto_strings();
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */

#if !defined(OPENSSL_NO_ENGINE)
#if !defined(CONF_MFLAGS_DEFAULT_SECTION)
	OPENSSL_config(NULL);
#else  /* if !defined(CONF_MFLAGS_DEFAULT_SECTION) */
	/*
	 * OPENSSL_config() can only be called a single time as of
	 * 1.0.2e so do the steps individually.
	 */
	OPENSSL_load_builtin_modules();
	ENGINE_load_builtin_engines();
	ERR_clear_error();
	CONF_modules_load_file(NULL, NULL,
			       CONF_MFLAGS_DEFAULT_SECTION |
				       CONF_MFLAGS_IGNORE_MISSING_FILE);
#endif /* if !defined(CONF_MFLAGS_DEFAULT_SECTION) */

	if (engine != NULL && *engine == '\0') {
		engine = NULL;
	}

	if (engine != NULL) {
		e = ENGINE_by_id(engine);
		if (e == NULL) {
			result = DST_R_NOENGINE;
			goto cleanup_rm;
		}
		/* This will init the engine. */
		if (!ENGINE_set_default(e, ENGINE_METHOD_ALL)) {
			result = DST_R_NOENGINE;
			goto cleanup_rm;
		}
	}

#endif /* !defined(OPENSSL_NO_ENGINE) */

	/* Protect ourselves against unseeded PRNG */
	if (RAND_status() != 1) {
		FATAL_ERROR(__FILE__, __LINE__,
			    "OpenSSL pseudorandom number generator "
			    "cannot be initialized (see the `PRNG not "
			    "seeded' message in the OpenSSL FAQ)");
	}

	return (ISC_R_SUCCESS);

#if !defined(OPENSSL_NO_ENGINE)
cleanup_rm:
	if (e != NULL) {
		ENGINE_free(e);
	}
	e = NULL;
#endif /* if !defined(OPENSSL_NO_ENGINE) */
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	CRYPTO_set_locking_callback(NULL);
	isc_mutexblock_destroy(locks, nlocks);
	isc_mem_free(dst__mctx, locks);
	locks = NULL;
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	* defined(LIBRESSL_VERSION_NUMBER) */
	return (result);
}

void
dst__openssl_destroy(void) {
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(LIBRESSL_VERSION_NUMBER)
	/*
	 * Sequence taken from apps_shutdown() in <apps/apps.h>.
	 */
	CONF_modules_free();
	OBJ_cleanup();
	EVP_cleanup();
#if !defined(OPENSSL_NO_ENGINE)
	if (e != NULL) {
		ENGINE_free(e);
	}
	e = NULL;
	ENGINE_cleanup();
#endif /* if !defined(OPENSSL_NO_ENGINE) */
	CRYPTO_cleanup_all_ex_data();
	ERR_clear_error();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	ERR_remove_thread_state(NULL);
#elif defined(LIBRESSL_VERSION_NUMBER)
	ERR_remove_state(0);
#endif /* if OPENSSL_VERSION_NUMBER < 0x10100000L */
	ERR_free_strings();

#ifdef DNS_CRYPTO_LEAKS
	CRYPTO_mem_leaks_fp(stderr);
#endif /* ifdef DNS_CRYPTO_LEAKS */

	if (locks != NULL) {
		CRYPTO_set_locking_callback(NULL);
		isc_mutexblock_destroy(locks, nlocks);
		isc_mem_free(dst__mctx, locks);
		locks = NULL;
	}
#endif /* if (OPENSSL_VERSION_NUMBER < 0x10100000L) || \
	* defined(LIBRESSL_VERSION_NUMBER) */
	isc_mem_detach(&dst__mctx);
}

static isc_result_t
toresult(isc_result_t fallback) {
	isc_result_t result = fallback;
	unsigned long err = ERR_peek_error();
#if defined(ECDSA_R_RANDOM_NUMBER_GENERATION_FAILED)
	int lib = ERR_GET_LIB(err);
#endif /* if defined(ECDSA_R_RANDOM_NUMBER_GENERATION_FAILED) */
	int reason = ERR_GET_REASON(err);

	switch (reason) {
	/*
	 * ERR_* errors are globally unique; others
	 * are unique per sublibrary
	 */
	case ERR_R_MALLOC_FAILURE:
		result = ISC_R_NOMEMORY;
		break;
	default:
#if defined(ECDSA_R_RANDOM_NUMBER_GENERATION_FAILED)
		if (lib == ERR_R_ECDSA_LIB &&
		    reason == ECDSA_R_RANDOM_NUMBER_GENERATION_FAILED) {
			result = ISC_R_NOENTROPY;
			break;
		}
#endif /* if defined(ECDSA_R_RANDOM_NUMBER_GENERATION_FAILED) */
		break;
	}

	return (result);
}

isc_result_t
dst__openssl_toresult(isc_result_t fallback) {
	isc_result_t result;

	result = toresult(fallback);

	ERR_clear_error();
	return (result);
}

isc_result_t
dst__openssl_toresult2(const char *funcname, isc_result_t fallback) {
	return (dst__openssl_toresult3(DNS_LOGCATEGORY_GENERAL, funcname,
				       fallback));
}

isc_result_t
dst__openssl_toresult3(isc_logcategory_t *category, const char *funcname,
		       isc_result_t fallback) {
	isc_result_t result;
	unsigned long err;
	const char *file, *data;
	int line, flags;
	char buf[256];

	result = toresult(fallback);

	isc_log_write(dns_lctx, category, DNS_LOGMODULE_CRYPTO, ISC_LOG_WARNING,
		      "%s failed (%s)", funcname, isc_result_totext(result));

	if (result == ISC_R_NOMEMORY) {
		goto done;
	}

	for (;;) {
		err = ERR_get_error_line_data(&file, &line, &data, &flags);
		if (err == 0U) {
			goto done;
		}
		ERR_error_string_n(err, buf, sizeof(buf));
		isc_log_write(dns_lctx, category, DNS_LOGMODULE_CRYPTO,
			      ISC_LOG_INFO, "%s:%s:%d:%s", buf, file, line,
			      ((flags & ERR_TXT_STRING) != 0) ? data : "");
	}

done:
	ERR_clear_error();
	return (result);
}

#if !defined(OPENSSL_NO_ENGINE)
ENGINE *
dst__openssl_getengine(const char *engine) {
	if (engine == NULL) {
		return (NULL);
	}
	if (e == NULL) {
		return (NULL);
	}
	if (strcmp(engine, ENGINE_get_id(e)) == 0) {
		return (e);
	}
	return (NULL);
}
#endif /* if !defined(OPENSSL_NO_ENGINE) */

/*! \file */
