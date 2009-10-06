/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 * packet related headers.
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <time.h>

#ifdef HAVE_OPENSSL_BN_H
#include <openssl/bn.h>
#endif

#include "types.h"
#include "errors.h"

/** General-use structure for variable-length data
 */

typedef struct {
	size_t          len;
	unsigned char  *contents;
	unsigned char	mmapped;	/* contents need an munmap(2) */
} __ops_data_t;

/************************************/
/* Packet Tags - RFC4880, 4.2 */
/************************************/

/** Packet Tag - Bit 7 Mask (this bit is always set).
 * The first byte of a packet is the "Packet Tag".  It always
 * has bit 7 set.  This is the mask for it.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_ALWAYS_SET		0x80

/** Packet Tag - New Format Flag.
 * Bit 6 of the Packet Tag is the packet format indicator.
 * If it is set, the new format is used, if cleared the
 * old format is used.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_NEW_FORMAT		0x40


/** Old Packet Format: Mask for content tag.
 * In the old packet format bits 5 to 2 (including)
 * are the content tag.  This is the mask to apply
 * to the packet tag.  Note that you need to
 * shift by #OPS_PTAG_OF_CONTENT_TAG_SHIFT bits.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_OF_CONTENT_TAG_MASK	0x3c
/** Old Packet Format: Offset for the content tag.
 * As described at #OPS_PTAG_OF_CONTENT_TAG_MASK the
 * content tag needs to be shifted after being masked
 * out from the Packet Tag.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_OF_CONTENT_TAG_SHIFT	2
/** Old Packet Format: Mask for length type.
 * Bits 1 and 0 of the packet tag are the length type
 * in the old packet format.
 *
 * See #__ops_ptag_of_lt_t for the meaning of the values.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_OF_LENGTH_TYPE_MASK	0x03


/** Old Packet Format Lengths.
 * Defines the meanings of the 2 bits for length type in the
 * old packet format.
 *
 * \see RFC4880 4.2.1
 */
typedef enum {
	OPS_PTAG_OLD_LEN_1 = 0x00,	/* Packet has a 1 byte length -
					 * header is 2 bytes long. */
	OPS_PTAG_OLD_LEN_2 = 0x01,	/* Packet has a 2 byte length -
					 * header is 3 bytes long. */
	OPS_PTAG_OLD_LEN_4 = 0x02,	/* Packet has a 4 byte
						 * length - header is 5 bytes
						 * long. */
	OPS_PTAG_OLD_LEN_INDETERMINATE = 0x03	/* Packet has a
						 * indeterminate length. */
} __ops_ptag_of_lt_t;


/** New Packet Format: Mask for content tag.
 * In the new packet format the 6 rightmost bits
 * are the content tag.  This is the mask to apply
 * to the packet tag.  Note that you need to
 * shift by #OPS_PTAG_NF_CONTENT_TAG_SHIFT bits.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_NF_CONTENT_TAG_MASK	0x3f
/** New Packet Format: Offset for the content tag.
 * As described at #OPS_PTAG_NF_CONTENT_TAG_MASK the
 * content tag needs to be shifted after being masked
 * out from the Packet Tag.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_NF_CONTENT_TAG_SHIFT	0

/* PTag Content Tags */
/***************************/

/** Package Tags (aka Content Tags) and signature subpacket types.
 * This enumerates all rfc-defined packet tag values and the
 * signature subpacket type values that we understand.
 *
 * \see RFC4880 4.3
 * \see RFC4880 5.2.3.1
 */
