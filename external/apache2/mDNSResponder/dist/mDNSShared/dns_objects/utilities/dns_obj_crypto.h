/*
 * Copyright (c) 2022 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DNS_OBJ_CRYPTO_H
#define DNS_OBJ_CRYPTO_H

//======================================================================================================================
// MARK: - Headers

#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <CommonCrypto/CommonDigestSPI.h>
#endif

#include "dns_assert_macros.h"
#include "nullability.h"

//======================================================================================================================
// MARK: - Constants

#ifndef SHA1_OUTPUT_SIZE

// Digest constants.
typedef enum digest_type {
	DIGEST_UNSUPPORTED,
	DIGEST_SHA_1,
	DIGEST_SHA_256,
	DIGEST_SHA_384,
	DIGEST_SHA_512
} digest_type_t;

#define SHA1_OUTPUT_SIZE		20
#define SHA256_OUTPUT_SIZE		32
#define SHA384_OUTPUT_SIZE		48
#define SHA512_OUTPUT_SIZE		64
#ifndef MAX_DIGEST_OUTPUT_SIZE
#define MAX_DIGEST_OUTPUT_SIZE	SHA512_OUTPUT_SIZE
#endif

check_compile_time(SHA512_OUTPUT_SIZE	<= MAX_DIGEST_OUTPUT_SIZE);
check_compile_time(SHA384_OUTPUT_SIZE	<= MAX_DIGEST_OUTPUT_SIZE);
check_compile_time(SHA1_OUTPUT_SIZE		<= MAX_DIGEST_OUTPUT_SIZE);
check_compile_time(SHA256_OUTPUT_SIZE	<= MAX_DIGEST_OUTPUT_SIZE);

#define MAX_HASHED_NAME_INPUT_SIZE	(MAX_DOMAIN_NAME + UINT8_MAX)
#define MAX_HASHED_NAME_OUTPUT_SIZE	MAX_DIGEST_OUTPUT_SIZE
#define MAX_HASHED_NAME_BUFF_SIZE	(MAX(MAX_HASHED_NAME_INPUT_SIZE, MAX_HASHED_NAME_OUTPUT_SIZE))

//======================================================================================================================
// Signature verification constants.

#define MAX_PUBLIC_KEY_BYTES		RSA_PUBLIC_MAX_KEY_BYTES
#define MAX_SECRET_KEY_BYTES		RSA_SECRET_MAX_KEY_BYTES
#define MAX_SIGNATURE_BYTES			RSA_SIGNATURE_BYTES

// RSA
#define RSA_PUBLIC_MAX_KEY_BYTES	260
#define RSA_SECRET_MAX_KEY_BYTES	1190
#define RSA_SIGNATURE_BYTES			256
check_compile_time(MAX_PUBLIC_KEY_BYTES	>= RSA_PUBLIC_MAX_KEY_BYTES);
check_compile_time(MAX_SECRET_KEY_BYTES	>= RSA_SECRET_MAX_KEY_BYTES);
check_compile_time(MAX_SIGNATURE_BYTES	>= RSA_SIGNATURE_BYTES);

// ECDSAP
#define ECDSAP_PUBLIC_KEY_BYTES		64
#define ECDSAP_SECRET_KEY_BYTES		96
#define ECDSAP_SIGNATURE_BYTES		64
check_compile_time(MAX_PUBLIC_KEY_BYTES	>= ECDSAP_PUBLIC_KEY_BYTES);
check_compile_time(MAX_SECRET_KEY_BYTES	>= ECDSAP_SECRET_KEY_BYTES);
check_compile_time(MAX_SIGNATURE_BYTES	>= ECDSAP_SIGNATURE_BYTES);

// ED25519
#define ED25519_PUBLIC_KEY_BYTES	32
#define ED25519_SECRET_KEY_BYTES	32
#define ED25519_SIGNATURE_BYTES		64
check_compile_time(MAX_PUBLIC_KEY_BYTES	>= ED25519_PUBLIC_KEY_BYTES);
check_compile_time(MAX_SECRET_KEY_BYTES	>= ED25519_SECRET_KEY_BYTES);
check_compile_time(MAX_SIGNATURE_BYTES	>= ED25519_SIGNATURE_BYTES);

// ED448
#define ED448_PUBLIC_KEY_BYTES		57
#define ED448_SECRET_KEY_BYTES		57
#define ED448_SIGNATURE_BYTES		114
check_compile_time(MAX_PUBLIC_KEY_BYTES	>= ED448_PUBLIC_KEY_BYTES);
check_compile_time(MAX_SECRET_KEY_BYTES	>= ED448_SECRET_KEY_BYTES);
check_compile_time(MAX_SIGNATURE_BYTES	>= ED448_SIGNATURE_BYTES);

#endif // #ifndef SHA1_OUTPUT_SIZE

//======================================================================================================================
// MARK: - Structs

typedef struct dns_obj_digest_ctx_s {
#ifdef __APPLE__
	CCDigestCtx ctx;
#else
	uint8_t _dummy_ctx;
#endif
} dns_obj_digest_ctx_t;

//======================================================================================================================
// MARK: - Function Declarations

/*!
 *	@brief
 *		Initialize the digest context and get it ready for updating the digest.
 *
 *	@param context
 *		The digest context.
 *
 *	@param digest_type
 *		The digest type that would be calculated, including:
 *			DIGEST_SHA_1
 *			DIGEST_SHA_256
 *			DIGEST_SHA_384
 *			DIGEST_SHA_512
 *
 *	@result
 *		DNS_OBJ_ERROR_NO_ERROR if no error occurs, otherwise, the corresponding error encountered when doing initialization.
 */
