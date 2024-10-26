/*
 * Copyright (c) 2022-2023 Apple Inc. All rights reserved.
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

#ifndef DNS_OBJ_COMMON_H
#define DNS_OBJ_COMMON_H

//======================================================================================================================
// MARK: - Headers

#include "ref_count.h"
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Macro Helpers

#ifndef MIN
	#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif

#ifndef MAX
	#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#endif

#ifndef countof
	#define	countof(X)					(sizeof(X) / sizeof(X[0]))
#endif

#ifndef countof_field
	#define	countof_field(TYPE, FIELD)	countof(((TYPE *)0)->FIELD)
#endif

#ifndef likely
	#define likely(EXPRESSSION)			__builtin_expect(!!(EXPRESSSION), 1)
#endif

#ifndef unlikely
	#define unlikely(EXPRESSSION)		__builtin_expect(!!(EXPRESSSION), 0)
#endif

#ifndef isdigit_safe
	#define isdigit_safe(X) isdigit(((unsigned char)((X) & 0xFF)))
#endif

#if (defined(__clang__) && __clang__)
	#define DNS_OBJ_CLANG_WARNING_HELPER_0(X)		#X
	#define DNS_OBJ_CLANG_WARNING_HELPER_1(X)		DNS_OBJ_CLANG_WARNING_HELPER_0(clang diagnostic ignored X)
	#define DNS_OBJ_CLANG_WARNING_HELPER_2(Y)		DNS_OBJ_CLANG_WARNING_HELPER_1(#Y)
	#define DNS_OBJ_CLANG_WARNING(X)					_Pragma(DNS_OBJ_CLANG_WARNING_HELPER_2(X))

	#define DNS_OBJ_CLANG_WARNING_IGNORE_BEGIN(X)	\
		_Pragma( "clang diagnostic push" )			\
		DNS_OBJ_CLANG_WARNING(X)						\
		do {} while( 0 )
#else
	#define DNS_OBJ_CLANG_WARNING_IGNORE_BEGIN(X)	do {} while(0)
#endif

#if (defined(__clang__) && __clang__)
	#define DNS_OBJ_CLANG_WARNING_IGNORE_END()		_Pragma( "clang diagnostic pop" ) do {} while(0)
#else
	#define DNS_OBJ_CLANG_WARNING_IGNORE_END()		do {} while(0)
#endif

//======================================================================================================================
// MARK: - Constants

typedef int32_t dns_obj_error_t;
#define DNS_OBJ_ERROR_NO_ERROR						0		// No error occurred.

#define DNS_OBJ_ERROR_GENERIC_ERROR_BASE			-6700	// Starting error code for all generic errors.
#define DNS_OBJ_ERROR_UNKNOWN_ERR					-6700	// Unknown error occurred.
#define DNS_OBJ_ERROR_PARAM_ERR						-6705	// Parameter is incorrect, missing, or not appropriate.
#define DNS_OBJ_ERROR_STATE_ERR						-6709	// Not in appropriate state to perform operation.
#define DNS_OBJ_ERROR_RANGE_ERR						-6710	// Index is out of range or not valid.
#define DNS_OBJ_ERROR_REQUEST_ERR					-6711	// Request was improperly formed or not appropriate.
#define DNS_OBJ_ERROR_NOT_INITIALIZED_ERR			-6718	// Action request before needed services were initialized.
#define DNS_OBJ_ERROR_ALREADY_INITIALIZED_ERR		-6719	// Attempt made to initialize when already initialized.
#define DNS_OBJ_ERROR_NOT_IN_USE_ERR				-6720	// Object not in use (e.g. cannot abort if not already in use).
#define DNS_OBJ_ERROR_NOT_FOUND_ERR					-6727	// Something was not found.
#define DNS_OBJ_ERROR_NO_MEMORY						-6728	// Not enough memory was available to perform the operation.
#define DNS_OBJ_ERROR_NO_RESOURCES					-6729	// Resources unavailable to perform the operation.
#define DNS_OBJ_ERROR_DUPLICATE_ERR					-6730	// Duplicate found or something is a duplicate.
#define DNS_OBJ_ERROR_UNSUPPORTED_ERR				-6735	// Feature or option is not supported.
#define DNS_OBJ_ERROR_UNEXPECTED_ERR				-6736	// Error occurred that was not expected.
#define DNS_OBJ_ERROR_MALFORMED_ERR					-6742	// Something was not formed correctly.
#define DNS_OBJ_ERROR_NOT_READY_ERR					-6745	// Device or service is not ready.
#define DNS_OBJ_ERROR_MISMATCH_ERR					-6748	// Something does not match.
#define DNS_OBJ_ERROR_DATE_ERR						-6749	// Date is invalid or out-of-range.
#define DNS_OBJ_ERROR_AUTHENTICATION_ERR			-6754	// Authentication failed or is not supported.
#define DNS_OBJ_ERROR_UNDER_RUN						-6750	// Less data than expected.
#define DNS_OBJ_ERROR_OVER_RUN						-6751	// More data than expected.
#define DNS_OBJ_ERROR_TYPE_ERR						-6756	// Incorrect or incompatible type (e.g. file, data, etc.).
#define DNS_OBJ_ERROR_GENERIC_ERROR_END				-6779	// Last generic error code (inclusive)

//======================================================================================================================
// MARK: - DNS Message Common Values

#ifndef MAX_ESCAPED_DOMAIN_NAME
#define MAX_ESCAPED_DOMAIN_NAME 1009
#endif

#ifndef MAX_DOMAIN_NAME
#define MAX_DOMAIN_NAME 256
#endif

#ifndef MAX_DOMAIN_LABEL
#define MAX_DOMAIN_LABEL 63
#endif

#define MAX_UNICAST_TTL_IN_SECONDS ((uint32_t)3600)

// Record types
#ifndef kDNSRecordType_Enum
#define kDNSRecordType_Enum
typedef enum
{
	kDNSRecordType_Invalid		= 0,
	kDNSRecordType_A			= 1,
	kDNSRecordType_NS			= 2,
	kDNSRecordType_MD			= 3,
	kDNSRecordType_MF			= 4,
	kDNSRecordType_CNAME		= 5,
	kDNSRecordType_SOA			= 6,
	kDNSRecordType_MB			= 7,
	kDNSRecordType_MG			= 8,
	kDNSRecordType_MR			= 9,
	kDNSRecordType_NULL			= 10,
	kDNSRecordType_WKS			= 11,
	kDNSRecordType_PTR			= 12,
	kDNSRecordType_HINFO		= 13,
	kDNSRecordType_MINFO		= 14,
	kDNSRecordType_MX			= 15,
	kDNSRecordType_TXT			= 16,
	kDNSRecordType_RP			= 17,
	kDNSRecordType_AFSDB		= 18,
	kDNSRecordType_X25			= 19,
	kDNSRecordType_ISDN			= 20,
	kDNSRecordType_RT			= 21,
	kDNSRecordType_NSAP			= 22,
	kDNSRecordType_NSAP_PTR		= 23,
	kDNSRecordType_SIG			= 24,
	kDNSRecordType_KEY			= 25,
	kDNSRecordType_PX			= 26,
	kDNSRecordType_GPOS			= 27,
	kDNSRecordType_AAAA			= 28,
	kDNSRecordType_LOC			= 29,
	kDNSRecordType_NXT			= 30,
	kDNSRecordType_EID			= 31,
	kDNSRecordType_NIMLOC		= 32,
	kDNSRecordType_SRV			= 33,
	kDNSRecordType_ATMA			= 34,
	kDNSRecordType_NAPTR		= 35,
	kDNSRecordType_KX			= 36,
	kDNSRecordType_CERT			= 37,
	kDNSRecordType_A6			= 38,
	kDNSRecordType_DNAME		= 39,
	kDNSRecordType_SINK			= 40,
	kDNSRecordType_OPT			= 41,
	kDNSRecordType_APL			= 42,
	kDNSRecordType_DS			= 43,
	kDNSRecordType_SSHFP		= 44,
	kDNSRecordType_IPSECKEY		= 45,
	kDNSRecordType_RRSIG		= 46,
	kDNSRecordType_NSEC			= 47,
	kDNSRecordType_DNSKEY		= 48,
	kDNSRecordType_DHCID		= 49,
	kDNSRecordType_NSEC3		= 50,
	kDNSRecordType_NSEC3PARAM	= 51,
	kDNSRecordType_TLSA			= 52,
	kDNSRecordType_SMIMEA		= 53,
	kDNSRecordType_HIP			= 55,
	kDNSRecordType_NINFO		= 56,
	kDNSRecordType_RKEY			= 57,
	kDNSRecordType_TALINK		= 58,
	kDNSRecordType_CDS			= 59,
	kDNSRecordType_CDNSKEY		= 60,
	kDNSRecordType_OPENPGPKEY	= 61,
	kDNSRecordType_CSYNC		= 62,
	kDNSRecordType_ZONEMD		= 63,
	kDNSRecordType_SVCB			= 64,
	kDNSRecordType_HTTPS		= 65,
	kDNSRecordType_SPF			= 99,
	kDNSRecordType_UINFO		= 100,
	kDNSRecordType_UID			= 101,
	kDNSRecordType_GID			= 102,
	kDNSRecordType_UNSPEC		= 103,
	kDNSRecordType_NID			= 104,
	kDNSRecordType_L32			= 105,
	kDNSRecordType_L64			= 106,
	kDNSRecordType_LP			= 107,
	kDNSRecordType_EUI48		= 108,
	kDNSRecordType_EUI64		= 109,
	kDNSRecordType_TKEY			= 249,
	kDNSRecordType_TSIG			= 250,
	kDNSRecordType_IXFR			= 251,
	kDNSRecordType_AXFR			= 252,
	kDNSRecordType_MAILB		= 253,
	kDNSRecordType_MAILA		= 254,
	kDNSRecordType_ANY			= 255,
	kDNSRecordType_URI			= 256,
	kDNSRecordType_CAA			= 257,
	kDNSRecordType_AVC			= 258,
	kDNSRecordType_DOA			= 259,
	kDNSRecordType_AMTRELAY		= 260,
	kDNSRecordType_TA			= 32768,
	kDNSRecordType_DLV			= 32769,
	kDNSRecordType_Reserved		= 65535,

} dns_record_type_t;
#endif // #ifndef kDNSRecordType_Enum

// Record class
#ifndef kDNSClassType_Enum
#define kDNSClassType_Enum
typedef enum
{
	kDNSClassType_INVALID = 0,
	kDNSClassType_IN = 1,
	kDNSClassType_CHAOS = 3

} dns_class_type_t;
#endif //#ifndef kDNSClassType_Enum

//======================================================================================================================
// MARK: - Resource Record Constants

#ifndef NSEC3_FLAG_TYPE_ENUM
#define NSEC3_FLAG_TYPE_ENUM
// NSEC3 hash flags
// Taken from https://www.iana.org/assignments/dnssec-nsec3-parameters/dnssec-nsec3-parameters.xhtml.
typedef enum nsec3_flag_type {
	// Unassigned 0 - 6
	NSEC3_FLAG_OPT_OUT = 0x01
} nsec3_flag_type_t;
#endif

#ifndef NSEC3_HASH_ALGORITHM_ENUM
#define NSEC3_HASH_ALGORITHM_ENUM
// NSEC3 hash algorithms
// Taken from https://www.iana.org/assignments/dnssec-nsec3-parameters/dnssec-nsec3-parameters.xhtml.
typedef enum nsec3_hash_algorithm_type {
	// Reserved 0
	NSEC3_HASH_ALGORITHM_SHA_1	= 1
	// Unassigned 2 - 255
} nsec3_hash_algorithm_type_t;
#endif

//======================================================================================================================
// MARK: - Functions

/*!
 *	@brief
 *		Get the uint16_t integer from the network-byte-order bytes.
 *
 *	@param bytes
 *		The bytes that encode an uint16_t integer.
 *
 *	@result
 *		The uint16_t integer.
 */