typedef enum {
	OPS_PTAG_CT_RESERVED = 0,	/* Reserved - a packet tag must
					 * not have this value */
	OPS_PTAG_CT_PK_SESSION_KEY = 1,	/* Public-Key Encrypted Session
					 * Key Packet */
	OPS_PTAG_CT_SIGNATURE = 2,	/* Signature Packet */
	OPS_PTAG_CT_SK_SESSION_KEY = 3,	/* Symmetric-Key Encrypted Session
					 * Key Packet */
	OPS_PTAG_CT_1_PASS_SIG = 4,	/* One-Pass Signature
						 * Packet */
	OPS_PTAG_CT_SECRET_KEY = 5,	/* Secret Key Packet */
	OPS_PTAG_CT_PUBLIC_KEY = 6,	/* Public Key Packet */
	OPS_PTAG_CT_SECRET_SUBKEY = 7,	/* Secret Subkey Packet */
	OPS_PTAG_CT_COMPRESSED = 8,	/* Compressed Data Packet */
	OPS_PTAG_CT_SE_DATA = 9,/* Symmetrically Encrypted Data Packet */
	OPS_PTAG_CT_MARKER = 10,/* Marker Packet */
	OPS_PTAG_CT_LITDATA = 11,	/* Literal Data Packet */
	OPS_PTAG_CT_TRUST = 12,	/* Trust Packet */
	OPS_PTAG_CT_USER_ID = 13,	/* User ID Packet */
	OPS_PTAG_CT_PUBLIC_SUBKEY = 14,	/* Public Subkey Packet */
	OPS_PTAG_CT_RESERVED2 = 15,	/* reserved */
	OPS_PTAG_CT_RESERVED3 = 16,	/* reserved */
	OPS_PTAG_CT_USER_ATTR = 17,	/* User Attribute Packet */
	OPS_PTAG_CT_SE_IP_DATA = 18,	/* Sym. Encrypted and Integrity
					 * Protected Data Packet */
	OPS_PTAG_CT_MDC = 19,	/* Modification Detection Code Packet */

	OPS_PARSER_PTAG = 0x100,/* Internal Use: The packet is the "Packet
				 * Tag" itself - used when callback sends
				 * back the PTag. */
	OPS_PTAG_RAW_SS = 0x101,/* Internal Use: content is raw sig subtag */
	OPS_PTAG_SS_ALL = 0x102,/* Internal Use: select all subtags */
	OPS_PARSER_PACKET_END = 0x103,

	/* signature subpackets (0x200-2ff) (type+0x200) */
	/* only those we can parse are listed here */
	OPS_PTAG_SIG_SUBPKT_BASE = 0x200,	/* Base for signature
							 * subpacket types - All
							 * signature type values
							 * are relative to this
							 * value. */
	OPS_PTAG_SS_CREATION_TIME = 0x200 + 2,	/* signature creation time */
	OPS_PTAG_SS_EXPIRATION_TIME = 0x200 + 3,	/* signature
							 * expiration time */

	OPS_PTAG_SS_EXPORT_CERT = 0x200 + 4,	/* exportable certification */
	OPS_PTAG_SS_TRUST = 0x200 + 5,	/* trust signature */
	OPS_PTAG_SS_REGEXP = 0x200 + 6,	/* regular expression */
	OPS_PTAG_SS_REVOCABLE = 0x200 + 7,	/* revocable */
	OPS_PTAG_SS_KEY_EXPIRY = 0x200 + 9,	/* key expiration
							 * time */
	OPS_PTAG_SS_RESERVED = 0x200 + 10,	/* reserved */
	OPS_PTAG_SS_PREFERRED_SKA = 0x200 + 11,	/* preferred symmetric
						 * algs */
	OPS_PTAG_SS_REVOCATION_KEY = 0x200 + 12,	/* revocation key */
	OPS_PTAG_SS_ISSUER_KEY_ID = 0x200 + 16,	/* issuer key ID */
	OPS_PTAG_SS_NOTATION_DATA = 0x200 + 20,	/* notation data */
	OPS_PTAG_SS_PREFERRED_HASH = 0x200 + 21,	/* preferred hash
							 * algs */
	OPS_PTAG_SS_PREF_COMPRESS = 0x200 + 22,	/* preferred
							 * compression
							 * algorithms */
	OPS_PTAG_SS_KEYSERV_PREFS = 0x200 + 23,	/* key server
							 * preferences */
	OPS_PTAG_SS_PREF_KEYSERV = 0x200 + 24,	/* Preferred Key
							 * Server */
	OPS_PTAG_SS_PRIMARY_USER_ID = 0x200 + 25,	/* primary User ID */
	OPS_PTAG_SS_POLICY_URI = 0x200 + 26,	/* Policy URI */
	OPS_PTAG_SS_KEY_FLAGS = 0x200 + 27,	/* key flags */
	OPS_PTAG_SS_SIGNERS_USER_ID = 0x200 + 28,	/* Signer's User ID */
	OPS_PTAG_SS_REVOCATION_REASON = 0x200 + 29,	/* reason for
							 * revocation */
	OPS_PTAG_SS_FEATURES = 0x200 + 30,	/* features */
	OPS_PTAG_SS_SIGNATURE_TARGET = 0x200 + 31,	/* signature target */
	OPS_PTAG_SS_EMBEDDED_SIGNATURE = 0x200 + 32,	/* embedded signature */

	OPS_PTAG_SS_USERDEFINED00 = 0x200 + 100,	/* internal or
							 * user-defined */
	OPS_PTAG_SS_USERDEFINED01 = 0x200 + 101,
	OPS_PTAG_SS_USERDEFINED02 = 0x200 + 102,
	OPS_PTAG_SS_USERDEFINED03 = 0x200 + 103,
	OPS_PTAG_SS_USERDEFINED04 = 0x200 + 104,
	OPS_PTAG_SS_USERDEFINED05 = 0x200 + 105,
	OPS_PTAG_SS_USERDEFINED06 = 0x200 + 106,
	OPS_PTAG_SS_USERDEFINED07 = 0x200 + 107,
	OPS_PTAG_SS_USERDEFINED08 = 0x200 + 108,
	OPS_PTAG_SS_USERDEFINED09 = 0x200 + 109,
	OPS_PTAG_SS_USERDEFINED10 = 0x200 + 110,

	/* pseudo content types */
	OPS_PTAG_CT_LITDATA_HEADER = 0x300,
	OPS_PTAG_CT_LITDATA_BODY = 0x300 + 1,
	OPS_PTAG_CT_SIGNATURE_HEADER = 0x300 + 2,
	OPS_PTAG_CT_SIGNATURE_FOOTER = 0x300 + 3,
	OPS_PTAG_CT_ARMOUR_HEADER = 0x300 + 4,
	OPS_PTAG_CT_ARMOUR_TRAILER = 0x300 + 5,
	OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER = 0x300 + 6,
	OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY = 0x300 + 7,
	OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER = 0x300 + 8,
	OPS_PTAG_CT_UNARMOURED_TEXT = 0x300 + 9,
	OPS_PTAG_CT_ENCRYPTED_SECRET_KEY = 0x300 + 10,	/* In this case the
							 * algorithm specific
							 * fields will not be
							 * initialised */
	OPS_PTAG_CT_SE_DATA_HEADER = 0x300 + 11,
	OPS_PTAG_CT_SE_DATA_BODY = 0x300 + 12,
	OPS_PTAG_CT_SE_IP_DATA_HEADER = 0x300 + 13,
	OPS_PTAG_CT_SE_IP_DATA_BODY = 0x300 + 14,
	OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY = 0x300 + 15,

	/* commands to the callback */
	OPS_GET_PASSPHRASE = 0x400,
	OPS_GET_SECKEY = 0x400 + 1,

	/* Errors */
	OPS_PARSER_ERROR = 0x500,	/* Internal Use: Parser Error */
	OPS_PARSER_ERRCODE = 0x500 + 1	/* Internal Use: Parser Error
					 * with errcode returned */
} __ops_content_tag_t;

