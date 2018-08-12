/*	$NetBSD: random.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_RANDOM_H
#define ISC_RANDOM_H 1

#include <isc/lang.h>
#include <isc/types.h>
#include <isc/entropy.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/deprecated.h>

/*! \file isc/random.h
 * \brief Implements pseudo random number generators.
 *
 * Two pseudo-random number generators are implemented, in isc_random_*
 * and isc_rng_*. Neither one is very strong; they should not be used
 * in cryptography functions.
 *
 * isc_random_* is based on arc4random if it is available on the system.
 * Otherwise it is based on the posix srand() and rand() functions.
 * It is useful for jittering values a bit here and there, such as
 * timeouts, etc, but should not be relied upon to generate
 * unpredictable sequences (for example, when choosing transaction IDs).
 *
 * isc_rng_* is based on ChaCha20, and is seeded and stirred from the
 * system entropy source. It is stronger than isc_random_* and can
 * be used for generating unpredictable sequences. It is still not as
 * good as using system entropy directly (see entropy.h) and should not
 * be used for cryptographic functions such as key generation.
 */

ISC_LANG_BEGINDECLS

typedef struct isc_rng isc_rng_t;
/*%<
 * Opaque type
 */

void
isc_random_seed(isc_uint32_t seed);
/*%<
 * Set the initial seed of the random state.
 */

void
isc_random_get(isc_uint32_t *val);
/*%<
 * Get a random value.
 *
 * Requires:
 *	val != NULL.
 */

isc_uint32_t
isc_random_jitter(isc_uint32_t max, isc_uint32_t jitter);
/*%<
 * Get a random value between (max - jitter) and (max).
 * This is useful for jittering timer values.
 */

isc_result_t
isc_rng_create(isc_mem_t *mctx, isc_entropy_t *entropy, isc_rng_t **rngp);
/*%<
 * Creates and initializes a pseudo random number generator. The
 * returned RNG can be used to generate pseudo random numbers.
 *
 * The reference count of the returned RNG is set to 1.
 *
 * Requires:
 * \li mctx is a pointer to a valid memory context.
 * \li entropy is an optional entopy source (can be NULL)
 * \li rngp != NULL && *rngp == NULL is where a pointer to the RNG is
 *     returned.
 *
 * Ensures:
 *\li	If result is ISC_R_SUCCESS:
 *		*rngp points to a valid RNG.
 *
 *\li	If result is failure:
 *		*rngp does not point to a valid RNG.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS	Success
 *\li	#ISC_R_NOMEMORY Resource limit: Out of Memory
 */

void
isc_rng_attach(isc_rng_t *source, isc_rng_t **targetp);
/*%<
 * Increments a reference count on the passed RNG.
 *
 * Requires:
 * \li source the RNG struct to attach to (is refcount is incremented)
 * \li targetp != NULL && *targetp == NULL where a pointer to the
 *     reference incremented RNG is returned.
 */

void
isc_rng_detach(isc_rng_t **rngp);
/*%<
 * Decrements a reference count on the passed RNG. If the reference
 * count reaches 0, the RNG is destroyed.
 *
 * Requires:
 * \li rngp != NULL the RNG struct to decrement reference for
 */

void
isc_rng_randombytes(isc_rng_t *rngctx, void *output, size_t length);
/*%<
 * Returns a pseudo random sequence of length octets in output.
 */

isc_uint16_t
isc_rng_random(isc_rng_t *rngctx) ISC_DEPRECATED;
/*%<
 * Returns a pseudo random 16-bit unsigned integer.
 *
 * This function is deprecated. You should use `isc_rng_randombytes()`
 * instead.
 */

isc_uint16_t
isc_rng_uniformrandom(isc_rng_t *rngctx, isc_uint16_t upper_bound);
/*%<
 * Returns a uniformly distributed pseudo-random 16-bit unsigned integer
 * less than 'upper_bound'.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_RANDOM_H */