dns_obj_error_t
dns_obj_data_compute_digest_init(dns_obj_digest_ctx_t * NONNULL context, digest_type_t digest_type);

/*!
 *	@brief
 *		Append more data to generate the final digest output.
 *
 *	@param context
 *		The context that has been initialized by <code>data_compute_digest_init()</code>.
 *
 *	@param length
 *		 The length of the data passed in bytes.
 *
 *	@result
 *		DNS_OBJ_ERROR_NO_ERROR if no error occurs, otherwise, the corresponding error encountered when doing initialization.
 */
dns_obj_error_t
dns_obj_data_compute_digest_update(dns_obj_digest_ctx_t * NONNULL context, const uint8_t * NONNULL data, size_t length);

/*!
 *	@brief
 *		Get the digest result of all data that has been updated with <code>data_compute_digest_update()</code>.
 *
 *	@param context
 *		The context that has been fed with the data to calculate the digest.
 *
 *	@param out_digest
 *		The digest output of the data for the specific digest type.
 *
 *	@result
 *		DNS_OBJ_ERROR_NO_ERROR if no error occurs, otherwise, the corresponding error encountered when doing initialization.
 */
dns_obj_error_t
dns_obj_data_compute_digest_final(dns_obj_digest_ctx_t * NONNULL context, uint8_t * NONNULL out_digest);

/*!
 *	@brief
 *		Get the digest length of the specific digest type.
 *
 *	@param digest_type
 *		The type of the digest, including:
 *			DIGEST_SHA_1
 *			DIGEST_SHA_256
 *			DIGEST_SHA_384
 *			DIGEST_SHA_512
 *
 *	@result
 *		The size of the digest.
 */
size_t
dns_obj_data_compute_digest_get_output_size(digest_type_t digest_type);

/*!
 *	@brief
 *		Reset the context to compute a new digest for a new data.
 *
 *	@param context
 *		The digest context.
 */
void
dns_obj_data_compute_digest_reset(dns_obj_digest_ctx_t * NONNULL context);

/*!
 *	@brief
 *		Compute the digest of the data in one shot.
 *
 *	@param digest_type
 *		The type of the digest to compute.
 *
 *	@param data
 *		The pointer to the data bytes.
 *
 *	@param data_len
 *		The length of the data.
 *
 *	@param out_digest
 *		The pointer to the buffer that stores the digest output.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 */
void
dns_obj_data_compute_digest(digest_type_t digest_type, const uint8_t * NONNULL data, size_t data_len,
	uint8_t * NONNULL out_digest, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Compute the digest used by NSEC3 resource record to hash a domain name.
 *
 *	@param digest_type
 *		The type of the digest to compute, currently NSEC3 only uses DIGEST_SHA_1.
 *
 *	@param data
 *		The data to compute the digest.
 *
 *	@param data_length
 *		The length of the data above.
 *
 *	@param iterations
 *		The extra iterations of the digest calculation.
 *
 *	@param salt
 *		The salt to append when calculating the digest for each iteration.
 *
 *	@param salt_length
 *		The length of the salt above.
 *
 *	@param out_digest
 *		The buffer where the digest will be stored as the output.
 *
 *	@result
 *		DNS_OBJ_ERROR_NO_ERROR if no error occurs, otherwise, the corresponding error encountered during the calculation.
 */
dns_obj_error_t
dns_obj_compute_nsec3_digest(digest_type_t digest_type, const uint8_t * NONNULL data, size_t data_length,
	uint16_t iterations, const uint8_t * NULLABLE salt, uint8_t salt_length, uint8_t * NONNULL out_digest);

/*!
 *	@brief
 *		Verify the DNSSEC signature of the data given the public key.
 *
 *	@param data
 *		The pointer to the data bytes to be verified.
 *
 *	@param data_len
 *		The length of the data bytes.
 *
 *	@param algorithm
 *		The algorithm used to generate the signature.
 *
 *	@param key
 *		The public key in bytes.
 *
 *	@param key_size
 *		The size of the public key.
 *
 *	@param signature
 *		The pointer to the signature bytes.
 *
 *	@param signature_len
 *		The length of the signature.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the validation call or the cause of the validation failure.
 *
 *	@result
 *		True if the signature matches, otherwise, false.
 */
bool
dns_obj_dnssec_signature_verify(const uint8_t * NONNULL data, size_t data_len, uint8_t algorithm,
	const uint8_t * NONNULL key, size_t key_size, const uint8_t * NONNULL signature, size_t signature_len,
	dns_obj_error_t * NULLABLE out_error);

#endif // DNS_OBJ_CRYPTO_H