typedef __ops_content_tag_t __ops_packet_tag_t;
typedef __ops_content_tag_t __ops_ss_type_t;

/** Structure to hold one parse error string. */
typedef struct {
	const char     *error;	/* error message. */
} __ops_parser_error_t;

/** Structure to hold one error code */
typedef struct {
	__ops_errcode_t   errcode;
} __ops_parser_errcode_t;

/** Structure to hold one packet tag.
 * \see RFC4880 4.2
 */
typedef struct {
	unsigned        new_format;	/* Whether this packet tag is new
					 * (1) or old format (0) */
	unsigned        type;	/* content_tag value - See
					 * #__ops_content_tag_t for meanings */
	__ops_ptag_of_lt_t length_type;	/* Length type (#__ops_ptag_of_lt_t)
					 * - only if this packet tag is old
					 * format.  Set to 0 if new format. */
	unsigned        length;	/* The length of the packet.  This value
				 * is set when we read and compute the length
				 * information, not at the same moment we
				 * create the packet tag structure. Only
	 * defined if #readc is set. *//* XXX: Ben, is this correct? */
	unsigned        position;	/* The position (within the
					 * current reader) of the packet */
	unsigned	size;	/* number of bits */
} __ops_ptag_t;

/** Public Key Algorithm Numbers.
 * OpenPGP assigns a unique Algorithm Number to each algorithm that is part of OpenPGP.
 *
 * This lists algorithm numbers for public key algorithms.
 *
 * \see RFC4880 9.1
 */
typedef enum {
	OPS_PKA_NOTHING	= 0,	/* No PKA */
	OPS_PKA_RSA = 1,	/* RSA (Encrypt or Sign) */
	OPS_PKA_RSA_ENCRYPT_ONLY = 2,	/* RSA Encrypt-Only (deprecated -
					 * \see RFC4880 13.5) */
	OPS_PKA_RSA_SIGN_ONLY = 3,	/* RSA Sign-Only (deprecated -
					 * \see RFC4880 13.5) */
	OPS_PKA_ELGAMAL = 16,	/* Elgamal (Encrypt-Only) */
	OPS_PKA_DSA = 17,	/* DSA (Digital Signature Algorithm) */
	OPS_PKA_RESERVED_ELLIPTIC_CURVE = 18,	/* Reserved for Elliptic
						 * Curve */
	OPS_PKA_RESERVED_ECDSA = 19,	/* Reserved for ECDSA */
	OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN = 20,	/* Deprecated. */
	OPS_PKA_RESERVED_DH = 21,	/* Reserved for Diffie-Hellman
					 * (X9.42, as defined for
					 * IETF-S/MIME) */
	OPS_PKA_PRIVATE00 = 100,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE01 = 101,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE02 = 102,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE03 = 103,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE04 = 104,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE05 = 105,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE06 = 106,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE07 = 107,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE08 = 108,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE09 = 109,/* Private/Experimental Algorithm */
	OPS_PKA_PRIVATE10 = 110	/* Private/Experimental Algorithm */
} __ops_pubkey_alg_t;

/** Structure to hold one DSA public key params.
 *
 * \see RFC4880 5.5.2
 */
typedef struct {
	BIGNUM         *p;	/* DSA prime p */
	BIGNUM         *q;	/* DSA group order q */
	BIGNUM         *g;	/* DSA group generator g */
	BIGNUM         *y;	/* DSA public key value y (= g^x mod p
				 * with x being the secret) */
} __ops_dsa_pubkey_t;

/** Structure to hold an RSA public key.
 *
 * \see RFC4880 5.5.2
 */
typedef struct {
	BIGNUM         *n;	/* RSA public modulus n */
	BIGNUM         *e;	/* RSA public encryption exponent e */
} __ops_rsa_pubkey_t;

/** Structure to hold an ElGamal public key params.
 *
 * \see RFC4880 5.5.2
 */