uint16_t
get_uint16_from_bytes(const uint8_t * NONNULL bytes);

/*!
 *	@brief
 *		Get the uint32_t integer from the network-byte-order bytes.
 *
 *	@param bytes
 *		The bytes that encode an uint32_t integer.
 *
 *	@result
 *		The uint32_t integer.
 */
uint32_t
get_uint32_from_bytes(const uint8_t * NONNULL bytes);

/*!
 *	@brief
 *		Put the uint16_t integer into network-byte-order bytes.
 *
 *	@param u16
 *		The uint16_t integer.
 *
 *	@param ptr
 *		The pointer to the buffer that stores the bytes in network byte order.
 */
void
put_uint16_to_bytes(uint16_t u16, uint8_t * NULLABLE * NONNULL ptr);

/*!
 *	@brief
 *		Put the uint32_t integer into network-byte-order bytes.
 *
 *	@param u32
 *		The uint32_t integer.
 *
 *	@param ptr
 *		The pointer to the buffer that stores the bytes in network byte order.
 */
void
put_uint32_to_bytes(uint32_t u32, uint8_t * NULLABLE * NONNULL ptr);

/*!
 *	@brief
 *		Convert the bytes to its hex representation in C string.
 *
 *	@param bytes
 *		The bytes to be converted.
 *
 *	@param len
 *		The length of the bytes.
 *
 *	@param buffer
 *		The output buffer that holds the hex C string.
 *
 *	@param buffer_len
 *		The max size of the buffer.
 *
 *	@result
 *		If no error occurs, the next position of the hex C string end is returned. Otherwise, the value of <code>buffer</code> is returned.
 *
 *	@discussion
 *		To do conversion successfully, the length of the buffer has to be greater than 2 times of the bytes length.
 *
 */
char * NULLABLE
put_hex_from_bytes(const uint8_t * NULLABLE bytes, size_t len, char * const NONNULL buffer, size_t buffer_len);

/*!
 *	@brief
 *		Get the string description of the DNS record type.
 *
 *	@param type
 *		The DNS record type.
 *
 *	@result
 *		The string description.
 */
const char * NULLABLE
dns_record_type_value_to_string(uint16_t type);

/*!
 *	@brief
 *		Convert the integer error code to a string description.
 *
 *	@param error
 *		The error to convert.
 *
 *	@result
 *		The string description of the <code>error</code>.
 */
const char * NONNULL
dns_obj_error_get_error_description(dns_obj_error_t error);

#endif // DNS_OBJ_COMMON_H
