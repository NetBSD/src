/*
 * Copyright (c) 2022-2024 Apple Inc. All rights reserved.
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

//======================================================================================================================
// MARK: - Headers

#include "dns_obj_log.h"
#include "dns_obj_crypto.h"
#include "dnssec_obj_rr_dnskey.h"

#ifdef __APPLE__
#include <Security/Security.h>
#include <Security/SecKeyPriv.h>
#endif

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Local Prototypes

#ifdef __APPLE__

static SecKeyRef
sec_key_create(uint8_t algorithm, const uint8_t * NONNULL key, size_t key_size, dns_obj_error_t * NULLABLE out_error);

static CFDataRef
signed_data_create(uint8_t algorithm, const uint8_t * NONNULL data, size_t data_len, dns_obj_error_t * NULLABLE out_error);

static dns_obj_error_t
dnskey_algorithm_to_sec_key_algorithm(uint8_t dnskey_algorithm, SecKeyAlgorithm * NONNULL out_sec_key_algorithm);

#endif

//======================================================================================================================
// MARK: - Public Functions

dns_obj_error_t
dns_obj_data_compute_digest_init(dns_obj_digest_ctx_t * const context, const digest_type_t digest_type)
{
#ifdef __APPLE__
	dns_obj_error_t err;
	CCDigestAlgorithm algorithm;

	err = DNSSEC_ERROR_NO_ERROR;
	switch (digest_type) {
		case DIGEST_SHA_1:
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			algorithm = kCCDigestSHA1;
		#pragma clang diagnostic pop
			break;
		case DIGEST_SHA_256:
			algorithm = kCCDigestSHA256;
			break;
		case DIGEST_SHA_384:
			algorithm = kCCDigestSHA384;
			break;
		case DIGEST_SHA_512:
			algorithm = kCCDigestSHA512;
			break;
		case DIGEST_UNSUPPORTED:
		MDNS_COVERED_SWITCH_DEFAULT:
			err = DNSSEC_ERROR_UNSUPPORTED_ERR;
			algorithm = kCCDigestNone;
			break;
	}
	require_noerr(err, exit);

	const int ret = CCDigestInit(algorithm, &context->ctx);
	require_action(ret == 0, exit, err = DNSSEC_ERROR_UNKNOWN_ERR);

exit:
	return err;
#else // __APPLE__
	(void)context;
	(void)digest_type;
	return DNSSEC_ERROR_UNSUPPORTED_ERR;
#endif // __APPLE__
}

//======================================================================================================================

dns_obj_error_t
dns_obj_data_compute_digest_update(dns_obj_digest_ctx_t * const context, const uint8_t * const data, const size_t length)
{
#ifdef __APPLE__
	const int ret = CCDigestUpdate(&context->ctx, data, length);
	return (ret == 0) ? DNSSEC_ERROR_NO_ERROR : DNSSEC_ERROR_UNKNOWN_ERR;
#else
	(void)context;
	(void)data;
	(void)length;
	return DNSSEC_ERROR_UNSUPPORTED_ERR;
#endif
}

//======================================================================================================================

dns_obj_error_t
dns_obj_data_compute_digest_final(dns_obj_digest_ctx_t * const context, uint8_t * const out_digest)
{
#ifdef __APPLE__
	const int ret = CCDigestFinal(&context->ctx, out_digest);
	return (ret == 0) ? DNSSEC_ERROR_NO_ERROR : DNSSEC_ERROR_UNKNOWN_ERR;
#else
	(void)context;
	(void)output;
	return DNSSEC_ERROR_UNSUPPORTED_ERR;
#endif
}

//======================================================================================================================

size_t
dns_obj_data_compute_digest_get_output_size(const digest_type_t digest_type)
{
#ifdef __APPLE__
	dns_obj_error_t err;
	CCDigestAlgorithm algorithm;

	err = DNSSEC_ERROR_NO_ERROR;
	switch (digest_type) {
		case DIGEST_SHA_1:
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			algorithm = kCCDigestSHA1;
			break;
		#pragma clang diagnostic pop
		case DIGEST_SHA_256:
			algorithm = kCCDigestSHA256;
			break;
		case DIGEST_SHA_384:
			algorithm = kCCDigestSHA384;
			break;
		case DIGEST_SHA_512:
			algorithm = kCCDigestSHA512;
			break;
		case DIGEST_UNSUPPORTED:
		MDNS_COVERED_SWITCH_DEFAULT:
			err = DNSSEC_ERROR_UNSUPPORTED_ERR;
			algorithm = kCCDigestNone;
			break;
	}
	if (err != DNSSEC_ERROR_NO_ERROR) {
		return 0;
	}

	return CCDigestGetOutputSize(algorithm);
#else
	(void)digest_type;
	return 0;
#endif
}

//======================================================================================================================

void
dns_obj_data_compute_digest_reset(dns_obj_digest_ctx_t * const context)
{
#ifdef __APPLE__
	CCDigestReset(&context->ctx);
#else
	(void)context;
	return DNSSEC_ERROR_UNSUPPORTED_ERR;
#endif
}

//======================================================================================================================

void
dns_obj_data_compute_digest(const digest_type_t digest_type, const uint8_t * const data, const size_t data_len,
	uint8_t * const out_digest, dns_obj_error_t * const out_error)
{
#ifdef __APPLE__
	dns_obj_error_t err;
	CCDigestAlgorithm algorithm;

	err = DNSSEC_ERROR_NO_ERROR;
	switch (digest_type) {
		case DIGEST_SHA_1:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			algorithm = kCCDigestSHA1;
			break;
#pragma clang diagnostic pop
		case DIGEST_SHA_256:
			algorithm = kCCDigestSHA256;
			break;
		case DIGEST_SHA_384:
			algorithm = kCCDigestSHA384;
			break;
		case DIGEST_SHA_512:
			algorithm = kCCDigestSHA512;
			break;
		case DIGEST_UNSUPPORTED:
		MDNS_COVERED_SWITCH_DEFAULT:
			err = DNSSEC_ERROR_UNSUPPORTED_ERR;
			algorithm = kCCDigestNone;
			break;
	}
	require_noerr(err, exit);

	CCDigest(algorithm, data, data_len, out_digest);

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
#else
	(void)digest_type;
	(void)data;
	(void)data_len;
	(void)out_digest;
	*out_error = DNSSEC_ERROR_UNSUPPORTED_ERR;
#endif
}

//======================================================================================================================

dns_obj_error_t
dns_obj_compute_nsec3_digest(const digest_type_t digest_type, const uint8_t * const data, const size_t data_length,
	const uint16_t iterations, const uint8_t * const salt, const uint8_t salt_length, uint8_t * const out_digest)
{
	dns_obj_error_t err;
	uint8_t buff[MAX_HASHED_NAME_BUFF_SIZE];
	size_t buff_len;
	dns_obj_digest_ctx_t context;

	err = dns_obj_data_compute_digest_init(&context, digest_type);
	require_noerr(err, exit);

	memcpy(buff, data, data_length);
	buff_len = data_length;

	for (size_t i = 0; i <= iterations; dns_obj_data_compute_digest_reset(&context), i++) {
		err = dns_obj_data_compute_digest_update(&context, buff, buff_len);
		require_noerr(err, exit);

		err = dns_obj_data_compute_digest_update(&context, salt, salt_length);
		require_noerr(err, exit);

		err = dns_obj_data_compute_digest_final(&context, buff);
		require_noerr(err, exit);

		buff_len = dns_obj_data_compute_digest_get_output_size(digest_type);
		require_action(buff_len > 0, exit, err = DNSSEC_ERROR_UNSUPPORTED_ERR);
	}

	memcpy(out_digest, buff, buff_len);

exit:
	return err;
}

//======================================================================================================================

bool
dns_obj_dnssec_signature_verify(const uint8_t * const data, const size_t data_len, const uint8_t algorithm,
	const uint8_t * const key, const size_t key_size, const uint8_t * const signature, const size_t signature_len,
	dns_obj_error_t * const out_error)
{
#ifndef __APPLE__
	(void)data;
	(void)data_len;
	(void)algorithm;
	(void)key;
	(void)key_size;
	(void)signature;
	(void)signature_len;
	*out_error = DNSSEC_ERROR_UNSUPPORTED_ERR;
	return dns_obj_indeterminate;
#else

	dns_obj_error_t err;
	SecKeyRef sec_key = NULL;
	CFDataRef signature_cfdata = NULL;
	CFDataRef signed_data_cfdata = NULL;
	bool result;

	sec_key = sec_key_create(algorithm, key, key_size, &err);
	require_noerr_action(err, exit, result = false);

	signature_cfdata = CFDataCreate(kCFAllocatorDefault, signature, (CFIndex)signature_len);
	require_action(signature_cfdata != NULL, exit, err = DNSSEC_ERROR_NO_MEMORY; result = false);

	signed_data_cfdata = signed_data_create(algorithm, data, data_len, &err);
	require_noerr_action(err, exit, result = false);

	SecKeyAlgorithm sec_key_algorithm;
	err = dnskey_algorithm_to_sec_key_algorithm(algorithm, &sec_key_algorithm);
	require_noerr_action(err, exit, result = false);

	const bool valid = SecKeyVerifySignature(sec_key, sec_key_algorithm, signed_data_cfdata, signature_cfdata, NULL);
	result = valid;
	err = valid ? DNSSEC_ERROR_NO_ERROR : DNSSEC_ERROR_MISMATCH_ERR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_CF_OBJECT(signed_data_cfdata);
	MDNS_DISPOSE_CF_OBJECT(signature_cfdata);
	MDNS_DISPOSE_CF_OBJECT(sec_key);
	return result;
#endif
}

//======================================================================================================================
// MARK: - Local Functions

#ifdef __APPLE__

//======================================================================================================================

static void
rsa_public_key_parse(uint8_t * const key, const size_t key_size,
	uint8_t ** const out_modulus, CFIndex * const out_modulus_len,
	uint8_t ** const out_exponent, CFIndex * const out_exponent_len)
{
	// <https://datatracker.ietf.org/doc/html/rfc3110#section-2>

	uint8_t exponent_len_bytes_len;
	if (key[0] != 0) {
		*out_exponent_len = key[0];
		exponent_len_bytes_len = 1;
	} else {
		*out_exponent_len = get_uint16_from_bytes(&key[1]);
		exponent_len_bytes_len = 3;
	}

	*out_exponent = key + exponent_len_bytes_len;
	*out_modulus_len = (CFIndex)(key_size - (((size_t)*out_exponent_len) + exponent_len_bytes_len));
	*out_modulus = key + exponent_len_bytes_len + *out_exponent_len;
}

//======================================================================================================================

static SecKeyRef
sec_key_create_rsa(const uint8_t * const key, const size_t key_size, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	SecKeyRef rsa_key = NULL;
	uint8_t *key_copy = NULL;

	key_copy = mdns_malloc(key_size);
	require_action(key_copy != NULL, exit, err = DNSSEC_ERROR_NO_MEMORY);

	memcpy(key_copy, key, key_size);

	SecRSAPublicKeyParams params;
	bzero(&params, sizeof(params));
	rsa_public_key_parse(key_copy, key_size, &params.modulus, &params.modulusLength, &params.exponent, &params.exponentLength);

	rsa_key = SecKeyCreateRSAPublicKey(kCFAllocatorDefault, (const uint8_t *)&params, sizeof(params), kSecKeyEncodingRSAPublicParams);
	err = DNSSEC_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	mdns_free(key_copy);
	return rsa_key;
}

//======================================================================================================================

static SecKeyRef
sec_key_create_ecdsa(const uint8_t * const key, const size_t key_size, dns_obj_error_t * const out_error)
{
	// <https://datatracker.ietf.org/doc/html/rfc6605#section-4>
	// <https://datatracker.ietf.org/doc/html/rfc5480#section-2.2>
	dns_obj_error_t err;
	CFMutableDataRef key_cfdata = NULL;
	CFDictionaryRef key_options_cfdictionary = NULL;
	SecKeyRef ecdsa_key = NULL;

	const uint8_t const_four = 4;
	mdns_compile_time_check_local(sizeof(const_four) == 1);
	key_cfdata = CFDataCreateMutable(kCFAllocatorDefault, (CFIndex)(sizeof(const_four) + key_size));
	require_action(key_cfdata != NULL, exit, err = DNSSEC_ERROR_NO_MEMORY);

	// ECDSA key format: ANSI X9.63 format (04 || X || Y)
	CFDataAppendBytes(key_cfdata, &const_four, (CFIndex)sizeof(const_four));
	CFDataAppendBytes(key_cfdata, key, (CFIndex)key_size);

	const void *key_options_keys[]		= {kSecAttrKeyType,					kSecAttrKeyClass};
	const void *key_options_values[]	= {kSecAttrKeyTypeECSECPrimeRandom,	kSecAttrKeyClassPublic};
	mdns_compile_time_check_local(countof(key_options_keys) == countof(key_options_values));
	key_options_cfdictionary = CFDictionaryCreate(kCFAllocatorDefault, key_options_keys, key_options_values,
		countof(key_options_keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	require_action(key_options_cfdictionary != NULL, exit, err = DNSSEC_ERROR_NO_MEMORY);

	ecdsa_key = SecKeyCreateWithData(key_cfdata, key_options_cfdictionary, NULL);
	require_action(ecdsa_key != NULL, exit, err = DNSSEC_ERROR_UNKNOWN_ERR);

	err = DNSSEC_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_CF_OBJECT(key_options_cfdictionary);
	MDNS_DISPOSE_CF_OBJECT(key_cfdata);
	return ecdsa_key;
}

//======================================================================================================================

static SecKeyRef
sec_key_create(const uint8_t algorithm, const uint8_t * const key, const size_t key_size, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	SecKeyRef sec_key = NULL;

	switch (algorithm) {
		case DNSKEY_ALGORITHM_RSASHA1:
		case DNSKEY_ALGORITHM_RSASHA1_NSEC3_SHA1:
		case DNSKEY_ALGORITHM_RSASHA256:
		case DNSKEY_ALGORITHM_RSASHA512:
			sec_key = sec_key_create_rsa(key, key_size, &err);
			break;
		case DNSKEY_ALGORITHM_ECDSAP256SHA256:
		case DNSKEY_ALGORITHM_ECDSAP384SHA384:
			sec_key = sec_key_create_ecdsa(key, key_size, &err);
			break;
		default:
			err = DNSSEC_ERROR_DNSKEY_UNSUPPORTED_ALGORITHM;
			break;
	}

	if (out_error != NULL) {
		*out_error = err;
	}
	return sec_key;
}

//======================================================================================================================

static CFDataRef
signed_data_create(const uint8_t algorithm, const uint8_t * const data, const size_t data_len,
	dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	CFDataRef data_ref = NULL;
	digest_type_t digest_type = DIGEST_UNSUPPORTED;

	switch (algorithm) {
		case DNSKEY_ALGORITHM_RSASHA1:
		case DNSKEY_ALGORITHM_RSASHA1_NSEC3_SHA1:
		case DNSKEY_ALGORITHM_RSASHA256:
		case DNSKEY_ALGORITHM_RSASHA512:
			data_ref = CFDataCreate(kCFAllocatorDefault, data, (CFIndex)data_len);
			require_action(data_ref != NULL, exit, err = DNSSEC_ERROR_NO_MEMORY);

			err = DNSSEC_ERROR_NO_ERROR;
			break;
		case DNSKEY_ALGORITHM_ECDSAP256SHA256:
			if (digest_type == DIGEST_UNSUPPORTED) {
				digest_type = DIGEST_SHA_256;
			}
			// Fall through
		case DNSKEY_ALGORITHM_ECDSAP384SHA384:
			if (digest_type == DIGEST_UNSUPPORTED) {
				digest_type = DIGEST_SHA_384;
			}

			uint8_t hashed_data[MAX_DIGEST_OUTPUT_SIZE];
			data_compute_digest(digest_type, data, data_len, hashed_data, &err);
			require_noerr(err, exit);
			const size_t hashed_data_len = data_compute_digest_get_output_size(digest_type);

			data_ref = CFDataCreate(kCFAllocatorDefault, hashed_data, (CFIndex)hashed_data_len);
			require_action(data_ref != NULL, exit, err = DNSSEC_ERROR_NO_MEMORY);

			err = DNSSEC_ERROR_NO_ERROR;
			break;
		default:
			err = DNSSEC_ERROR_DNSKEY_UNSUPPORTED_ALGORITHM;
			break;
	}

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return data_ref;
}

//======================================================================================================================

static dns_obj_error_t
dnskey_algorithm_to_sec_key_algorithm(const uint8_t dnskey_algorithm, SecKeyAlgorithm * const out_sec_key_algorithm)
{
	dns_obj_error_t err = DNSSEC_ERROR_NO_ERROR;;
	switch (dnskey_algorithm) {
		case DNSKEY_ALGORITHM_RSASHA1:
		case DNSKEY_ALGORITHM_RSASHA1_NSEC3_SHA1:
			*out_sec_key_algorithm = kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA1;
			break;
		case DNSKEY_ALGORITHM_RSASHA256:
			*out_sec_key_algorithm = kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256;
			break;
		case DNSKEY_ALGORITHM_RSASHA512:
			*out_sec_key_algorithm = kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA512;
			break;
		case DNSKEY_ALGORITHM_ECDSAP256SHA256:
		case DNSKEY_ALGORITHM_ECDSAP384SHA384:
			*out_sec_key_algorithm = kSecKeyAlgorithmECDSASignatureDigestRFC4754;
			break;
		default:
			err = DNSSEC_ERROR_UNSUPPORTED_ERR;
			break;
	}

	return err;
}

#endif // __APPLE__