typedef struct {
	BIGNUM         *p;	/* ElGamal prime p */
	BIGNUM         *g;	/* ElGamal group generator g */
	BIGNUM         *y;	/* ElGamal public key value y (= g^x mod p
				 * with x being the secret) */
} __ops_elgamal_pubkey_t;

/** Union to hold public key params of any algorithm */
typedef union {
	__ops_dsa_pubkey_t dsa;	/* A DSA public key */
	__ops_rsa_pubkey_t rsa;	/* An RSA public key */
	__ops_elgamal_pubkey_t elgamal;	/* An ElGamal public key */
} __ops_pubkey_union_t;

/** Version.
 * OpenPGP has two different protocol versions: version 3 and version 4.
 *
 * \see RFC4880 5.2
 */
typedef enum {
	OPS_V2 = 2,		/* Version 2 (essentially the same as v3) */
	OPS_V3 = 3,		/* Version 3 */
	OPS_V4 = 4		/* Version 4 */
} __ops_version_t;

/** Structure to hold a pgp public key */
typedef struct {
	__ops_version_t		version;/* version of the key (v3, v4...) */
	time_t			birthtime;
		/* when the key was created.  Note that
		 * interpretation varies with key version.  */
	unsigned		days_valid;
		/* validity period of the key in days since
		* creation.  A value of 0 has a special meaning
		* indicating this key does not expire.  Only used with
		* v3 keys.  */
	__ops_pubkey_alg_t	alg;	/* Public Key Algorithm type */
	__ops_pubkey_union_t	key;	/* Public Key Parameters */
} __ops_pubkey_t;

/** Structure to hold data for one RSA secret key
 */
typedef struct {
	BIGNUM         *d;
	BIGNUM         *p;
	BIGNUM         *q;
	BIGNUM         *u;
} __ops_rsa_seckey_t;

/** __ops_dsa_seckey_t */
typedef struct {
	BIGNUM         *x;
} __ops_dsa_seckey_t;

/** __ops_seckey_union_t */
typedef union {
	__ops_rsa_seckey_t rsa;
	__ops_dsa_seckey_t dsa;
} __ops_seckey_union_t;

/** s2k_usage_t
 */
typedef enum {
	OPS_S2KU_NONE = 0,
	OPS_S2KU_ENCRYPTED_AND_HASHED = 254,
	OPS_S2KU_ENCRYPTED = 255
} __ops_s2k_usage_t;

/** s2k_specifier_t
 */
typedef enum {
	OPS_S2KS_SIMPLE = 0,
	OPS_S2KS_SALTED = 1,
	OPS_S2KS_ITERATED_AND_SALTED = 3
} __ops_s2k_specifier_t;

/** Symmetric Key Algorithm Numbers.
 * OpenPGP assigns a unique Algorithm Number to each algorithm that is
 * part of OpenPGP.
 *
 * This lists algorithm numbers for symmetric key algorithms.
 *
 * \see RFC4880 9.2
 */
typedef enum {
	OPS_SA_PLAINTEXT = 0,	/* Plaintext or unencrypted data */
	OPS_SA_IDEA = 1,	/* IDEA */
	OPS_SA_TRIPLEDES = 2,	/* TripleDES */
	OPS_SA_CAST5 = 3,	/* CAST5 */
	OPS_SA_BLOWFISH = 4,	/* Blowfish */
	OPS_SA_AES_128 = 7,	/* AES with 128-bit key (AES) */
	OPS_SA_AES_192 = 8,	/* AES with 192-bit key */
	OPS_SA_AES_256 = 9,	/* AES with 256-bit key */
	OPS_SA_TWOFISH = 10	/* Twofish with 256-bit key (TWOFISH) */
} __ops_symm_alg_t;

/** Hashing Algorithm Numbers.
 * OpenPGP assigns a unique Algorithm Number to each algorithm that is
 * part of OpenPGP.
 *
 * This lists algorithm numbers for hash algorithms.
 *
 * \see RFC4880 9.4
 */
typedef enum {
	OPS_HASH_UNKNOWN = -1,	/* used to indicate errors */
	OPS_HASH_MD5 = 1,	/* MD5 */
	OPS_HASH_SHA1 = 2,	/* SHA-1 */
	OPS_HASH_RIPEMD = 3,	/* RIPEMD160 */

	OPS_HASH_SHA256 = 8,	/* SHA256 */
	OPS_HASH_SHA384 = 9,	/* SHA384 */
	OPS_HASH_SHA512 = 10,	/* SHA512 */
	OPS_HASH_SHA224 = 11	/* SHA224 */
} __ops_hash_alg_t;

void   __ops_calc_mdc_hash(const unsigned char *,
			const size_t,
			const unsigned char *,
			const unsigned int,
			unsigned char *);
unsigned   __ops_is_hash_alg_supported(const __ops_hash_alg_t *);

/* Maximum block size for symmetric crypto */
#define OPS_MAX_BLOCK_SIZE	16

/* Maximum key size for symmetric crypto */
#define OPS_MAX_KEY_SIZE	32

/* Salt size for hashing */
#define OPS_SALT_SIZE		8

/* Max hash size */
#define OPS_MAX_HASH_SIZE	64

/** __ops_seckey_t
 */
typedef struct {
	__ops_pubkey_t			pubkey;
	__ops_s2k_usage_t		s2k_usage;
	__ops_s2k_specifier_t		s2k_specifier;
	__ops_symm_alg_t		alg;
	__ops_hash_alg_t		hash_alg;
	unsigned char			salt[OPS_SALT_SIZE];
	unsigned			octetc;
	unsigned char			iv[OPS_MAX_BLOCK_SIZE];
	__ops_seckey_union_t		key;
	unsigned			checksum;
	unsigned char		       *checkhash;
} __ops_seckey_t;

/** Structure to hold one trust packet's data */

typedef struct {
	__ops_data_t      data;	/* Trust Packet */
} __ops_trust_t;

/** Structure to hold one user id */
typedef struct {
	unsigned char  *userid;/* User ID - UTF-8 string */
} __ops_userid_t;

/** Structure to hold one user attribute */
typedef struct {
	__ops_data_t      data;	/* User Attribute */
} __ops_userattr_t;

/** Signature Type.
 * OpenPGP defines different signature types that allow giving
 * different meanings to signatures.  Signature types include 0x10 for
 * generitc User ID certifications (used when Ben signs Weasel's key),
 * Subkey binding signatures, document signatures, key revocations,
 * etc.
 *
 * Different types are used in different places, and most make only
 * sense in their intended location (for instance a subkey binding has
 * no place on a UserID).
 *
 * \see RFC4880 5.2.1
 */
typedef enum {
	OPS_SIG_BINARY = 0x00,	/* Signature of a binary document */
	OPS_SIG_TEXT = 0x01,	/* Signature of a canonical text document */
	OPS_SIG_STANDALONE = 0x02,	/* Standalone signature */

	OPS_CERT_GENERIC = 0x10,/* Generic certification of a User ID and
				 * Public Key packet */
	OPS_CERT_PERSONA = 0x11,/* Persona certification of a User ID and
				 * Public Key packet */
	OPS_CERT_CASUAL = 0x12,	/* Casual certification of a User ID and
				 * Public Key packet */
	OPS_CERT_POSITIVE = 0x13,	/* Positive certification of a
					 * User ID and Public Key packet */

	OPS_SIG_SUBKEY = 0x18,	/* Subkey Binding Signature */
	OPS_SIG_PRIMARY = 0x19,	/* Primary Key Binding Signature */
	OPS_SIG_DIRECT = 0x1f,	/* Signature directly on a key */

	OPS_SIG_REV_KEY = 0x20,	/* Key revocation signature */
	OPS_SIG_REV_SUBKEY = 0x28,	/* Subkey revocation signature */
	OPS_SIG_REV_CERT = 0x30,/* Certification revocation signature */

	OPS_SIG_TIMESTAMP = 0x40,	/* Timestamp signature */

	OPS_SIG_3RD_PARTY = 0x50/* Third-Party Confirmation signature */
} __ops_sig_type_t;

/** Struct to hold params of an RSA signature */
typedef struct {
	BIGNUM         *sig;	/* the signature value (m^d % n) */
} __ops_rsa_sig_t;

/** Struct to hold params of a DSA signature */
typedef struct {
	BIGNUM         *r;	/* DSA value r */
	BIGNUM         *s;	/* DSA value s */
} __ops_dsa_sig_t;

/** __ops_elgamal_signature_t */
typedef struct {
	BIGNUM         *r;
	BIGNUM         *s;
} __ops_elgamal_sig_t;

/** Struct to hold data for a private/experimental signature */
typedef struct {
	__ops_data_t      data;
} __ops_unknown_sig_t;

/** Union to hold signature params of any algorithm */
typedef union {
	__ops_rsa_sig_t rsa;/* An RSA Signature */
	__ops_dsa_sig_t dsa;/* A DSA Signature */
	__ops_elgamal_sig_t elgamal;	/* deprecated */
	__ops_unknown_sig_t unknown;	/* private or experimental */
} __ops_sig_union_t;

#define OPS_KEY_ID_SIZE		8
#define OPS_FINGERPRINT_SIZE	20

/** Struct to hold a signature packet.
 *
 * \see RFC4880 5.2.2
 * \see RFC4880 5.2.3
 */
typedef struct {
	__ops_version_t   version;/* signature version number */
	__ops_sig_type_t  type;	/* signature type value */
	time_t          birthtime;	/* creation time of the signature */
	unsigned char   signer_id[OPS_KEY_ID_SIZE];	/* Eight-octet key ID
							 * of signer */
	__ops_pubkey_alg_t key_alg;	/* public key
							 * algorithm number */
	__ops_hash_alg_t hash_alg;	/* hashing algorithm
						 * number */
	__ops_sig_union_t sig;	/* signature params */
	size_t          v4_hashlen;
	unsigned char  *v4_hashed;
	unsigned   birthtime_set:1;
	unsigned   signer_id_set:1;
} __ops_sig_info_t;

/** Struct used when parsing a signature */
typedef struct __ops_sig_t {
	__ops_sig_info_t info;	/* The signature information */
	/* The following fields are only used while parsing the signature */
	unsigned char   hash2[2];	/* high 2 bytes of hashed value -
					 * for quick test */
	size_t          v4_hashstart;	/* only valid if accumulate
						 * is set */
	__ops_hash_t     *hash;	/* if set, the hash filled in for the data
				 * so far */
} __ops_sig_t;

/** The raw bytes of a signature subpacket */

typedef struct {
	__ops_content_tag_t	 tag;
	size_t          	 length;
	unsigned char		*raw;
} __ops_ss_raw_t;

/** Signature Subpacket : Trust Level */

typedef struct {
	unsigned char		level;	/* Trust Level */
	unsigned char		amount;	/* Amount */
} __ops_ss_trust_t;

/** Signature Subpacket : Revocable */
typedef struct {
	unsigned		revocable;
} __ops_ss_revocable_t;

/** Signature Subpacket : Time */
typedef struct {
	time_t			time;
} __ops_ss_time_t;

/** Signature Subpacket : Key ID */
typedef struct {
	unsigned char		key_id[OPS_KEY_ID_SIZE];
} __ops_ss_key_id_t;

/** Signature Subpacket : Notation Data */
typedef struct {
	__ops_data_t		flags;
	__ops_data_t		name;
	__ops_data_t		value;
} __ops_ss_notation_t;

/** Signature Subpacket : User Defined */
typedef struct {
	__ops_data_t		data;
} __ops_ss_userdef_t;

/** Signature Subpacket : Unknown */
typedef struct {
	__ops_data_t		data;
} __ops_ss_unknown_t;

/** Signature Subpacket : Preferred Symmetric Key Algorithm */
typedef struct {
	__ops_data_t		data;
	/*
	 * Note that value 0 may represent the plaintext algorithm so we
	 * cannot expect data->contents to be a null-terminated list
	 */
} __ops_ss_skapref_t;

/** Signature Subpacket : Preferrred Hash Algorithm */
typedef struct {
	__ops_data_t      data;
} __ops_ss_hashpref_t;

/** Signature Subpacket : Preferred Compression */
typedef struct {
	__ops_data_t      data;
} __ops_ss_zpref_t;

/** Signature Subpacket : Key Flags */
typedef struct {
	__ops_data_t      data;
} __ops_ss_key_flags_t;

/** Signature Subpacket : Key Server Preferences */
typedef struct {
	__ops_data_t      data;
} __ops_ss_key_server_prefs_t;

/** Signature Subpacket : Features */
typedef struct {
	__ops_data_t      data;
} __ops_ss_features_t;

/** Signature Subpacket : Signature Target */
typedef struct {
	__ops_pubkey_alg_t	pka_alg;
	__ops_hash_alg_t	hash_alg;
	__ops_data_t		hash;
} __ops_ss_sig_target_t;

/** Signature Subpacket : Embedded Signature */
typedef struct {
	__ops_data_t      sig;
} __ops_ss_embedded_sig_t;

/** __ops_subpacket_t */

typedef struct __ops_subpacket_t {
	size_t          length;
	unsigned char  *raw;
} __ops_subpacket_t;

/** Types of Compression */
typedef enum {
	OPS_C_NONE = 0,
	OPS_C_ZIP = 1,
	OPS_C_ZLIB = 2,
	OPS_C_BZIP2 = 3
} __ops_compression_type_t;

/*
 * unlike most structures, this will feed its data as a stream to the
 * application instead of directly including it
 */
/** __ops_compressed_t */
typedef struct {
	__ops_compression_type_t type;
} __ops_compressed_t;

/** __ops_one_pass_sig_t */
typedef struct {
	unsigned char		version;
	__ops_sig_type_t	sig_type;
	__ops_hash_alg_t	hash_alg;
	__ops_pubkey_alg_t	key_alg;
	unsigned char		keyid[OPS_KEY_ID_SIZE];
	unsigned		nested;
} __ops_one_pass_sig_t;

/** Signature Subpacket : Primary User ID */
typedef struct {
	unsigned   primary_userid;
} __ops_ss_primary_userid_t;

/** Signature Subpacket : Regexp */
typedef struct {
	char           *regexp;
} __ops_ss_regexp_t;

/** Signature Subpacket : Policy URL */
typedef struct {
	char           *url;
} __ops_ss_policy_t;

/** Signature Subpacket : Preferred Key Server */
typedef struct {
	char           *name;
} __ops_ss_keyserv_t;

/** Signature Subpacket : Revocation Key */
typedef struct {
	unsigned char   class;
	unsigned char   algid;
	unsigned char   fingerprint[OPS_FINGERPRINT_SIZE];
} __ops_ss_revocation_key_t;

/** Signature Subpacket : Revocation Reason */
typedef struct {
	unsigned char   code;
	char           *reason;
} __ops_ss_revocation_t;

/** litdata_type_t */
typedef enum {
	OPS_LDT_BINARY = 'b',
	OPS_LDT_TEXT = 't',
	OPS_LDT_UTF8 = 'u',
	OPS_LDT_LOCAL = 'l',
	OPS_LDT_LOCAL2 = '1'
} __ops_litdata_type_t;

/** __ops_litdata_header_t */
typedef struct {
	__ops_litdata_type_t	format;
	char			filename[256];
	time_t			mtime;
} __ops_litdata_header_t;

/** __ops_litdata_body_t */
typedef struct {
	unsigned         length;
	unsigned char   *data;
	void		*mem;		/* __ops_memory_t pointer */
} __ops_litdata_body_t;

/** __ops_mdc_t */
typedef struct {
	unsigned	 length;
	unsigned char   *data;
} __ops_mdc_t;

/** __ops_header_var_t */
typedef struct {
	char           *key;
	char           *value;
} __ops_header_var_t;

/** __ops_headers_t */
typedef struct {
	__ops_header_var_t	*headers;
	unsigned	         headerc;
} __ops_headers_t;

/** __ops_armour_header_t */
typedef struct {
	const char	*type;
	__ops_headers_t	 headers;
} __ops_armour_header_t;

/** __ops_armour_trailer_t */
typedef struct {
	const char     *type;
} __ops_armour_trailer_t;

/** __ops_cleartext_head_t */
typedef struct {
	__ops_headers_t   headers;
} __ops_cleartext_head_t;

/** __ops_cleartext_body_t */
typedef struct {
	unsigned        length;
	unsigned char   data[8192];	/* \todo fix hard-coded value? */
} __ops_cleartext_body_t;

/** __ops_cleartext_trailer_t */
typedef struct {
	struct _ops_hash_t *hash;	/* This will not have been
					 * finalised, but will have seen all
					 * the cleartext data in canonical
					 * form */
} __ops_cleartext_trailer_t;

/** __ops_unarmoured_text_t */
typedef struct {
	unsigned        length;
	unsigned char  *data;
} __ops_unarmoured_text_t;

typedef enum {
	SE_IP_DATA_VERSION = 1
} __ops_se_ip_data_version_t;

typedef enum {
	OPS_PKSK_V3 = 3
} __ops_pk_sesskey_version_t;

/** __ops_pk_sesskey_params_rsa_t */
typedef struct {
	BIGNUM         *encrypted_m;
	BIGNUM         *m;
} __ops_pk_sesskey_params_rsa_t;

/** __ops_pk_sesskey_params_elgamal_t */
typedef struct {
	BIGNUM         *g_to_k;
	BIGNUM         *encrypted_m;
} __ops_pk_sesskey_params_elgamal_t;

/** __ops_pk_sesskey_params_t */
typedef union {
	__ops_pk_sesskey_params_rsa_t rsa;
	__ops_pk_sesskey_params_elgamal_t elgamal;
} __ops_pk_sesskey_params_t;

/** __ops_pk_sesskey_t */
typedef struct {
	__ops_pk_sesskey_version_t version;
	unsigned char   key_id[OPS_KEY_ID_SIZE];
	__ops_pubkey_alg_t alg;
	__ops_pk_sesskey_params_t params;
	__ops_symm_alg_t symm_alg;
	unsigned char   key[OPS_MAX_KEY_SIZE];
	unsigned short  checksum;
} __ops_pk_sesskey_t;

/** __ops_seckey_passphrase_t */
typedef struct {
	const __ops_seckey_t *seckey;
	char          **passphrase;	/* point somewhere that gets filled
					 * in to work around constness of
					 * content */
} __ops_seckey_passphrase_t;

typedef enum {
	OPS_SE_IP_V1 = 1
} __ops_se_ip_version_t;

/** __ops_se_ip_data_header_t */
typedef struct {
	__ops_se_ip_version_t version;
} __ops_se_ip_data_header_t;

/** __ops_se_ip_data_body_t */
typedef struct {
	unsigned        length;
	unsigned char  *data;	/* \todo remember to free this */
} __ops_se_ip_data_body_t;

/** __ops_se_data_body_t */
typedef struct {
	unsigned        length;
	unsigned char   data[8192];	/* \todo parameterise this! */
} __ops_se_data_body_t;

/** __ops_get_seckey_t */
typedef struct {
	const __ops_seckey_t **seckey;
	const __ops_pk_sesskey_t *pk_sesskey;
} __ops_get_seckey_t;

/** __ops_parser_union_content_t */
typedef union {
	__ops_parser_error_t		error;
	__ops_parser_errcode_t		errcode;
	__ops_ptag_t			ptag;
	__ops_pubkey_t			pubkey;
	__ops_trust_t			trust;
	__ops_userid_t			userid;
	__ops_userattr_t		userattr;
	__ops_sig_t			sig;
	__ops_ss_raw_t			ss_raw;
	__ops_ss_trust_t		ss_trust;
	__ops_ss_revocable_t		ss_revocable;
	__ops_ss_time_t			ss_time;
	__ops_ss_key_id_t		ss_issuer;
	__ops_ss_notation_t		ss_notation;
	__ops_subpacket_t		packet;
	__ops_compressed_t		compressed;
	__ops_one_pass_sig_t		one_pass_sig;
	__ops_ss_skapref_t		ss_skapref;
	__ops_ss_hashpref_t		ss_hashpref;
	__ops_ss_zpref_t		ss_zpref;
	__ops_ss_key_flags_t		ss_key_flags;
	__ops_ss_key_server_prefs_t	ss_key_server_prefs;
	__ops_ss_primary_userid_t	ss_primary_userid;
	__ops_ss_regexp_t		ss_regexp;
	__ops_ss_policy_t		ss_policy;
	__ops_ss_keyserv_t		ss_keyserv;
	__ops_ss_revocation_key_t	ss_revocation_key;
	__ops_ss_userdef_t		ss_userdef;
	__ops_ss_unknown_t		ss_unknown;
	__ops_litdata_header_t		litdata_header;
	__ops_litdata_body_t		litdata_body;
	__ops_mdc_t			mdc;
	__ops_ss_features_t		ss_features;
	__ops_ss_sig_target_t		ss_sig_target;
	__ops_ss_embedded_sig_t		ss_embedded_sig;
	__ops_ss_revocation_t		ss_revocation;
	__ops_seckey_t			seckey;
	__ops_userid_t			ss_signer;
	__ops_armour_header_t		armour_header;
	__ops_armour_trailer_t		armour_trailer;
	__ops_cleartext_head_t		cleartext_head;
	__ops_cleartext_body_t		cleartext_body;
	__ops_cleartext_trailer_t	cleartext_trailer;
	__ops_unarmoured_text_t		unarmoured_text;
	__ops_pk_sesskey_t		pk_sesskey;
	__ops_seckey_passphrase_t	skey_passphrase;
	__ops_se_ip_data_header_t	se_ip_data_header;
	__ops_se_ip_data_body_t		se_ip_data_body;
	__ops_se_data_body_t		se_data_body;
	__ops_get_seckey_t		get_seckey;
} __ops_contents_t;

/** __ops_packet_t */
struct __ops_packet_t {
	__ops_content_tag_t	tag;		/* type of contents */
	unsigned char		critical;	/* for sig subpackets */
	__ops_contents_t	u;		/* union for contents */
};

/** __ops_fingerprint_t */
typedef struct {
	unsigned char   fingerprint[OPS_FINGERPRINT_SIZE];
	unsigned        length;
} __ops_fingerprint_t;

void __ops_init(void);
void __ops_finish(void);
void __ops_keyid(unsigned char *, const size_t, const __ops_pubkey_t *);
void __ops_fingerprint(__ops_fingerprint_t *, const __ops_pubkey_t *);
void __ops_pubkey_free(__ops_pubkey_t *);
void __ops_userid_free(__ops_userid_t *);
void __ops_userattr_free(__ops_userattr_t *);
void __ops_sig_free(__ops_sig_t *);
void __ops_trust_free(__ops_trust_t *);
void __ops_ss_skapref_free(__ops_ss_skapref_t *);
void __ops_ss_hashpref_free(__ops_ss_hashpref_t *);
void __ops_ss_zpref_free(__ops_ss_zpref_t *);
void __ops_ss_key_flags_free(__ops_ss_key_flags_t *);
void __ops_ss_key_server_prefs_free(__ops_ss_key_server_prefs_t *);
void __ops_ss_features_free(__ops_ss_features_t *);
void __ops_ss_notation_free(__ops_ss_notation_t *);
void __ops_ss_policy_free(__ops_ss_policy_t *);
void __ops_ss_keyserv_free(__ops_ss_keyserv_t *);
void __ops_ss_regexp_free(__ops_ss_regexp_t *);
void __ops_ss_userdef_free(__ops_ss_userdef_t *);
void __ops_ss_reserved_free(__ops_ss_unknown_t *);
void __ops_ss_revocation_free(__ops_ss_revocation_t *);
void __ops_ss_sig_target_free(__ops_ss_sig_target_t *);
void __ops_ss_embedded_sig_free(__ops_ss_embedded_sig_t *);

void __ops_subpacket_free(__ops_subpacket_t *);
void __ops_parser_content_free(__ops_packet_t *);
void __ops_seckey_free(__ops_seckey_t *);
void __ops_pk_sesskey_free(__ops_pk_sesskey_t *);

int __ops_print_packet(const __ops_packet_t *);

#define DYNARRAY(type, arr)	\
	unsigned arr##c; unsigned arr##vsize; type *arr##s

#define EXPAND_ARRAY(str, arr) do {					\
	if (str->arr##c == str->arr##vsize) {				\
		str->arr##vsize = (str->arr##vsize * 2) + 10; 		\
		str->arr##s = realloc(str->arr##s,			\
			str->arr##vsize * sizeof(*str->arr##s));	\
	}								\
} while(/*CONSTCOND*/0)

/** __ops_keydata_key_t
 */
typedef union {
	__ops_pubkey_t pubkey;
	__ops_seckey_t seckey;
} __ops_keydata_key_t;


/** sigpacket_t */
typedef struct {
	__ops_userid_t		*userid;
	__ops_subpacket_t	*packet;
} sigpacket_t;

/* XXX: gonna have to expand this to hold onto subkeys, too... */
/** \struct __ops_keydata
 * \todo expand to hold onto subkeys
 */
struct __ops_key_t {
	DYNARRAY(__ops_userid_t, uid);
	DYNARRAY(__ops_subpacket_t, packet);
	DYNARRAY(sigpacket_t, sig);
	unsigned char		key_id[OPS_KEY_ID_SIZE];
	__ops_fingerprint_t	fingerprint;
	__ops_content_tag_t	type;
	__ops_keydata_key_t	key;
};

#define MDC_PKT_TAG	0xd3

#endif /* PACKET_H_ */
