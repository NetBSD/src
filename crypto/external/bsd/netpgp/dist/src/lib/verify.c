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

#include "config.h"

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#if defined(__NetBSD__)
__COPYRIGHT("@(#) Copyright (c) 2009 The NetBSD Foundation, Inc. All rights reserved.");
__RCSID("$NetBSD: verify.c,v 1.1 2009/06/08 06:09:53 agc Exp $");
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include <sys/mman.h>

#ifdef HAVE_OPENSSL_AES_H
#include <openssl/aes.h>
#endif

#ifdef HAVE_OPENSSL_CAST_H
#include <openssl/cast.h>
#endif

#ifdef HAVE_OPENSSL_DES_H
#include <openssl/des.h>
#endif

#ifdef HAVE_OPENSSL_DSA_H
#include <openssl/dsa.h>
#endif

#ifdef HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif

#ifdef HAVE_OPENSSL_IDEA_H
#include <openssl/idea.h>
#endif

#ifdef HAVE_OPENSSL_MD5_H
#include <openssl/md5.h>
#endif

#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#endif

/* Apple */
#ifdef HAVE_COMMONCRYPTO_COMMONDIGEST_H
#undef MD5_DIGEST_LENGTH
#undef SHA_DIGEST_LENGTH
#define COMMON_DIGEST_FOR_OPENSSL	1
#include <CommonCrypto/CommonDigest.h>
#endif

#ifdef HAVE_OPENSSL_RAND_H
#include <openssl/rand.h>
#endif

#ifdef HAVE_OPENSSL_RSA_H
#include <openssl/rsa.h>
#endif

#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif

#include <errno.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

/* external interface */
#include <netpgp.h>

#ifndef NETPGP_AUTOCONF_VERSION
#define NETPGP_AUTOCONF_VERSION	PACKAGE_VERSION
#endif

#ifndef NETPGP_MAINTAINER
#define NETPGP_MAINTAINER	PACKAGE_BUGREPORT
#endif

/* don't use some digest algorithms */
#undef USE_SHA224
#undef USE_SHA384

/* development versions have .99 suffix */
#define NETPGP_BASE_VERSION	"1.99.9"

#define NETPGP_VERSION_CAT(a, b)	"NetPGP portable " a "/[" b "]"
#define NETPGP_VERSION_STRING \
	NETPGP_VERSION_CAT(NETPGP_BASE_VERSION, NETPGP_AUTOCONF_VERSION)


/* SHA1 Hash Size */
#define OPS_SHA1_HASH_SIZE 	SHA_DIGEST_LENGTH
#define OPS_SHA256_HASH_SIZE	SHA256_DIGEST_LENGTH
#define OPS_CHECKHASH_SIZE	OPS_SHA1_HASH_SIZE

enum {
	MAX_ID_LENGTH		= 128,
	MAX_PASSPHRASE_LENGTH	= 256
};

#define PRItime		"ll"

#ifdef WIN32
#define PRIsize		"I"
#else
#define PRIsize		"z"
#endif

/* if this is defined, we'll use mmap in preference to file ops */
#define USE_MMAP_FOR_FILES      1

/* for silencing unused parameter warnings */
#define __OPS_USED(x)	/*LINTED*/(void)&(x)

#ifndef __UNCONST
#define __UNCONST(a)	((void *)(unsigned long)(const void *)(a))
#endif

/* number of elements in an array */
#define NETPGP_BUFSIZ	8192

#define CALLBACK(cbdata, t, pkt)	do {				\
	(pkt)->tag = (t);						\
	if (__ops_callback(cbdata, pkt) == OPS_RELEASE_MEMORY) {	\
		__ops_pkt_free(pkt);					\
	}								\
} while(/* CONSTCOND */0)

/* this struct describes an area of memory allocated/mmapped */
typedef struct __ops_memory_t {
	uint8_t		*buf;		/* pointer to memory */
	size_t           length;	/* length of memory */
	size_t		 allocated;	/* size, if allocated */
	unsigned	 mmapped;	/* whether buf is mmap-ed */
} __ops_memory_t;

/* Remember to add names to map in errors.c */
typedef enum {
	OPS_E_OK = 0x0000,	/* no error */
	OPS_E_FAIL = 0x0001,	/* general error */
	OPS_E_SYSTEM_ERROR = 0x0002,	/* system error, look at errno for
					 * details */
	OPS_E_UNIMPLEMENTED = 0x0003,	/* feature not yet implemented */

	/* reader errors */
	OPS_E_R = 0x1000,	/* general reader error */
	OPS_E_R_READ_FAILED = OPS_E_R + 1,
	OPS_E_R_EARLY_EOF = OPS_E_R + 2,
	OPS_E_R_BAD_FORMAT = OPS_E_R + 3,	/* For example, malformed
						 * armour */
	OPS_E_R_UNSUPPORTED = OPS_E_R + 4,
	OPS_E_R_UNCONSUMED_DATA = OPS_E_R + 5,

	/* writer errors */
	OPS_E_W = 0x2000,	/* general writer error */
	OPS_E_W_WRITE_FAILED = OPS_E_W + 1,
	OPS_E_W_WRITE_TOO_SHORT = OPS_E_W + 2,

	/* parser errors */
	OPS_E_P = 0x3000,	/* general parser error */
	OPS_E_P_NOT_ENOUGH_DATA = OPS_E_P + 1,
	OPS_E_P_UNKNOWN_TAG = OPS_E_P + 2,
	OPS_E_P_PACKET_CONSUMED = OPS_E_P + 3,
	OPS_E_P_MPI_FORMAT_ERROR = OPS_E_P + 4,
	OPS_E_P_PACKET_NOT_CONSUMED = OPS_E_P + 5,
	OPS_E_P_DECOMPRESSION_ERROR = OPS_E_P + 6,
	OPS_E_P_NO_USERID = OPS_E_P + 7,

	/* creator errors */
	OPS_E_C = 0x4000,	/* general creator error */

	/* validation errors */
	OPS_E_V = 0x5000,	/* general validation error */
	OPS_E_V_BAD_SIGNATURE = OPS_E_V + 1,
	OPS_E_V_NO_SIGNATURE = OPS_E_V + 2,
	OPS_E_V_UNKNOWN_SIGNER = OPS_E_V + 3,
	OPS_E_V_BAD_HASH = OPS_E_V + 4,

	/* Algorithm support errors */
	OPS_E_ALG = 0x6000,	/* general algorithm error */
	OPS_E_ALG_UNSUPPORTED_SYMMETRIC_ALG = OPS_E_ALG + 1,
	OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG = OPS_E_ALG + 2,
	OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG = OPS_E_ALG + 3,
	OPS_E_ALG_UNSUPPORTED_HASH_ALG = OPS_E_ALG + 4,
	OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG = OPS_E_ALG + 5,

	/* Protocol errors */
	OPS_E_PROTO = 0x7000,	/* general protocol error */
	OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT = OPS_E_PROTO + 2,
	OPS_E_PROTO_UNKNOWN_SS = OPS_E_PROTO + 3,
	OPS_E_PROTO_CRITICAL_SS_IGNORED = OPS_E_PROTO + 4,
	OPS_E_PROTO_BAD_PUBLIC_KEY_VRSN = OPS_E_PROTO + 5,
	OPS_E_PROTO_BAD_SIGNATURE_VRSN = OPS_E_PROTO + 6,
	OPS_E_PROTO_BAD_ONE_PASS_SIG_VRSN = OPS_E_PROTO + 7,
	OPS_E_PROTO_BAD_PKSK_VRSN = OPS_E_PROTO + 8,
	OPS_E_PROTO_DECRYPTED_MSG_WRONG_LEN = OPS_E_PROTO + 9,
	OPS_E_PROTO_BAD_SK_CHECKSUM = OPS_E_PROTO + 10
} __ops_errcode_t;

/** one entry in a linked list of errors */
typedef struct __ops_error_t {
	__ops_errcode_t	errcode;
	int		sys_errno; /* if errcode is OPS_E_SYSTEM_ERROR */
	char		*comment;
	const char	*file;
	int		 line;
	struct __ops_error_t	*next;
} __ops_error_t;

#define OPS_SYSTEM_ERROR_1(err,code,sys,fmt,arg)	do {		\
	__ops_push_error(err,OPS_E_SYSTEM_ERROR,errno,__FILE__,__LINE__,sys);\
	__ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg);		\
} while(/*CONSTCOND*/0)

#define OPS_MEMORY_ERROR(err) {						\
	loggit("Memory error");				\
}				/* \todo placeholder for better error
				 * handling */
#define OPS_ERROR(err,code,fmt)	do {					\
	__ops_push_error(err,code,0,__FILE__,__LINE__,fmt);		\
} while(/*CONSTCOND*/0)
#define OPS_ERROR_1(err,code,fmt,arg)	do {				\
	__ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg);		\
} while(/*CONSTCOND*/0)
#define OPS_ERROR_2(err,code,fmt,arg,arg2)	do {			\
	__ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg,arg2);	\
} while(/*CONSTCOND*/0)
#define OPS_ERROR_3(err,code,fmt,arg,arg2,arg3)	do {			\
	__ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg,arg2,arg3);	\
} while(/*CONSTCOND*/0)
#define OPS_ERROR_4(err,code,fmt,arg,arg2,arg3,arg4)	do {		\
	__ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg,arg2,arg3,arg4); \
} while(/*CONSTCOND*/0)

/** General-use structure for variable-length data
 */

typedef struct __ops_data_t {
	size_t           len;
	uint8_t		*contents;
	uint8_t		 mmapped;	/* contents need an munmap(2) */
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
 * See #__ops_ptag_oldlen_t for the meaning of the values.
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
} __ops_ptag_oldlen_t;


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
	OPS_PTAG_CT_LITERAL_DATA = 11,	/* Literal Data Packet */
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
	OPS_PTAG_CT_LITERAL_DATA_HEADER = 0x300,
	OPS_PTAG_CT_LITERAL_DATA_BODY = 0x300 + 1,
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

/** Structure to hold one parse error string. */
typedef struct __ops_parser_error_t {
	const char     *error;	/* error message. */
} __ops_parser_error_t;

/** Structure to hold one error code */
typedef struct __ops_parser_errcode_t {
	__ops_errcode_t   errcode;
} __ops_parser_errcode_t;

/** Structure to hold one packet tag.
 * \see RFC4880 4.2
 */
typedef struct __ops_ptag_t {
	unsigned        new_format;	/* Whether this packet tag is new
					 * (1) or old format (0) */
	unsigned        type;	/* content_tag value - See
					 * #__ops_content_tag_t for meanings */
	__ops_ptag_oldlen_t lentype;	/* Length type (#__ops_ptag_oldlen_t)
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

/** Structure to hold one DSA public key parameters.
 *
 * \see RFC4880 5.5.2
 */
typedef struct __ops_dsa_pubkey_t {
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
typedef struct __ops_rsa_pubkey_t {
	BIGNUM         *n;	/* RSA public modulus n */
	BIGNUM         *e;	/* RSA public encryption exponent e */
} __ops_rsa_pubkey_t;

/** Structure to hold an ElGamal public key parameters.
 *
 * \see RFC4880 5.5.2
 */
typedef struct __ops_elgamal_pubkey_t {
	BIGNUM         *p;	/* ElGamal prime p */
	BIGNUM         *g;	/* ElGamal group generator g */
	BIGNUM         *y;	/* ElGamal public key value y (= g^x mod p
				 * with x being the secret) */
} __ops_elgamal_pubkey_t;

/** Union to hold public key parameters of any algorithm */
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
typedef struct __ops_pubkey_t {
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
typedef struct __ops_rsa_seckey_t {
	BIGNUM         *d;
	BIGNUM         *p;
	BIGNUM         *q;
	BIGNUM         *u;
} __ops_rsa_seckey_t;

/** __ops_dsa_seckey_t */
typedef struct __ops_dsa_seckey_t {
	BIGNUM         *x;
} __ops_dsa_seckey_t;

/** __ops_seckey_union_t */
typedef union __ops_seckey_union_t {
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
#ifdef USE_SHA384
	OPS_HASH_SHA384 = 9,	/* SHA384 */
#endif
	OPS_HASH_SHA512 = 10,	/* SHA512 */
#ifdef USE_SHA224
	OPS_HASH_SHA224 = 11	/* SHA224 */
#endif
} __ops_hash_alg_t;


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
typedef struct __ops_seckey_t {
	__ops_pubkey_t			pubkey;
	__ops_s2k_usage_t		s2k_usage;
	__ops_s2k_specifier_t		s2k_specifier;
	__ops_symm_alg_t		alg;
	__ops_hash_alg_t		hash_alg;
	uint8_t				salt[OPS_SALT_SIZE];
	unsigned			octetc;
	uint8_t				iv[OPS_MAX_BLOCK_SIZE];
	__ops_seckey_union_t		key;
	unsigned			checksum;
	uint8_t			       *checkhash;
} __ops_seckey_t;

/** Structure to hold one trust packet's data */

typedef struct __ops_trust_t {
	__ops_data_t      data;	/* Trust Packet */
} __ops_trust_t;

/** Structure to hold one user id */
typedef struct __ops_userid_t {
	uint8_t  *userid;/* User ID - UTF-8 string */
} __ops_userid_t;

/** Structure to hold one user attribute */
typedef struct __ops_userattr_t {
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
} __ops_signtype_t;

/** Struct to hold parameters of an RSA signature */
typedef struct __ops_rsa_sig_t {
	BIGNUM         *sig;	/* the signature value (m^d % n) */
} __ops_rsa_sig_t;

/** Struct to hold parameters of a DSA signature */
typedef struct __ops_dsa_sig_t {
	BIGNUM         *r;	/* DSA value r */
	BIGNUM         *s;	/* DSA value s */
} __ops_dsa_sig_t;

/** __ops_elgamal_signature_t */
typedef struct __ops_elgamal_sig_t {
	BIGNUM         *r;
	BIGNUM         *s;
} __ops_elgamal_sig_t;

/** Struct to hold data for a private/experimental signature */
typedef struct __ops_unknown_sig_t {
	__ops_data_t      data;
} __ops_unknown_sig_t;

/** Union to hold signature parameters of any algorithm */
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
typedef struct __ops_signinfo_t {
	__ops_version_t		version;/* signature version number */
	__ops_signtype_t	type;	/* signature type value */
	time_t			birthtime; /* creation time of the signature */
	uint8_t			signer[OPS_KEY_ID_SIZE];
					/* Eight-octet key ID of signer */
	__ops_pubkey_alg_t	key_alg; /* public key algorithm number */
	__ops_hash_alg_t	hash_alg; /* hashing algorithm number */
	__ops_sig_union_t	sig;	/* signature parameters */
	size_t			v4_hashlen;
	uint8_t			*v4_hashed;
	unsigned		 gotbirthtime:1;
	unsigned		 gotsigner:1;
} __ops_signinfo_t;

#define OPS_MIN_HASH_SIZE	16

/** __ops_hash_t */
typedef struct __ops_hash_t {
	__ops_hash_alg_t	 alg;		/* algorithm */
	size_t			 size;		/* size */
	const char		*name;		/* what it's known as */
	void	(*init)(struct __ops_hash_t *);	/* initialisation func */
	void	(*add)(struct __ops_hash_t *, const uint8_t *, unsigned);
		/* add text func */
	unsigned (*finish)(struct __ops_hash_t *, uint8_t *);
		/* finalise func */
	void		 	*data;		/* blob for data */
} __ops_hash_t;

/** Struct used when parsing a signature */
typedef struct __ops_sig_t {
	__ops_signinfo_t info;	/* The signature information */
	/* The following fields are only used while parsing the signature */
	uint8_t   hash2[2];	/* high 2 bytes of hashed value -
					 * for quick test */
	size_t          v4_hashstart;	/* only valid if accumulate
						 * is set */
	__ops_hash_t     *hash;	/* if set, the hash filled in for the data
				 * so far */
} __ops_sig_t;

/** The raw bytes of a signature subpacket */
typedef struct __ops_ss_raw_t {
	__ops_content_tag_t	 tag;
	size_t          	 length;
	uint8_t			*raw;
} __ops_ss_raw_t;

/** Signature Subpacket : Trust Level */
typedef struct __ops_ss_trust_t {
	uint8_t		level;	/* Trust Level */
	uint8_t		amount;	/* Amount */
} __ops_ss_trust_t;

/** Signature Subpacket : Revocable */
typedef struct __ops_ss_revocable_t {
	unsigned	revocable;
} __ops_ss_revocable_t;

/** Signature Subpacket : Time */
typedef struct __ops_ss_time_t {
	time_t		time;
} __ops_ss_time_t;

/** Signature Subpacket : Key ID */
typedef struct __ops_ss_key_id_t {
	uint8_t		keyid[OPS_KEY_ID_SIZE];
} __ops_ss_key_id_t;

/** Signature Subpacket : Notation Data */
typedef struct __ops_ss_notation_t {
	__ops_data_t		flags;
	__ops_data_t		name;
	__ops_data_t		value;
} __ops_ss_notation_t;

/** Signature Subpacket : User Defined */
typedef struct __ops_ss_userdef_t {
	__ops_data_t		data;
} __ops_ss_userdef_t;

/** Signature Subpacket : Unknown */
typedef struct __ops_ss_unknown_t {
	__ops_data_t		data;
} __ops_ss_unknown_t;

/** Signature Subpacket : Preferred Symmetric Key Algorithm */
typedef struct __ops_ss_skapref_t {
	__ops_data_t		data;
	/*
	 * Note that value 0 may represent the plaintext algorithm so we
	 * cannot expect data->contents to be a null-terminated list
	 */
} __ops_ss_skapref_t;

/** Signature Subpacket : Preferrred Hash Algorithm */
typedef struct __ops_ss_hashpref_t {
	__ops_data_t      data;
} __ops_ss_hashpref_t;

/** Signature Subpacket : Preferred Compression */
typedef struct __ops_ss_zpref_t {
	__ops_data_t      data;
} __ops_ss_zpref_t;

/** Signature Subpacket : Key Flags */
typedef struct __ops_ss_key_flags_t {
	__ops_data_t      data;
} __ops_ss_key_flags_t;

/** Signature Subpacket : Key Server Preferences */
typedef struct __ops_ss_key_server_prefs_t {
	__ops_data_t      data;
} __ops_ss_key_server_prefs_t;

/** Signature Subpacket : Features */
typedef struct __ops_ss_features_t {
	__ops_data_t      data;
} __ops_ss_features_t;

/** Signature Subpacket : Signature Target */
typedef struct __ops_ss_sig_target_t {
	__ops_pubkey_alg_t	pka_alg;
	__ops_hash_alg_t	hash_alg;
	__ops_data_t		hash;
} __ops_ss_sig_target_t;

/** Signature Subpacket : Embedded Signature */
typedef struct __ops_ss_embedded_sig_t {
	__ops_data_t      sig;
} __ops_ss_embedded_sig_t;

/** __ops_subpacket_t */
typedef struct __ops_subpacket_t {
	size_t           length;
	uint8_t		*raw;
} __ops_subpacket_t;

/** Types of Compression */
typedef enum {
	OPS_C_NONE = 0,
	OPS_C_ZIP = 1,
	OPS_C_ZLIB = 2,
	OPS_C_BZIP2 = 3
} __ops_ztype_t;

/*
 * unlike most structures, this will feed its data as a stream to the
 * application instead of directly including it
 */
/** __ops_compressed_t */
typedef struct __ops_compressed_t {
	__ops_ztype_t type;
} __ops_compressed_t;

/** __ops_one_pass_sig_t */
typedef struct __ops_one_pass_sig_t {
	uint8_t			version;
	__ops_signtype_t	sig_type;
	__ops_hash_alg_t	hash_alg;
	__ops_pubkey_alg_t	key_alg;
	uint8_t			keyid[OPS_KEY_ID_SIZE];
	unsigned		nested;
} __ops_one_pass_sig_t;

/** Signature Subpacket : Primary User ID */
typedef struct __ops_ss_primary_userid_t {
	unsigned   primary_userid;
} __ops_ss_primary_userid_t;

/** Signature Subpacket : Regexp */
typedef struct __ops_ss_regexp_t {
	char           *regexp;
} __ops_ss_regexp_t;

/** Signature Subpacket : Policy URL */
typedef struct __ops_ss_policy_t {
	char           *url;
} __ops_ss_policy_t;

/** Signature Subpacket : Preferred Key Server */
typedef struct __ops_ss_keyserv_t {
	char           *name;
} __ops_ss_keyserv_t;

/** Signature Subpacket : Revocation Key */
typedef struct __ops_ss_revocation_key_t {
	uint8_t   class;
	uint8_t   algid;
	uint8_t   fingerprint[OPS_FINGERPRINT_SIZE];
} __ops_ss_revocation_key_t;

/** Signature Subpacket : Revocation Reason */
typedef struct __ops_ss_revocation_t {
	uint8_t   code;
	char     *reason;
} __ops_ss_revocation_t;

/** litdata_type_t */
typedef enum {
	OPS_LITDATA_BINARY = 'b',
	OPS_LITDATA_TEXT = 't',
	OPS_LITDATA_UTF8 = 'u',
	OPS_LITDATA_LOCAL = 'l',
	OPS_LITDATA_LOCAL2 = '1'
} __ops_litdata_type_t;

/** __ops_litdata_header_t */
typedef struct __ops_litdata_header_t {
	__ops_litdata_type_t	format;
	char			filename[256];
	time_t			mtime;
} __ops_litdata_header_t;

/** __ops_litdata_body_t */
typedef struct __ops_litdata_body_t {
	unsigned         length;
	uint8_t		*data;
	__ops_memory_t	*mem;
} __ops_litdata_body_t;

/** __ops_mdc_t */
typedef struct __ops_mdc_t {
	unsigned	 length;
	uint8_t		*data;
} __ops_mdc_t;

/** __ops_header_var_t */
typedef struct __ops_header_var_t {
	char           *key;
	char           *value;
} __ops_header_var_t;

/** __ops_headers_t */
typedef struct __ops_headers_t {
	__ops_header_var_t	*headers;
	unsigned	         headerc;
} __ops_headers_t;

/** __ops_armour_header_t */
typedef struct __ops_armour_header_t {
	const char	*type;
	__ops_headers_t	 headers;
} __ops_armour_header_t;

/** __ops_armour_trailer_t */
typedef struct __ops_armour_trailer_t {
	const char     *type;
} __ops_armour_trailer_t;

/** __ops_cleartext_head_t */
typedef struct __ops_cleartext_head_t {
	__ops_headers_t   headers;
} __ops_cleartext_head_t;

/** __ops_cleartext_body_t */
typedef struct __ops_cleartext_body_t {
	unsigned        length;
	uint8_t   data[8192];	/* \todo fix hard-coded value? */
} __ops_cleartext_body_t;

/** __ops_cleartext_trailer_t */
typedef struct __ops_cleartext_trailer_t {
	struct __ops_hash_t *hash;	/* This will not have been
					 * finalised, but will have seen all
					 * the cleartext data in canonical
					 * form */
} __ops_cleartext_trailer_t;

/** __ops_unarmoured_text_t */
typedef struct __ops_unarmoured_text_t {
	unsigned        length;
	uint8_t		*data;
} __ops_unarmoured_text_t;

typedef enum {
	SE_IP_DATA_VERSION = 1
} __ops_se_ip_data_version_t;

typedef enum {
	OPS_PKSK_V3 = 3
} __ops_pk_sesskey_version_t;

/** __ops_pk_sesskey_parameters_rsa_t */
typedef struct __ops_pk_sesskey_parameters_rsa_t {
	BIGNUM         *encrypted_m;
	BIGNUM         *m;
} __ops_pk_sesskey_parameters_rsa_t;

/** __ops_pk_sesskey_parameters_elgamal_t */
typedef struct __ops_pk_sesskey_parameters_elgamal_t {
	BIGNUM         *g_to_k;
	BIGNUM         *encrypted_m;
} __ops_pk_sesskey_parameters_elgamal_t;

/** __ops_pk_sesskey_parameters_t */
typedef union {
	__ops_pk_sesskey_parameters_rsa_t rsa;
	__ops_pk_sesskey_parameters_elgamal_t elgamal;
} __ops_pk_sesskey_parameters_t;

/** __ops_pk_sesskey_t */
typedef struct __ops_pk_sesskey_t {
	__ops_pk_sesskey_version_t	version;
	uint8_t				keyid[OPS_KEY_ID_SIZE];
	__ops_pubkey_alg_t		alg;
	__ops_pk_sesskey_parameters_t	parameters;
	__ops_symm_alg_t		symm_alg;
	uint8_t				key[OPS_MAX_KEY_SIZE];
	uint16_t			checksum;
} __ops_pk_sesskey_t;

/** __ops_seckey_passphrase_t */
typedef struct __ops_seckey_passphrase_t {
	const __ops_seckey_t	 *seckey;
	char			**passphrase;
		/* point somewhere that gets filled in to work around
		 * constness of content */
} __ops_seckey_passphrase_t;

typedef enum {
	OPS_SE_IP_V1 = 1
} __ops_se_ip_version_t;

/** __ops_se_ip_data_header_t */
typedef struct __ops_se_ip_data_header_t {
	__ops_se_ip_version_t version;
} __ops_se_ip_data_header_t;

/** __ops_se_ip_data_body_t */
typedef struct __ops_se_ip_data_body_t {
	unsigned        length;
	uint8_t  *data;	/* \todo remember to free this */
} __ops_se_ip_data_body_t;

/** __ops_se_data_body_t */
typedef struct __ops_se_data_body_t {
	unsigned        length;
	uint8_t   data[8192];	/* \todo parameterise this! */
} __ops_se_data_body_t;

/** __ops_get_seckey_t */
typedef struct __ops_get_seckey_t {
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
typedef struct __ops_packet_t {
	__ops_content_tag_t	tag;		/* type of contents */
	uint8_t		critical;	/* for sig subpackets */
	__ops_contents_t	u;		/* union for contents */
} __ops_packet_t;

/** __ops_fingerprint_t */
typedef struct __ops_fingerprint_t {
	uint8_t   fingerprint[OPS_FINGERPRINT_SIZE];
	unsigned        length;
} __ops_fingerprint_t;

#define DYNARRAY(type, arr)	\
	unsigned arr##c; unsigned arr##size; type *arr##s

#define EXPAND(str, arr) do {						\
	if (str->arr##c == str->arr##size) {				\
		str->arr##size = (str->arr##size * 2) + 10;		\
		str->arr##s = realloc(str->arr##s,			\
			str->arr##size * sizeof(*str->arr##s));		\
	}								\
} while(/*CONSTCOND*/0)

/** __ops_keypair_union_t
 */
typedef union {
	__ops_pubkey_t pubkey;
	__ops_seckey_t seckey;
} __ops_keypair_union_t;

/** sigpacket_t */
typedef struct sigpacket_t {
	__ops_userid_t		*userid;
	__ops_subpacket_t	*packet;
} sigpacket_t;

/** 
 * \todo expand to hold onto subkeys
 */
typedef struct __ops_key_t {
	DYNARRAY(__ops_userid_t,	uid);
	DYNARRAY(__ops_subpacket_t, 	packet);
	DYNARRAY(sigpacket_t, 		sig);
	uint8_t			keyid[OPS_KEY_ID_SIZE];
	__ops_fingerprint_t		fingerprint;
	__ops_content_tag_t		type;
	__ops_keypair_union_t		key;
} __ops_key_t;

/* this struct defines a number of keys (a keyring) */
typedef struct __ops_keyring_t {
	DYNARRAY(__ops_key_t,	key);
} __ops_keyring_t;

#define MDC_PKT_TAG	0xd3

typedef struct __ops_io_t {
	void	*outs;	/* output file stream */
	void	*errs;	/* file stream to put error messages */
} __ops_io_t;

/** Writer settings */
typedef struct __ops_writer_t {
	unsigned		(*writer)(struct __ops_writer_t *,
					const uint8_t *,
					unsigned,
					__ops_error_t **);
					/* the writer itself */
	unsigned		(*finaliser)(__ops_error_t **,
					struct __ops_writer_t *);
					/* the writer's finaliser */
	void			(*destroyer)(struct __ops_writer_t *);
					/* the writer's destroyer */
	void			 *arg;	/* writer-specific argument */
	struct __ops_writer_t 	 *next;	/* next writer in the stack */
	__ops_io_t		 *io;	/* IO for errors and output */
} __ops_writer_t;

typedef struct __ops_output_t {
	__ops_writer_t	 writer;
	__ops_error_t	*errors;	/* error stack */
} __ops_output_t;

/** _ops_crypt_t */
typedef struct __ops_crypt_t {
	__ops_symm_alg_t	alg;
	size_t			blocksize;
	size_t			keysize;
	void	(*set_iv)(struct __ops_crypt_t *, const uint8_t *);
	void	(*set_key)(struct __ops_crypt_t *, const uint8_t *);
	void	(*base_init)(struct __ops_crypt_t *);
	void	(*decrypt_resync)(struct __ops_crypt_t *);
	/* encrypt/decrypt one block  */
	void	(*block_encrypt)(struct __ops_crypt_t *, void *, const void *);
	void	(*block_decrypt)(struct __ops_crypt_t *, void *, const void *);
	/* Standard CFB encrypt/decrypt (as used by Sym Enc Int Prot packets) */
	void	(*cfb_encrypt)(struct __ops_crypt_t *, void *, const void *,
					size_t);
	void	(*cfb_decrypt)(struct __ops_crypt_t *, void *, const void *,
					size_t);
	void	(*decrypt_finish)(struct __ops_crypt_t *);
	uint8_t		iv[OPS_MAX_BLOCK_SIZE];
	uint8_t		civ[OPS_MAX_BLOCK_SIZE];
	uint8_t		siv[OPS_MAX_BLOCK_SIZE];
		/* siv is needed for weird v3 resync */
	uint8_t		key[OPS_MAX_KEY_SIZE];
	int			num;
		/* num is offset - see openssl _encrypt doco */
	void			*encrypt_key;
	void			*decrypt_key;
} __ops_crypt_t;

/** __ops_region_t */
typedef struct __ops_region_t {
	struct __ops_region_t	*parent;
	unsigned		 length;
	unsigned		 readc;		/* length read */
	unsigned		 lastread;
		/* length of last read, only valid in deepest child */
	unsigned		 indeterm:1;
} __ops_region_t;

/** __ops_cb_ret_t */
typedef enum {
	OPS_RELEASE_MEMORY,
	OPS_KEEP_MEMORY,
	OPS_FINISHED
} __ops_cb_ret_t;

/*
   A reader MUST read at least one byte if it can, and should read up
   to the number asked for. Whether it reads more for efficiency is
   its own decision, but if it is a stacked reader it should never
   read more than the length of the region it operates in (which it
   would have to be given when it is stacked).

   If a read is short because of EOF, then it should return the short
   read (obviously this will be zero on the second attempt, if not the
   first). Because a reader is not obliged to do a full read, only a
   zero return can be taken as an indication of EOF.

   If there is an error, then the callback should be notified, the
   error stacked, and -1 should be returned.

   Note that although length is a size_t, a reader will never be asked
   to read more than INT_MAX in one go.

 */
/** Used to specify whether subpackets should be returned raw, parsed
* or ignored.  */
typedef enum {
	OPS_PARSE_RAW,		/* Callback Raw */
	OPS_PARSE_PARSED,	/* Callback Parsed */
	OPS_PARSE_IGNORE	/* Don't callback */
} __ops_parse_type_t;

/** __ops_cbdata_t */
typedef struct __ops_cbdata_t {
	__ops_cb_ret_t		(*cbfunc)(struct __ops_cbdata_t *,
						const __ops_packet_t *);
						/* callback function */
	void			*arg;	/* args to pass to callback func */
	__ops_error_t		**errors; /* address of error stack */
	struct __ops_cbdata_t	*next;
	__ops_output_t		*output;/* used if writing out parsed info */
	__ops_io_t		*io;		/* error/output messages */
} __ops_cbdata_t;

/** __ops_reader_t */
typedef struct __ops_reader_t {
	int		(*reader)(void *, size_t, __ops_error_t **,
				struct __ops_reader_t *,
				__ops_cbdata_t *);
	void		(*destroyer)(struct __ops_reader_t *);
	void		*arg;	/* args to pass to reader function */
	unsigned	 accumulate:1;	/* set to gather packet data */
	uint8_t	*accumulated;	/* the accumulated data */
	unsigned	 asize;	/* size of the buffer */
	unsigned	 alength;/* used buffer */
	unsigned	 position;	/* reader-specific offset */
	struct __ops_reader_t	*next;
	struct __ops_stream_t *parent;/* parent parse_info structure */
} __ops_reader_t;

/** __ops_hashtype_t */
typedef struct __ops_hashtype_t {
	__ops_hash_t	hash;
	uint8_t	keyid[OPS_KEY_ID_SIZE];
} __ops_hashtype_t;

#define NTAGS	0x100	/* == 256 */

/** \brief Structure to hold information about a packet parse.
 *
 *  This information includes options about the parse:
 *  - whether the packet contents should be accumulated or not
 *  - whether signature subpackets should be parsed or left raw
 *
 *  It contains options specific to the parsing of armoured data:
 *  - whether headers are allowed in armoured data without a gap
 *  - whether a blank line is allowed at the start of the armoured data
 *
 *  It also specifies :
 *  - the callback function to use and its arguments
 *  - the reader function to use and its arguments
 *
 *  It also contains information about the current state of the parse:
 *  - offset from the beginning
 *  - the accumulated data, if any
 *  - the size of the buffer, and how much has been used
 *
 *  It has a linked list of errors.
 */
typedef struct __ops_stream_t {
	uint8_t		 ss_raw[NTAGS / 8];
		/* 1 bit / sig-subpkt type; set to get raw data */
	uint8_t		 ss_parsed[NTAGS / 8];
		/* 1 bit / sig-subpkt type; set to get parsed data */
	__ops_reader_t	 	 readinfo;
	__ops_cbdata_t		 cbdata;
	__ops_error_t		*errors;
	void			*io;		/* io streams */
	__ops_crypt_t		 decrypt;
	size_t			 hashc;
	__ops_hashtype_t        *hashes;
	unsigned		 reading_v3_secret:1;
	unsigned		 reading_mpi_len:1;
	unsigned		 exact_read:1;
} __ops_stream_t;

typedef struct __ops_validation_t {
	unsigned int		 validc;
	__ops_signinfo_t	*valids;
	unsigned int		 invalidc;
	__ops_signinfo_t	*invalids;
	unsigned int		 unknownc;
	__ops_signinfo_t	*unknowns;
} __ops_validation_t;

typedef struct validate_reader_t {
	const __ops_key_t	*key;
	unsigned	         packet;
	unsigned	         offset;
} validate_reader_t;

/** Struct use with the validate_data_cb callback */
typedef struct {
	enum {
		LITERAL_DATA,
		SIGNED_CLEARTEXT
	} type;
	union {
		__ops_litdata_body_t	 litdata_body;
		__ops_cleartext_body_t	 cleartext_body;
	} data;
	uint8_t			 hash[OPS_MAX_HASH_SIZE];
	__ops_memory_t			*mem;
	const __ops_keyring_t		*keyring;
	validate_reader_t		*reader;/* reader-specific arg */
	__ops_validation_t		*result;
	char				*detachname;
} validate_data_cb_t;

typedef enum {
	OPS_PGP_MESSAGE = 1,
	OPS_PGP_PUBLIC_KEY_BLOCK,
	OPS_PGP_PRIVATE_KEY_BLOCK,
	OPS_PGP_MULTIPART_MESSAGE_PART_X_OF_Y,
	OPS_PGP_MULTIPART_MESSAGE_PART_X,
	OPS_PGP_SIGNATURE
} __ops_armor_type_t;

#define CRC24_INIT 0xb704ceL

/*************************************************************************/
/* code starts here */
/*************************************************************************/

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

static void
loggit(const char *fmt, ...)
{
	va_list	args;
	char	buf[1024];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	printf("netpgpverify: %s\n", buf);
}

/* generic grab new storage function */
static void *
__ops_new(size_t size)
{
	void	*vp;

	if ((vp = calloc(1, size)) == NULL) {
		loggit("allocation failure for %" PRIsize "u bytes", size);
	}
	return vp;
}

/* \todo check where userid pointers are copied */
/**
\ingroup Core_Keys
\brief Copy user id, including contents
\param dst Destination User ID
\param src Source User ID
\note If dst already has a userid, it will be freed.
*/
static __ops_userid_t * 
__ops_copy_userid(__ops_userid_t *dst, const __ops_userid_t *src)
{
	size_t          len;

	len = strlen((char *) src->userid);
	if (dst->userid) {
		(void) free(dst->userid);
	}
	dst->userid = __ops_new(len + 1);
	(void) memcpy(dst->userid, src->userid, len);
	return dst;
}

/**
\ingroup Core_Keys
\brief Add User ID to key
\param key Key to which to add User ID
\param userid User ID to add
\return Pointer to new User ID
*/
static __ops_userid_t  *
__ops_add_userid(__ops_key_t *key, const __ops_userid_t *userid)
{
	__ops_userid_t  *uidp = NULL;

	EXPAND(key, uid);
	uidp = &key->uids[key->uidc++];
	uidp->userid = NULL;
	return __ops_copy_userid(uidp, userid);
}

/**
\ingroup Core_Keys
\brief Copy packet, including contents
\param dst Destination packet
\param src Source packet
\note If dst already has a packet, it will be freed.
*/
static __ops_subpacket_t *
__ops_copy_packet(__ops_subpacket_t *dst, const __ops_subpacket_t *src)
{
	if (dst->raw) {
		(void) free(dst->raw);
	}
	dst->raw = __ops_new(src->length);
	dst->length = src->length;
	(void) memcpy(dst->raw, src->raw, src->length);
	return dst;
}

/**
\ingroup Core_Keys
\brief Add packet to key
\param key Key to which to add packet
\param packet Packet to add
\return Pointer to new packet
*/
static __ops_subpacket_t   *
__ops_add_subpacket(__ops_key_t *key, const __ops_subpacket_t *packet)
{
	__ops_subpacket_t   *pktp = NULL;

	EXPAND(key, packet);
	pktp = &key->packets[key->packetc++];
	pktp->length = 0;
	pktp->raw = NULL;
	return __ops_copy_packet(pktp, packet);
}

static __ops_cb_ret_t __ops_callback(__ops_cbdata_t *, const __ops_packet_t *);

static void __ops_keyid(uint8_t *,
			const size_t,
			const __ops_pubkey_t *);
static void __ops_fingerprint(__ops_fingerprint_t *, const __ops_pubkey_t *);


typedef struct {
	__ops_keyring_t  *keyring;
} accumulate_t;

static const char     *__ops_errcode(const __ops_errcode_t);

/**
 * \ingroup Core_Errors
 * \brief Pushes the given error on the given errorstack
 * \param errstack Error stack to use
 * \param errcode Code of error to push
 * \param sys_errno System errno (used if errcode=OPS_E_SYSTEM_ERROR)
 * \param file Source filename where error occurred
 * \param line Line in source file where error occurred
 * \param fmt Comment
 *
 */

static void 
__ops_push_error(__ops_error_t **errstack, __ops_errcode_t errcode,
		int sys_errno, const char *file, int line, const char *fmt,...)
{
	/* first get the varargs and generate the comment */
	__ops_error_t  *err;
	unsigned	maxbuf = 128;
	va_list		args;
	char           *comment;

	if ((comment = __ops_new(maxbuf + 1)) == NULL) {
		loggit("alloc comment failure");
		return;
	}
	va_start(args, fmt);
	vsnprintf(comment, maxbuf + 1, fmt, args);
	va_end(args);
	/* alloc a new error and add it to the top of the stack */
	if ((err = __ops_new(sizeof(__ops_error_t))) == NULL) {
		loggit("alloc err failure");
		return;
	}
	err->next = *errstack;
	*errstack = err;

	/* fill in the details */
	err->errcode = errcode;
	err->sys_errno = sys_errno;
	err->file = file;
	err->line = line;

	err->comment = comment;
}

/**
 * \ingroup Core_Callbacks
 */
static __ops_cb_ret_t
accumulate_cb(__ops_cbdata_t *cbdata, const __ops_packet_t *pkt)
{
	const __ops_contents_t	*content;
	const __ops_pubkey_t	*pubkey;
	__ops_keyring_t		*keyring;
	__ops_key_t		*key;
	accumulate_t		*accumulate;

	content = &pkt->u;
	accumulate = cbdata->arg;
	keyring = accumulate->keyring;
	key = (keyring->keyc > 0) ? &keyring->keys[keyring->keyc] : NULL;
	switch (pkt->tag) {
	case OPS_PTAG_CT_PUBLIC_KEY:
	case OPS_PTAG_CT_SECRET_KEY:
	case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY:
		EXPAND(keyring, key);
		pubkey = (pkt->tag == OPS_PTAG_CT_PUBLIC_KEY) ?
					&content->pubkey :
					&content->seckey.pubkey;
		key = &keyring->keys[++keyring->keyc];
		(void) memset(key, 0x0, sizeof(*key));
		__ops_keyid(key->keyid, OPS_KEY_ID_SIZE, pubkey);
		__ops_fingerprint(&key->fingerprint, pubkey);
		key->type = pkt->tag;
		if (pkt->tag == OPS_PTAG_CT_PUBLIC_KEY) {
			key->key.pubkey = *pubkey;
		} else {
			key->key.seckey = content->seckey;
		}
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_USER_ID:
		if (key) {
			__ops_add_userid(key, &content->userid);
			return OPS_KEEP_MEMORY;
		}
		OPS_ERROR(cbdata->errors, OPS_E_P_NO_USERID, "No userid found");
		return OPS_KEEP_MEMORY;

	case OPS_PARSER_PACKET_END:
		if (key) {
			__ops_add_subpacket(key, &content->packet);
			return OPS_KEEP_MEMORY;
		}
		return OPS_RELEASE_MEMORY;

	case OPS_PARSER_ERROR:
		loggit("Error: %s", content->error.error);
		return OPS_FINISHED;

	case OPS_PARSER_ERRCODE:
		loggit("parse error: %s",
				__ops_errcode(content->errcode.errcode));
		break;

	default:
		break;
	}

	/* XXX: we now exclude so many things, we should either drop this or */
	/* do something to pass on copies of the stuff we keep */
	return __ops_callback(cbdata->next, pkt);
}

/**
\ingroup Core_ReadPackets
\brief Sets the parse_info's callback
This is used when adding the first callback in a stack of callbacks.
\sa __ops_callback_push()
*/
static void 
__ops_set_callback(__ops_stream_t *stream,
		__ops_cb_ret_t (*cb)(__ops_cbdata_t *, const __ops_packet_t *),
		void *arg)
{
	stream->cbdata.cbfunc = cb;
	stream->cbdata.arg = arg;
	stream->cbdata.errors = &stream->errors;
}

/**
\ingroup Core_ReadPackets
\brief Adds a further callback to a stack of callbacks
\sa __ops_set_callback()
*/
static void 
__ops_callback_push(__ops_stream_t *stream,
		__ops_cb_ret_t (*cb)(__ops_cbdata_t *, const __ops_packet_t *),
		void *arg)
{
	__ops_cbdata_t *cbdata;

	cbdata = __ops_new(sizeof(*cbdata));
	(void) memcpy(cbdata, &stream->cbdata, sizeof(*cbdata));
	cbdata->io = stream->io;
	stream->cbdata.next = cbdata;
	__ops_set_callback(stream, cb, arg);
}

static int __ops_parse(__ops_stream_t *, int);

/**
 * \ingroup Core_Parse
 *
 * Parse packets from an input stream until EOF or error.
 *
 * Key data found in the parsed data is added to #keyring.
 *
 * \param keyring Pointer to an existing keyring
 * \param parse Options to use when parsing
*/

static int 
__ops_parse_and_accumulate(__ops_keyring_t *keyring, __ops_stream_t *parse)
{
	accumulate_t	accumulate;
	const int	printerrors = 1;
	int             ret;

	if (parse->readinfo.accumulate) {
		loggit("__ops_parse_and_accumulate: already init");
		return 0;
	}
	(void) memset(&accumulate, 0x0, sizeof(accumulate));
	accumulate.keyring = keyring;
	__ops_callback_push(parse, accumulate_cb, &accumulate);
	parse->readinfo.accumulate = 1;
	ret = __ops_parse(parse, !printerrors);
	return ret;
}


/** \file
 * \brief Error Handling
 */
#define ERRNAME(code)	{ code, #code }

typedef struct {
	int             type;
	const char     *string;
} __ops_map_t;

static __ops_map_t	 errcode_name_map[] = {
	ERRNAME(OPS_E_OK),
	ERRNAME(OPS_E_FAIL),
	ERRNAME(OPS_E_SYSTEM_ERROR),
	ERRNAME(OPS_E_UNIMPLEMENTED),

	ERRNAME(OPS_E_R),
	ERRNAME(OPS_E_R_READ_FAILED),
	ERRNAME(OPS_E_R_EARLY_EOF),
	ERRNAME(OPS_E_R_BAD_FORMAT),
	ERRNAME(OPS_E_R_UNCONSUMED_DATA),

	ERRNAME(OPS_E_W),
	ERRNAME(OPS_E_W_WRITE_FAILED),
	ERRNAME(OPS_E_W_WRITE_TOO_SHORT),

	ERRNAME(OPS_E_P),
	ERRNAME(OPS_E_P_NOT_ENOUGH_DATA),
	ERRNAME(OPS_E_P_UNKNOWN_TAG),
	ERRNAME(OPS_E_P_PACKET_CONSUMED),
	ERRNAME(OPS_E_P_MPI_FORMAT_ERROR),

	ERRNAME(OPS_E_C),

	ERRNAME(OPS_E_V),
	ERRNAME(OPS_E_V_BAD_SIGNATURE),
	ERRNAME(OPS_E_V_NO_SIGNATURE),
	ERRNAME(OPS_E_V_UNKNOWN_SIGNER),

	ERRNAME(OPS_E_ALG),
	ERRNAME(OPS_E_ALG_UNSUPPORTED_SYMMETRIC_ALG),
	ERRNAME(OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG),
	ERRNAME(OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG),
	ERRNAME(OPS_E_ALG_UNSUPPORTED_HASH_ALG),

	ERRNAME(OPS_E_PROTO),
	ERRNAME(OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT),
	ERRNAME(OPS_E_PROTO_UNKNOWN_SS),
	ERRNAME(OPS_E_PROTO_CRITICAL_SS_IGNORED),
	ERRNAME(OPS_E_PROTO_BAD_PUBLIC_KEY_VRSN),
	ERRNAME(OPS_E_PROTO_BAD_SIGNATURE_VRSN),
	ERRNAME(OPS_E_PROTO_BAD_ONE_PASS_SIG_VRSN),
	ERRNAME(OPS_E_PROTO_BAD_PKSK_VRSN),
	ERRNAME(OPS_E_PROTO_DECRYPTED_MSG_WRONG_LEN),
	ERRNAME(OPS_E_PROTO_BAD_SK_CHECKSUM),

	{0x00, NULL}
};

/**
 * \ingroup Core_Print
 *
 * Searches the given map for the given type.
 * Returns a readable string if found, "Unknown" if not.
 */
static const char     *
findtype(int type, __ops_map_t *map)
{
	__ops_map_t      *row;

	for (row = map; row->string != NULL; row++) {
		if (row->type == type) {
			return row->string;
		}
	}
	return "Unknown";
}

/**
 * \ingroup Core_Errors
 * \brief returns error code name
 * \param errcode
 * \return error code name or "Unknown"
 */
static const char     *
__ops_errcode(const __ops_errcode_t errcode)
{
	return findtype((int) errcode, errcode_name_map);
}

/**
\ingroup Core_Errors
\brief print this error
\param err Error to print
*/
static void 
__ops_print_error(__ops_error_t *err)
{
	printf("%s:%d: ", err->file, err->line);
	if (err->errcode == OPS_E_SYSTEM_ERROR) {
		printf("system error %d returned from %s\n", err->sys_errno,
		       err->comment);
	} else {
		printf("%s, %s\n", __ops_errcode(err->errcode), err->comment);
	}
}

/**
\ingroup Core_Errors
\brief Print all errors on stack
\param errstack Error stack to print
*/
static void 
__ops_print_errors(__ops_error_t *errstack)
{
	__ops_error_t    *err;

	for (err = errstack; err != NULL; err = err->next) {
		__ops_print_error(err);
	}
}

/**
\ingroup Core_Errors
\brief Frees all errors on stack
\param errstack Error stack to free
*/
static void 
__ops_free_errors(__ops_error_t *errstack)
{
	__ops_error_t    *next;

	while (errstack != NULL) {
		next = errstack->next;
		(void) free(errstack->comment);
		(void) free(errstack);
		errstack = next;
	}
}

/**
\ingroup HighLevel_Memory
\brief Free memory and associated data
\param mem Memory to free
\note This does not free mem itself
\sa __ops_memory_clear()
\sa __ops_memory_free()
*/
static void 
__ops_memory_release(__ops_memory_t *mem)
{
	if (mem->mmapped) {
		(void) munmap(mem->buf, mem->length);
	} else {
		(void) free(mem->buf);
	}
	mem->buf = NULL;
	mem->length = 0;
}

/**
   \ingroup HighLevel_Memory
   \brief Free memory ptr and associated memory
   \param mem Memory to be freed
   \sa __ops_memory_release()
   \sa __ops_memory_clear()
*/
static void 
__ops_memory_free(__ops_memory_t *mem)
{
	__ops_memory_release(mem);
	(void) free(mem);
}

/* various message digest functions */

static void 
md5_init(__ops_hash_t *hash)
{
	if (hash->data) {
		loggit("md5_init: hash data non-null");
	}
	hash->data = __ops_new(sizeof(MD5_CTX));
	MD5_Init(hash->data);
}

static void 
md5_add(__ops_hash_t *hash, const uint8_t *data, unsigned length)
{
	MD5_Update(hash->data, data, length);
}

static unsigned 
md5_finish(__ops_hash_t *hash, uint8_t *out)
{
	MD5_Final(out, hash->data);
	free(hash->data);
	hash->data = NULL;
	return 16;
}

static __ops_hash_t __md5 = {
	OPS_HASH_MD5,
	MD5_DIGEST_LENGTH,
	"MD5",
	md5_init,
	md5_add,
	md5_finish,
	NULL
};

/**
   \ingroup Core_Crypto
   \brief Initialise to MD5
   \param hash Hash to initialise
*/
static void 
__ops_hash_md5(__ops_hash_t *hash)
{
	*hash = __md5;
}

static void 
sha1_init(__ops_hash_t *hash)
{
	if (hash->data) {
		loggit("sha1_init: hash data non-null");
	}
	hash->data = __ops_new(sizeof(SHA_CTX));
	SHA1_Init(hash->data);
}

static void 
sha1_add(__ops_hash_t *hash, const uint8_t *data, unsigned length)
{
	SHA1_Update(hash->data, data, length);
}

static unsigned 
sha1_finish(__ops_hash_t *hash, uint8_t *out)
{
	SHA1_Final(out, hash->data);
	(void) free(hash->data);
	hash->data = NULL;
	return OPS_SHA1_HASH_SIZE;
}

static __ops_hash_t __sha1 = {
	OPS_HASH_SHA1,
	OPS_SHA1_HASH_SIZE,
	"SHA1",
	sha1_init,
	sha1_add,
	sha1_finish,
	NULL
};

/**
   \ingroup Core_Crypto
   \brief Initialise to SHA1
   \param hash Hash to initialise
*/
static void 
__ops_hash_sha1(__ops_hash_t *hash)
{
	*hash = __sha1;
}

static void 
sha256_init(__ops_hash_t *hash)
{
	if (hash->data) {
		loggit("sha256_init: hash data non-null");
	}
	hash->data = __ops_new(sizeof(SHA256_CTX));
	SHA256_Init(hash->data);
}

static void 
sha256_add(__ops_hash_t *hash, const uint8_t *data, unsigned length)
{
	SHA256_Update(hash->data, data, length);
}

static unsigned 
sha256_finish(__ops_hash_t *hash, uint8_t *out)
{
	SHA256_Final(out, hash->data);
	(void) free(hash->data);
	hash->data = NULL;
	return SHA256_DIGEST_LENGTH;
}

static __ops_hash_t __sha256 = {
	OPS_HASH_SHA256,
	SHA256_DIGEST_LENGTH,
	"SHA256",
	sha256_init,
	sha256_add,
	sha256_finish,
	NULL
};

static void 
__ops_hash_sha256(__ops_hash_t *hash)
{
	*hash = __sha256;
}

#ifdef USE_SHA384
/*
 * SHA384
 */
static void 
sha384_init(__ops_hash_t *hash)
{
	if (hash->data) {
		loggit("sha384_init: hash data non-null");
	}
	hash->data = __ops_new(sizeof(SHA512_CTX));
	SHA384_Init(hash->data);
}

static void 
sha384_add(__ops_hash_t *hash, const uint8_t *data, unsigned length)
{
	SHA384_Update(hash->data, data, length);
}

static unsigned 
sha384_finish(__ops_hash_t *hash, uint8_t *out)
{
	SHA384_Final(out, hash->data);
	(void) free(hash->data);
	hash->data = NULL;
	return SHA384_DIGEST_LENGTH;
}

static __ops_hash_t __sha384 = {
	OPS_HASH_SHA384,
	SHA384_DIGEST_LENGTH,
	"SHA384",
	sha384_init,
	sha384_add,
	sha384_finish,
	NULL
};

static void 
__ops_hash_sha384(__ops_hash_t *hash)
{
	*hash = __sha384;
}
#endif

/*
 * SHA512
 */
static void 
sha512_init(__ops_hash_t *hash)
{
	if (hash->data) {
		loggit("sha512_init: hash data non-null");
	}
	hash->data = __ops_new(sizeof(SHA512_CTX));
	SHA512_Init(hash->data);
}

static void 
sha512_add(__ops_hash_t *hash, const uint8_t *data, unsigned length)
{
	SHA512_Update(hash->data, data, length);
}

static unsigned 
sha512_finish(__ops_hash_t *hash, uint8_t *out)
{
	SHA512_Final(out, hash->data);
	(void) free(hash->data);
	hash->data = NULL;
	return SHA512_DIGEST_LENGTH;
}

static __ops_hash_t __sha512 = {
	OPS_HASH_SHA512,
	SHA512_DIGEST_LENGTH,
	"SHA512",
	sha512_init,
	sha512_add,
	sha512_finish,
	NULL
};

static void 
__ops_hash_sha512(__ops_hash_t *hash)
{
	*hash = __sha512;
}

#ifdef USE_SHA224
/*
 * SHA224
 */

static void 
sha224_init(__ops_hash_t *hash)
{
	if (hash->data) {
		loggit("sha224_init: hash data non-null");
	}
	hash->data = __ops_new(sizeof(SHA256_CTX));
	SHA224_Init(hash->data);
}

static void 
sha224_add(__ops_hash_t *hash, const uint8_t *data, unsigned length)
{
	SHA224_Update(hash->data, data, length);
}

static unsigned 
sha224_finish(__ops_hash_t *hash, uint8_t *out)
{
	SHA224_Final(out, hash->data);
	(void) free(hash->data);
	hash->data = NULL;
	return SHA224_DIGEST_LENGTH;
}

static __ops_hash_t __sha224 = {
	OPS_HASH_SHA224,
	SHA224_DIGEST_LENGTH,
	"SHA224",
	sha224_init,
	sha224_add,
	sha224_finish,
	NULL
};

static void 
__ops_hash_sha224(__ops_hash_t *hash)
{
	*hash = __sha224;
}
#endif

/**
\ingroup Core_Hashes
\brief Add to the hash
\param hash Hash to add to
\param n Int to add
\param length Length of int in bytes
*/
static void 
__ops_hash_add_int(__ops_hash_t *hash, unsigned n, unsigned length)
{
	while (length--) {
		uint8_t   c;

		c = n >> (length * 8);
		hash->add(hash, &c, 1);
	}
}

static void __ops_build_pubkey(const __ops_pubkey_t *, __ops_memory_t *);

/**
 * \ingroup Core_Keys
 * \brief Calculate a public key fingerprint.
 * \param fp Where to put the calculated fingerprint
 * \param key The key for which the fingerprint is calculated
 */

static void 
__ops_fingerprint(__ops_fingerprint_t *fp, const __ops_pubkey_t *key)
{
	if (key->version == 2 || key->version == 3) {
		uint8_t  *bn;
		__ops_hash_t	md5;
		size_t		n;

		if (key->alg != OPS_PKA_RSA &&
		    key->alg != OPS_PKA_RSA_ENCRYPT_ONLY &&
		    key->alg != OPS_PKA_RSA_SIGN_ONLY) {
			loggit("__ops_fingerprint: bad algorithm");
			return;
		}
		/* start the md5 digest */
		__ops_hash_md5(&md5);
		md5.init(&md5);
		n = BN_num_bytes(key->key.rsa.n);
		/* add the digests of the RSA numbers */
		bn = __ops_new(n);
		BN_bn2bin(key->key.rsa.n, bn);
		md5.add(&md5, bn, n);
		(void) free(bn);
		n = BN_num_bytes(key->key.rsa.e);
		bn = __ops_new(n);
		BN_bn2bin(key->key.rsa.e, bn);
		md5.add(&md5, bn, n);
		(void) free(bn);
		fp->length = md5.finish(&md5, fp->fingerprint);
	} else {
		__ops_memory_t	*mem;
		__ops_hash_t	 sha1;
		size_t		 len;

		mem = __ops_new(sizeof(*mem));
		__ops_build_pubkey(key, mem);

		__ops_hash_sha1(&sha1);
		sha1.init(&sha1);

		len = mem->length;

		__ops_hash_add_int(&sha1, 0x99, 1);
		__ops_hash_add_int(&sha1, len, 2);
		sha1.add(&sha1, mem->buf, len);
		sha1.finish(&sha1, fp->fingerprint);

		fp->length = OPS_FINGERPRINT_SIZE;

		__ops_memory_free(mem);
	}
}

/**
 * \ingroup Core_Keys
 * \brief Calculate the Key ID from the public key.
 * \param keyid Space for the calculated ID to be stored
 * \param key The key for which the ID is calculated
 */

static void 
__ops_keyid(uint8_t *keyid, const size_t idlen, const __ops_pubkey_t *key)
{
	if (key->version == 2 || key->version == 3) {
		uint8_t   bn[NETPGP_BUFSIZ];
		unsigned        n = BN_num_bytes(key->key.rsa.n);

		if (n > sizeof(bn)) {
			loggit("__ops_keyid: bad num bytes");
			return;
		}
		if (key->alg != OPS_PKA_RSA &&
		    key->alg != OPS_PKA_RSA_ENCRYPT_ONLY &&
		    key->alg != OPS_PKA_RSA_SIGN_ONLY) {
			loggit("__ops_keyid: bad algorithm");
			return;
		}
		BN_bn2bin(key->key.rsa.n, bn);
		(void) memcpy(keyid, bn + n - idlen, idlen);
	} else {
		__ops_fingerprint_t finger;

		__ops_fingerprint(&finger, key);
		(void) memcpy(keyid,
				finger.fingerprint + finger.length - idlen,
				idlen);
	}
}

/**
\ingroup Core_Hashes
\brief Setup hash for given hash algorithm
\param hash Hash to set up
\param alg Hash algorithm to use
*/
static void 
__ops_hash_any(__ops_hash_t *hash, __ops_hash_alg_t alg)
{
	switch (alg) {
	case OPS_HASH_MD5:
		__ops_hash_md5(hash);
		break;

	case OPS_HASH_SHA1:
		__ops_hash_sha1(hash);
		break;

	case OPS_HASH_SHA256:
		__ops_hash_sha256(hash);
		break;

#ifdef USE_SHA384
	case OPS_HASH_SHA384:
		__ops_hash_sha384(hash);
		break;
#endif

	case OPS_HASH_SHA512:
		__ops_hash_sha512(hash);
		break;

#ifdef USE_SHA224
	case OPS_HASH_SHA224:
		__ops_hash_sha224(hash);
		break;
#endif

	default:
		loggit("__ops_hash_any: bad algorithm");
	}
}

/**
\ingroup Core_Hashes
\brief Returns size of hash for given hash algorithm
\param alg Hash algorithm to use
\return Size of hash algorithm in bytes
*/
static unsigned 
__ops_hash_size(__ops_hash_alg_t alg)
{
	switch (alg) {
	case OPS_HASH_MD5:
		return 16;

	case OPS_HASH_SHA1:
		return 20;

	case OPS_HASH_SHA256:
		return 32;

#ifdef USE_SHA224
	case OPS_HASH_SHA224:
		return 28;
#endif

#ifdef USE_SHA384
	case OPS_HASH_SHA384:
		return 48;
#endif

	case OPS_HASH_SHA512:
		return 64;

	default:
		loggit("__ops_hash_size: bad algorithm");
	}

	return 0;
}

/**
\ingroup Core_Hashes
\brief Returns hash enum corresponding to given string
\param hash Text name of hash algorithm i.e. "SHA1"
\returns Corresponding enum i.e. OPS_HASH_SHA1
*/
static __ops_hash_alg_t 
__ops_str_to_hash_alg(const char *hash)
{
	if (strcmp(hash, "SHA1") == 0) {
		return OPS_HASH_SHA1;
	}
	if (strcmp(hash, "MD5") == 0) {
		return OPS_HASH_MD5;
	}
	if (strcmp(hash, "SHA256") == 0) {
		return OPS_HASH_SHA256;
	}
#ifdef USE_SHA224
	/*
        if (strcmp(hash,"SHA224") == 0) {
		return OPS_HASH_SHA224;
	}
        */
#endif
#ifdef USE_SHA384
	if (strcmp(hash, "SHA384") == 0) {
		return OPS_HASH_SHA384;
	}
#endif
	if (strcmp(hash, "SHA512") == 0) {
		return OPS_HASH_SHA512;
	}
	return OPS_HASH_UNKNOWN;
}


/**
\ingroup Core_Hashes
\brief Calculate hash for MDC packet
\param preamble Preamble to hash
\param sz_preamble Size of preamble
\param plaintext Plaintext to hash
\param sz_plaintext Size of plaintext
\param hashed Resulting hash
*/
static void 
__ops_calc_mdc_hash(const uint8_t *preamble,
			const size_t sz_preamble,
			const uint8_t *plaintext,
			const unsigned int sz_plaintext,
			uint8_t *hashed)
{
	uint8_t	c;
	__ops_hash_t	hash;

	/* init */
	__ops_hash_any(&hash, OPS_HASH_SHA1);
	hash.init(&hash);

	/* preamble */
	hash.add(&hash, preamble, sz_preamble);
	/* plaintext */
	hash.add(&hash, plaintext, sz_plaintext);
	/* MDC packet tag */
	c = MDC_PKT_TAG;
	hash.add(&hash, &c, 1);
	/* MDC packet len */
	c = OPS_SHA1_HASH_SIZE;
	hash.add(&hash, &c, 1);

	/* finish */
	hash.finish(&hash, hashed);
}

/**
\ingroup HighLevel_Supported
\brief Is this Hash Algorithm supported?
\param hash_alg Hash Algorithm to check
\return 1 if supported; else 0
*/
static unsigned 
__ops_is_hash_alg_supported(const __ops_hash_alg_t *hash_alg)
{
	switch (*hash_alg) {
	case OPS_HASH_MD5:
	case OPS_HASH_SHA1:
	case OPS_HASH_SHA256:
		return 1;

	default:
		return 0;
	}
}


/**
\ingroup HighLevel_Memory
\brief Memory to initialise
\param mem memory to initialise
\param needed Size to initialise to
*/
static void 
__ops_memory_init(__ops_memory_t *mem, size_t needed)
{
	mem->length = 0;
	if (mem->buf) {
		if (mem->allocated < needed) {
			mem->buf = realloc(mem->buf, needed);
			mem->allocated = needed;
		}
		return;
	}
	mem->buf = __ops_new(needed);
	mem->allocated = needed;
}

/**
\ingroup HighLevel_Memory
\brief Pad memory to required length
\param mem Memory to use
\param length New size
*/
static void 
__ops_memory_pad(__ops_memory_t *mem, size_t length)
{
	if (mem->allocated < mem->length) {
		loggit("__ops_memory_pad: bad alloc in");
		return;
	}
	if (mem->allocated < mem->length + length) {
		mem->allocated = (mem->allocated * 2) + length;
		mem->buf = realloc(mem->buf, mem->allocated);
	}
	if (mem->allocated < mem->length + length) {
		loggit("__ops_memory_pad: bad alloc out");
	}
}

/**
\ingroup HighLevel_Memory
\brief Add data to memory
\param mem Memory to which to add
\param src Data to add
\param length Length of data to add
*/
static void 
__ops_memory_add(__ops_memory_t *mem, const uint8_t *src, size_t length)
{
	__ops_memory_pad(mem, length);
	(void) memcpy(mem->buf + mem->length, src, length);
	mem->length += length;
}

/* read a file into an __ops_memory_t */
static int
__ops_mem_readfile(__ops_memory_t *mem, const char *f)
{
	struct stat	 st;
	FILE		*fp;
	int		 cc;

	if ((fp = fopen(f, "rb")) == NULL) {
		loggit( "__ops_mem_readfile: can't open \"%s\"", f);
		return 0;
	}
	(void) fstat(fileno(fp), &st);
	mem->allocated = (size_t)st.st_size;
	mem->buf = mmap(NULL, mem->allocated, PROT_READ,
				MAP_FILE | MAP_PRIVATE, fileno(fp), 0);
	if (mem->buf == MAP_FAILED) {
		/* mmap failed for some reason - try to allocate memory */
		if ((mem->buf = __ops_new(mem->allocated)) == NULL) {
			loggit("__ops_mem_readfile: alloc");
			(void) fclose(fp);
			return 0;
		}
		/* read into contents of mem */
		for (mem->length = 0 ;
		     (cc = read(fileno(fp), &mem->buf[mem->length],
					mem->allocated - mem->length)) > 0 ;
		     mem->length += (size_t)cc) {
		}
	} else {
		mem->length = mem->allocated;
		mem->mmapped = 1;
	}
	(void) fclose(fp);
	return (mem->allocated == mem->length);
}

typedef struct {
	uint16_t  sum;
} sum16_t;

/*
 * XXX: replace __ops_ptag_t with something more appropriate for limiting reads
 */

/**
 * low-level function to read data from reader function
 *
 * Use this function, rather than calling the reader directly.
 *
 * If the accumulate flag is set in *stream, the function
 * adds the read data to the accumulated data, and updates
 * the accumulated length. This is useful if, for example,
 * the application wants access to the raw data as well as the
 * parsed data.
 *
 * This function will also try to read the entire amount asked for, but not
 * if it is over INT_MAX. Obviously many callers will know that they
 * never ask for that much and so can avoid the extra complexity of
 * dealing with return codes and filled-in lengths.
 *
 * \param *dest
 * \param *plength
 * \param flags
 * \param *stream
 *
 * \return OPS_R_OK
 * \return OPS_R_PARTIAL_READ
 * \return OPS_R_EOF
 * \return OPS_R_EARLY_EOF
 *
 * \sa #__ops_reader_ret_t for details of return codes
 */

static int 
sub_base_read(void *dest, size_t length, __ops_error_t **errors,
	      __ops_reader_t *readinfo, __ops_cbdata_t *cbdata)
{
	size_t          n;

	/* reading more than this would look like an error */
	if (length > INT_MAX) {
		length = INT_MAX;
	}

	for (n = 0; n < length;) {
		int	r;

		r = readinfo->reader((char *) dest + n, length - n, errors,
				readinfo, cbdata);
		if (r > (int)(length - n)) {
			loggit("sub_base_read: bad read");
			return 0;
		}
		if (r < 0) {
			return r;
		}
		if (r == 0) {
			break;
		}
		n += r;
	}

	if (n == 0) {
		return 0;
	}
	if (readinfo->accumulate) {
		if (readinfo->asize < readinfo->alength) {
			loggit("sub_base_read: bad size");
			return 0;
		}
		if (readinfo->alength + n > readinfo->asize) {
			readinfo->asize = readinfo->asize * 2 + n;
			readinfo->accumulated = realloc(readinfo->accumulated,
							readinfo->asize);
		}
		if (readinfo->asize < readinfo->alength + n) {
			loggit("sub_base_read: bad realloc");
			return 0;
		}
		(void) memcpy(readinfo->accumulated + readinfo->alength, dest,
				n);
	}
	/* we track length anyway, because it is used for packet offsets */
	readinfo->alength += n;
	/* and also the position */
	readinfo->position += n;

	return n;
}

static int 
__ops_stacked_read(void *dest, size_t length, __ops_error_t **errors,
		 __ops_reader_t *readinfo, __ops_cbdata_t *cbdata)
{
	return sub_base_read(dest, length, errors, readinfo->next, cbdata);
}

static int 
sum16_reader(void *destarg, size_t length, __ops_error_t **errors,
	     __ops_reader_t *readinfo, __ops_cbdata_t *cbdata)
{
	const uint8_t	*dest = destarg;
	sum16_t			*arg;
	int			 r;
	int			 n;

	arg = readinfo->arg;
	r = __ops_stacked_read(destarg, length, errors, readinfo, cbdata);
	if (r < 0) {
		return r;
	}
	for (n = 0; n < r; ++n) {
		arg->sum = (arg->sum + dest[n]) & 0xffff;
	}
	return r;
}

static void 
sum16_destroyer(__ops_reader_t *readinfo)
{
	(void) free(readinfo->arg);
}

/**
 * \ingroup Internal_Readers_Generic
 * \brief Starts reader stack
 * \param stream Parse settings
 * \param reader Reader to use
 * \param destroyer Destroyer to use
 * \param vp Reader-specific arg
 */
static void 
__ops_reader_set(__ops_stream_t *stream,
		int (*reader)(void *, size_t, __ops_error_t **,
				__ops_reader_t *, __ops_cbdata_t *),
		void (*destroyer)(__ops_reader_t *),
		void *vp)
{
	stream->readinfo.reader = reader;
	stream->readinfo.destroyer = destroyer;
	stream->readinfo.arg = vp;
}

/**
 * \ingroup Internal_Readers_Generic
 * \brief Adds to reader stack
 * \param stream Parse settings
 * \param reader Reader to use
 * \param destroyer Reader's destroyer
 * \param vp Reader-specific arg
 */
static void 
__ops_reader_push(__ops_stream_t *stream,
		int (*reader)(void *, size_t, __ops_error_t **,
				__ops_reader_t *, __ops_cbdata_t *),
		void (*destroyer)(__ops_reader_t *),
		void *vp)
{
	__ops_reader_t *readinfo = __ops_new(sizeof(*readinfo));

	*readinfo = stream->readinfo;
	(void) memset(&stream->readinfo, 0x0, sizeof(stream->readinfo));
	stream->readinfo.next = readinfo;
	stream->readinfo.parent = stream;

	/* should copy accumulate flags from other reader? RW */
	stream->readinfo.accumulate = readinfo->accumulate;

	__ops_reader_set(stream, reader, destroyer, vp);
}

/**
   \ingroup Internal_Readers_Sum16
   \param stream Parse settings
*/

static void 
__ops_reader_push_sum16(__ops_stream_t *stream)
{
	sum16_t    *arg;

	arg = __ops_new(sizeof(*arg));
	__ops_reader_push(stream, sum16_reader, sum16_destroyer, arg);
}

/**
 * \ingroup Internal_Readers_Generic
 * \brief Removes from reader stack
 * \param stream Parse settings
 */
static void 
__ops_reader_pop(__ops_stream_t *stream)
{
	__ops_reader_t *next = stream->readinfo.next;

	stream->readinfo = *next;
	(void) free(next);
}

/**
   \ingroup Internal_Readers_Sum16
   \param stream Parse settings
   \return sum
*/
static uint16_t 
__ops_reader_pop_sum16(__ops_stream_t *stream)
{
	uint16_t	 sum;
	sum16_t		*arg;

	arg = stream->readinfo.arg;
	sum = arg->sum;
	__ops_reader_pop(stream);
	free(arg);
	return sum;
}

/* return the version for the library */
static const char *
__ops_get_info(const char *type)
{
	if (strcmp(type, "version") == 0) {
		return NETPGP_VERSION_STRING;
	}
	if (strcmp(type, "maintainer") == 0) {
		return NETPGP_MAINTAINER;
	}
	return "[unknown]";
}


static unsigned 
__ops_dsa_verify(const uint8_t *hash, size_t hash_length,
	       const __ops_dsa_sig_t *sig,
	       const __ops_dsa_pubkey_t *dsa)
{
	unsigned	qlen;
	DSA_SIG        *osig;
	DSA            *odsa;
	int             ret;

	osig = DSA_SIG_new();
	osig->r = sig->r;
	osig->s = sig->s;

	odsa = DSA_new();
	odsa->p = dsa->p;
	odsa->q = dsa->q;
	odsa->g = dsa->g;
	odsa->pub_key = dsa->y;

	if ((qlen = BN_num_bytes(odsa->q)) < hash_length) {
		hash_length = qlen;
	}
	ret = DSA_do_verify(hash, (int)hash_length, osig, odsa);
	if (ret < 0) {
		loggit("__ops_dsa_verify: DSA verification");
		return 0;
	}

	odsa->p = odsa->q = odsa->g = odsa->pub_key = NULL;
	DSA_free(odsa);

	osig->r = osig->s = NULL;
	DSA_SIG_free(osig);

	return ret;
}

/**
   \ingroup Core_Crypto
   \brief Recovers message digest from the signature
   \param out Where to write decrypted data to
   \param in Encrypted data
   \param length Length of encrypted data
   \param pubkey RSA public key
   \return size of recovered message digest
*/
static int 
__ops_rsa_public_decrypt(uint8_t *out,
			const uint8_t *in,
			size_t length,
			const __ops_rsa_pubkey_t *pubkey)
{
	RSA            *orsa;
	int             n;

	orsa = RSA_new();
	orsa->n = pubkey->n;
	orsa->e = pubkey->e;

	n = RSA_public_decrypt((int)length, in, out, orsa, RSA_NO_PADDING);

	orsa->n = orsa->e = NULL;
	RSA_free(orsa);

	return n;
}

/**
\ingroup Core_Crypto
\brief Decrypts RSA-encrypted data
\param out Where to write the plaintext
\param in Encrypted data
\param length Length of encrypted data
\param seckey RSA secret key
\param pubkey RSA public key
\return size of recovered plaintext
*/
static int 
__ops_rsa_private_decrypt(uint8_t *out,
			const uint8_t *in,
			size_t length,
			const __ops_rsa_seckey_t *seckey,
			const __ops_rsa_pubkey_t *pubkey)
{
	RSA            *keypair;
	int             n;
	char            errbuf[1024];

	keypair = RSA_new();
	keypair->n = pubkey->n;	/* XXX: do we need n? */
	keypair->d = seckey->d;
	keypair->p = seckey->q;
	keypair->q = seckey->p;

	/* debug */
	keypair->e = pubkey->e;
	if (RSA_check_key(keypair) != 1) {
		loggit("RSA_check_key is not set");
		return 0;
	}
	/* end debug */

	n = RSA_private_decrypt((int)length, in, out, keypair, RSA_NO_PADDING);

	errbuf[0] = '\0';
	if (n == -1) {
		unsigned long   err = ERR_get_error();

		ERR_error_string(err, &errbuf[0]);
		loggit("openssl error : %s", errbuf);
	}
	keypair->n = keypair->d = keypair->p = keypair->q = NULL;
	RSA_free(keypair);

	return n;
}

static void 
copy_sig_info(__ops_signinfo_t *dst, const __ops_signinfo_t *src)
{
	(void) memcpy(dst, src, sizeof(*src));
	dst->v4_hashed = __ops_new(src->v4_hashlen);
	(void) memcpy(dst->v4_hashed, src->v4_hashed, src->v4_hashlen);
}

static void 
add_sig_to_list(const __ops_signinfo_t *sig, __ops_signinfo_t **sigs,
			unsigned *count)
{
	if (*count == 0) {
		*sigs = __ops_new((*count + 1) * sizeof(__ops_signinfo_t));
	} else {
		*sigs = realloc(*sigs,
				(*count + 1) * sizeof(__ops_signinfo_t));
	}
	copy_sig_info(&(*sigs)[*count], sig);
	*count += 1;
}

/**
 \ingroup HighLevel_KeyGeneral

 \brief Returns the public key in the given key.
 \param key

  \return Pointer to public key

  \note This is not a copy, do not free it after use.
*/

static const __ops_pubkey_t *
__ops_get_pubkey(const __ops_key_t *key)
{
	return (key->type == OPS_PTAG_CT_PUBLIC_KEY) ?
				&key->key.pubkey :
				&key->key.seckey.pubkey;
}

/**
   \ingroup HighLevel_KeyringFind

   \brief Finds key in keyring from its Key ID

   \param keyring Keyring to be searched
   \param keyid ID of required key

   \return Pointer to key, if found; NULL, if not found

   \note This returns a pointer to the key inside the given keyring,
   not a copy.  Do not free it after use.

*/
static const __ops_key_t *
__ops_getkeybyid(const __ops_keyring_t *keyring,
			   const uint8_t keyid[OPS_KEY_ID_SIZE])
{
	unsigned	n;

	for (n = 0; keyring && n < keyring->keyc; n++) {
		if (memcmp(keyring->keys[n].keyid, keyid,
					OPS_KEY_ID_SIZE) == 0) {
			return &keyring->keys[n];
		}
		if (memcmp(&keyring->keys[n].keyid[OPS_KEY_ID_SIZE / 2],
				keyid, OPS_KEY_ID_SIZE / 2) == 0) {
			return &keyring->keys[n];
		}
	}
	return NULL;
}

static unsigned 
__ops_check_sig(const uint8_t *, unsigned,
		    const __ops_sig_t *,
		    const __ops_pubkey_t *);

/* Does the signed hash match the given hash? */
static unsigned
check_binary_sig(const unsigned len,
		const uint8_t *data,
		const __ops_sig_t *sig,
		const __ops_pubkey_t *signer)
{
	uint8_t   hashout[OPS_MAX_HASH_SIZE];
	uint8_t   trailer[6];
	unsigned int    hashedlen;
	__ops_hash_t	hash;
	unsigned	n = 0;

	__OPS_USED(signer);
	__ops_hash_any(&hash, sig->info.hash_alg);
	hash.init(&hash);
	hash.add(&hash, data, len);
	switch (sig->info.version) {
	case OPS_V3:
		trailer[0] = sig->info.type;
		trailer[1] = (unsigned)(sig->info.birthtime) >> 24;
		trailer[2] = (unsigned)(sig->info.birthtime) >> 16;
		trailer[3] = (unsigned)(sig->info.birthtime) >> 8;
		trailer[4] = (uint8_t)(sig->info.birthtime);
		hash.add(&hash, &trailer[0], 5);
		break;

	case OPS_V4:
		hash.add(&hash, sig->info.v4_hashed, sig->info.v4_hashlen);
		trailer[0] = 0x04;	/* version */
		trailer[1] = 0xFF;
		hashedlen = sig->info.v4_hashlen;
		trailer[2] = hashedlen >> 24;
		trailer[3] = hashedlen >> 16;
		trailer[4] = hashedlen >> 8;
		trailer[5] = hashedlen;
		hash.add(&hash, trailer, 6);
		break;

	default:
		loggit("Invalid signature version %d",
				sig->info.version);
		return 0;
	}
	n = hash.finish(&hash, hashout);
	return __ops_check_sig(hashout, n, sig, signer);
}

static __ops_cb_ret_t
validate_data_cb(__ops_cbdata_t *cbdata, const __ops_packet_t *pkt)
{
	const __ops_contents_t	 *content = &pkt->u;
	const __ops_key_t	 *signer;
	validate_data_cb_t	 *data;
	__ops_error_t		**errors;
	__ops_io_t		 *io;
	unsigned		  valid = 0;

	io = cbdata->io;
	data = cbdata->arg;
	errors = cbdata->errors;
	switch (pkt->tag) {
	case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
		/*
		 * ignore - this gives us the "Armor Header" line "Hash:
		 * SHA1" or similar
		 */
		break;

	case OPS_PTAG_CT_LITERAL_DATA_HEADER:
		/* ignore */
		break;

	case OPS_PTAG_CT_LITERAL_DATA_BODY:
		data->data.litdata_body = content->litdata_body;
		data->type = LITERAL_DATA;
		__ops_memory_add(data->mem, data->data.litdata_body.data,
				       data->data.litdata_body.length);
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
		data->data.cleartext_body = content->cleartext_body;
		data->type = SIGNED_CLEARTEXT;
		__ops_memory_add(data->mem, data->data.litdata_body.data,
			       data->data.litdata_body.length);
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:
		/* this gives us an __ops_hash_t struct */
		break;

	case OPS_PTAG_CT_SIGNATURE:	/* V3 sigs */
	case OPS_PTAG_CT_SIGNATURE_FOOTER:	/* V4 sigs */
		signer = __ops_getkeybyid(data->keyring,
					 content->sig.info.signer);
		if (!signer) {
			OPS_ERROR(errors, OPS_E_V_UNKNOWN_SIGNER,
					"Unknown Signer");
			add_sig_to_list(&content->sig.info,
					&data->result->unknowns,
					&data->result->unknownc);
			break;
		}
		switch (content->sig.info.type) {
		case OPS_SIG_BINARY:
		case OPS_SIG_TEXT:
			if (data->mem->length == 0 &&
			    data->detachname) {
				/* check we have seen some data */
				/* if not, need to read from detached name */
				loggit(
				"netpgp: assuming signed data in \"%s\"",
					data->detachname);
				data->mem = __ops_new(sizeof(*data->mem));
				__ops_mem_readfile(data->mem, data->detachname);
			}
			valid = check_binary_sig(data->mem->length,
					data->mem->buf,
					&content->sig,
					__ops_get_pubkey(signer));
			break;

		default:
			OPS_ERROR_1(errors, OPS_E_UNIMPLEMENTED,
				    "No Sig Verification type 0x%02x yet\n",
				    content->sig.info.type);
			break;

		}

		if (valid) {
			add_sig_to_list(&content->sig.info,
					&data->result->valids,
					&data->result->validc);
		} else {
			OPS_ERROR(errors, OPS_E_V_BAD_SIGNATURE,
					"Bad Signature");
			add_sig_to_list(&content->sig.info,
					&data->result->invalids,
					&data->result->invalidc);
		}
		break;

		/* ignore these */
	case OPS_PARSER_PTAG:
	case OPS_PTAG_CT_SIGNATURE_HEADER:
	case OPS_PTAG_CT_ARMOUR_HEADER:
	case OPS_PTAG_CT_ARMOUR_TRAILER:
	case OPS_PTAG_CT_1_PASS_SIG:
		break;

	case OPS_PARSER_PACKET_END:
		break;

	default:
		OPS_ERROR(errors, OPS_E_V_NO_SIGNATURE, "No signature");
		break;
	}
	return OPS_RELEASE_MEMORY;
}



/**
 * \ingroup HighLevel_Verify
 * \brief Indicicates whether any errors were found
 * \param result Validation result to check
 * \return 0 if any invalid signatures or unknown signers
 	or no valid signatures; else 1
 */
static unsigned 
validate_result_status(__ops_validation_t *val)
{
	return val->validc && !val->invalidc && !val->unknownc;
}

/* utility function to zero out memory */
static void
__ops_forget(void *vp, unsigned size)
{
	(void) memset(vp, 0x0, size);
}

/**
 * \ingroup Core_ReadPackets
 *
 * \brief Specifies whether one or more signature
 * subpacket types should be returned parsed; or raw; or ignored.
 *
 * \param	stream	Pointer to previously allocated structure
 * \param	tag	Packet tag. OPS_PTAG_SS_ALL for all SS tags; or one individual signature subpacket tag
 * \param	type	Parse type
 * \todo Make all packet types optional, not just subpackets */
static void 
__ops_stream_options(__ops_stream_t *stream,
		  __ops_content_tag_t tag,
		  __ops_parse_type_t type)
{
	int             t8, t7;

	if (tag == OPS_PTAG_SS_ALL) {
		int             n;

		for (n = 0; n < 256; ++n) {
			__ops_stream_options(stream,
				OPS_PTAG_SIG_SUBPKT_BASE + n,
				type);
		}
		return;
	}
	if (tag < OPS_PTAG_SIG_SUBPKT_BASE ||
	    tag > OPS_PTAG_SIG_SUBPKT_BASE + NTAGS - 1) {
		loggit("__ops_stream_options: bad tag");
		return;
	}
	t8 = (tag - OPS_PTAG_SIG_SUBPKT_BASE) / 8;
	t7 = 1 << ((tag - OPS_PTAG_SIG_SUBPKT_BASE) & 7);
	switch (type) {
	case OPS_PARSE_RAW:
		stream->ss_raw[t8] |= t7;
		stream->ss_parsed[t8] &= ~t7;
		break;

	case OPS_PARSE_PARSED:
		stream->ss_raw[t8] &= ~t7;
		stream->ss_parsed[t8] |= t7;
		break;

	case OPS_PARSE_IGNORE:
		stream->ss_raw[t8] &= ~t7;
		stream->ss_parsed[t8] &= ~t7;
		break;
	}
}

/**
\ingroup Core_ReadPackets
\brief Free __ops_stream_t struct and its contents
*/
static void 
__ops_stream_delete(__ops_stream_t *stream)
{
	__ops_cbdata_t	*cbdata;
	__ops_cbdata_t	*next;

	for (cbdata = stream->cbdata.next; cbdata; cbdata = next) {
		next = cbdata->next;
		(void) free(cbdata);
	}
	if (stream->readinfo.destroyer) {
		stream->readinfo.destroyer(&stream->readinfo);
	}
	__ops_free_errors(stream->errors);
	if (stream->readinfo.accumulated) {
		(void) free(stream->readinfo.accumulated);
	}
	(void) free(stream);
}

static uint8_t prefix_md5[] = {
	0x30, 0x20, 0x30, 0x0C, 0x06, 0x08, 0x2A, 0x86, 0x48, 0x86,
	0xF7, 0x0D, 0x02, 0x05, 0x05, 0x00, 0x04, 0x10
};

static uint8_t prefix_sha1[] = {
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0E, 0x03, 0x02,
	0x1A, 0x05, 0x00, 0x04, 0x14
};

static uint8_t prefix_sha256[] = {
	0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
	0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20
};

static unsigned 
rsa_verify(__ops_hash_alg_t type,
	   const uint8_t *hash,
	   size_t hash_length,
	   const __ops_rsa_sig_t *sig,
	   const __ops_rsa_pubkey_t *pubrsa)
{
	const uint8_t	*prefix;
	unsigned       	 n;
	unsigned       	 keysize;
	unsigned	 plen;
	uint8_t   	 sigbuf[NETPGP_BUFSIZ];
	uint8_t   	 hashbuf_from_sig[NETPGP_BUFSIZ];

	plen = 0;
	prefix = (const uint8_t *) "";
	keysize = BN_num_bytes(pubrsa->n);
	/* RSA key can't be bigger than 65535 bits, so... */
	if (keysize > sizeof(hashbuf_from_sig)) {
		loggit("rsa_verify: keysize too big");
		return 0;
	}
	if ((unsigned) BN_num_bits(sig->sig) > 8 * sizeof(sigbuf)) {
		loggit("rsa_verify: BN_numbits too big");
		return 0;
	}
	BN_bn2bin(sig->sig, sigbuf);

	n = __ops_rsa_public_decrypt(hashbuf_from_sig, sigbuf,
		(unsigned)(BN_num_bits(sig->sig) + 7) / 8, pubrsa);

	if (n != keysize) {
		/* obviously, this includes error returns */
		return 0;
	}

	/* XXX: why is there a leading 0? The first byte should be 1... */
	/* XXX: because the decrypt should use keysize and not sigsize? */
	if (hashbuf_from_sig[0] != 0 || hashbuf_from_sig[1] != 1) {
		return 0;
	}

	switch (type) {
	case OPS_HASH_MD5:
		prefix = prefix_md5;
		plen = sizeof(prefix_md5);
		break;
	case OPS_HASH_SHA1:
		prefix = prefix_sha1;
		plen = sizeof(prefix_sha1);
		break;
	case OPS_HASH_SHA256:
		prefix = prefix_sha256;
		plen = sizeof(prefix_sha256);
		break;
	default:
		loggit("Unknown hash algorithm: %d", type);
		return 0;
	}

	if (keysize - plen - hash_length < 10) {
		return 0;
	}

	for (n = 2; n < keysize - plen - hash_length - 1; ++n) {
		if (hashbuf_from_sig[n] != 0xff) {
			return 0;
		}
	}

	if (hashbuf_from_sig[n++] != 0) {
		return 0;
	}

	return (memcmp(&hashbuf_from_sig[n], prefix, plen) == 0 &&
	        memcmp(&hashbuf_from_sig[n + plen], hash, hash_length) == 0);
}

/**
   \ingroup Core_Signature
   \brief Checks a signature
   \param hash Signature Hash to be checked
   \param length Signature Length
   \param sig The Signature to be checked
   \param signer The signer's public key
   \return 1 if good; else 0
*/
static unsigned 
__ops_check_sig(const uint8_t *hash, unsigned length,
		    const __ops_sig_t * sig,
		    const __ops_pubkey_t * signer)
{
	unsigned   ret;

	ret = 0;
	switch (sig->info.key_alg) {
	case OPS_PKA_DSA:
		ret = __ops_dsa_verify(hash, length, &sig->info.sig.dsa,
				&signer->key.dsa);
		break;
	case OPS_PKA_RSA:
		ret = rsa_verify(sig->info.hash_alg, hash, length,
				&sig->info.sig.rsa,
				&signer->key.rsa);
		break;
	default:
		loggit("__ops_check_sig: unusual alg");
		ret = 0;
	}
	return ret;
}


/*
 * return 1 if OK, otherwise 0
 */
static unsigned 
base_write(const void *src, unsigned len, __ops_output_t *out)
{
	return out->writer.writer(&out->writer, src, len, &out->errors);
}

/**
 * \ingroup Core_WritePackets
 *
 * \param src
 * \param len
 * \param output
 * \return 1 if OK, otherwise 0
 */

static unsigned 
__ops_write(__ops_output_t *output, const void *src, unsigned len)
{
	return base_write(src, len, output);
}

/**
 * \ingroup Core_WritePackets
 * \param n
 * \param len
 * \param output
 * \return 1 if OK, otherwise 0
 */
static unsigned 
__ops_write_scalar(__ops_output_t *output, unsigned n, unsigned len)
{
	uint8_t   c;

	while (len-- > 0) {
		c = n >> (len * 8);
		if (!base_write(&c, 1, output)) {
			return 0;
		}
	}
	return 1;
}

/**
 * \ingroup Core_WritePackets
 * \param bn
 * \param output
 * \return 1 if OK, otherwise 0
 */

static unsigned 
__ops_write_mpi(__ops_output_t *output, const BIGNUM *bn)
{
	uint8_t   buf[NETPGP_BUFSIZ];
	unsigned	bits = (unsigned)BN_num_bits(bn);

	if (bits > 65535) {
		loggit("__ops_write_mpi: too large %u", bits);
		return 0;
	}
	BN_bn2bin(bn, buf);
	return __ops_write_scalar(output, bits, 2) &&
		__ops_write(output, buf, (bits + 7) / 8);
}

/*
 * Note that we support v3 keys here because they're needed for for
 * verification - the writer doesn't allow them, though
 */
static unsigned 
write_pubkey_body(__ops_output_t *output, const __ops_pubkey_t *key)
{
	if (!(__ops_write_scalar(output, (unsigned)key->version, 1) &&
	      __ops_write_scalar(output, (unsigned)key->birthtime, 4))) {
		return 0;
	}
	if (key->version != 4 &&
	    !__ops_write_scalar(output, key->days_valid, 2)) {
		return 0;
	}
	if (!__ops_write_scalar(output, (unsigned)key->alg, 1)) {
		return 0;
	}
	switch (key->alg) {
	case OPS_PKA_DSA:
		return __ops_write_mpi(output, key->key.dsa.p) &&
			__ops_write_mpi(output, key->key.dsa.q) &&
			__ops_write_mpi(output, key->key.dsa.g) &&
			__ops_write_mpi(output, key->key.dsa.y);

	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		return __ops_write_mpi(output, key->key.rsa.n) &&
			__ops_write_mpi(output, key->key.rsa.e);

	case OPS_PKA_ELGAMAL:
		return __ops_write_mpi(output, key->key.elgamal.p) &&
			__ops_write_mpi(output, key->key.elgamal.g) &&
			__ops_write_mpi(output, key->key.elgamal.y);
	default:
		loggit("write_pubkey_body: bad algorithm");
		break;
	}
	return 0;
}

static unsigned 
memory_writer(__ops_writer_t *writer,
		const uint8_t *src,
		unsigned len,
		__ops_error_t **errors)
{
	__ops_memory_t   *mem;

	__OPS_USED(errors);
	mem = writer->arg;
	__ops_memory_add(mem, src, len);
	return 1;
}

/**
 * \ingroup Core_Writers
 *
 * Set a writer in output. There should not be another writer set.
 *
 * \param output The output structure
 * \param writer
 * \param finaliser
 * \param destroyer
 * \param arg The argument for the writer and destroyer
 */
static void 
__ops_writer_set(__ops_output_t *output,
	       unsigned (*writer)(__ops_writer_t *, const uint8_t *, unsigned,
	       			__ops_error_t **),
	       unsigned (*finaliser)(__ops_error_t **, __ops_writer_t *),
	       void	(*destroyer)(__ops_writer_t *),
	       void *arg)
{
	if (output->writer.writer) {
		loggit("__ops_writer_set: already set");
	} else {
		output->writer.writer = writer;
		output->writer.finaliser = finaliser;
		output->writer.destroyer = destroyer;
		output->writer.arg = arg;
	}
}


/**
 * \ingroup Core_WritersFirst
 * \brief Write to memory
 *
 * Set a memory writer.
 *
 * \param output The output structure
 * \param mem The memory structure
 * \note It is the caller's responsiblity to call __ops_memory_free(mem)
 * \sa __ops_memory_free()
 */
static void 
__ops_writer_set_memory(__ops_output_t *output, __ops_memory_t *mem)
{
	__ops_writer_set(output, memory_writer, NULL, NULL, mem);
}

static void 
writer_info_delete(__ops_writer_t *writer)
{
	/* we should have finalised before deleting */
	if (writer->finaliser) {
		loggit("writer_info_delete: not finalised");
		return;
	}
	if (writer->next) {
		writer_info_delete(writer->next);
		free(writer->next);
		writer->next = NULL;
	}
	if (writer->destroyer) {
		writer->destroyer(writer);
		writer->destroyer = NULL;
	}
	writer->writer = NULL;
}

/**
 * \ingroup Core_Create
 * \brief Delete an __ops_output_t strucut and associated resources.
 *
 * Delete an __ops_output_t structure. If a writer is active, then
 * that is also deleted.
 *
 * \param info the structure to be deleted.
 */
static void 
__ops_output_delete(__ops_output_t *output)
{
	writer_info_delete(&output->writer);
	(void) free(output);
}


/**
 * \ingroup Core_Create
 * \param out
 * \param key
 * \param make_packet
 */
static void 
__ops_build_pubkey(const __ops_pubkey_t *key, __ops_memory_t *out)
{
	__ops_output_t *output;

	output = __ops_new(sizeof(*output));
	__ops_memory_init(out, 128);
	__ops_writer_set_memory(output, out);
	write_pubkey_body(output, key);
	__ops_output_delete(output);
}


static unsigned __ops_key_size(__ops_symm_alg_t);

static __ops_map_t pubkeyalgs[] = {
	{OPS_PKA_RSA, "RSA (Encrypt or Sign)"},
	{OPS_PKA_RSA_ENCRYPT_ONLY, "RSA Encrypt-Only"},
	{OPS_PKA_RSA_SIGN_ONLY, "RSA Sign-Only"},
	{OPS_PKA_ELGAMAL, "Elgamal (Encrypt-Only)"},
	{OPS_PKA_DSA, "DSA"},
	{OPS_PKA_RESERVED_ELLIPTIC_CURVE, "Reserved for Elliptic Curve"},
	{OPS_PKA_RESERVED_ECDSA, "Reserved for ECDSA"},
	{OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN,
			"Reserved (formerly Elgamal Encrypt or Sign"},
	{OPS_PKA_RESERVED_DH, "Reserved for Diffie-Hellman (X9.42)"},
	{OPS_PKA_PRIVATE00, "Private/Experimental"},
	{OPS_PKA_PRIVATE01, "Private/Experimental"},
	{OPS_PKA_PRIVATE02, "Private/Experimental"},
	{OPS_PKA_PRIVATE03, "Private/Experimental"},
	{OPS_PKA_PRIVATE04, "Private/Experimental"},
	{OPS_PKA_PRIVATE05, "Private/Experimental"},
	{OPS_PKA_PRIVATE06, "Private/Experimental"},
	{OPS_PKA_PRIVATE07, "Private/Experimental"},
	{OPS_PKA_PRIVATE08, "Private/Experimental"},
	{OPS_PKA_PRIVATE09, "Private/Experimental"},
	{OPS_PKA_PRIVATE10, "Private/Experimental"},
	{0x00, NULL}
};

static __ops_map_t symm_alg_map[] = {
	{OPS_SA_PLAINTEXT, "Plaintext or unencrypted data"},
	{OPS_SA_IDEA, "IDEA"},
	{OPS_SA_TRIPLEDES, "TripleDES"},
	{OPS_SA_CAST5, "CAST5"},
	{OPS_SA_BLOWFISH, "Blowfish"},
	{OPS_SA_AES_128, "AES (128-bit key)"},
	{OPS_SA_AES_192, "AES (192-bit key)"},
	{OPS_SA_AES_256, "AES (256-bit key)"},
	{OPS_SA_TWOFISH, "Twofish(256-bit key)"},
	{0x00, NULL}
};

/**
\ingroup HighLevel_Supported
\brief Is this Symmetric Algorithm supported?
\param alg Symmetric Algorithm to check
\return 1 if supported; else 0
*/
static unsigned 
__ops_is_sa_supported(__ops_symm_alg_t alg)
{
	switch (alg) {
	case OPS_SA_AES_128:
	case OPS_SA_AES_256:
	case OPS_SA_CAST5:
	case OPS_SA_TRIPLEDES:
#ifndef OPENSSL_NO_IDEA
	case OPS_SA_IDEA:
#endif
		return 1;

	default:
		loggit("\nWarning: %s not supported",
				findtype((int) alg, symm_alg_map));
		return 0;
	}
}

/**
 \ingroup Core_Create
 \brief Calculate the checksum for a session key
 \param sesskey Session Key to use
 \param cs Checksum to be written
 \return 1 if OK; else 0
*/
static unsigned 
__ops_calc_sesskey_checksum(__ops_pk_sesskey_t *sesskey, uint8_t *cs)
{
	unsigned int    i = 0;
	unsigned long   checksum = 0;

	if (!__ops_is_sa_supported(sesskey->symm_alg)) {
		return 0;
	}

	for (i = 0; i < __ops_key_size(sesskey->symm_alg); i++) {
		checksum += sesskey->key[i];
	}
	checksum = checksum % 65536;

	cs[0] = (uint8_t)((checksum >> 8) & 0xff);
	cs[1] = (uint8_t)(checksum & 0xff);

	return 1;
}

static void 
std_set_iv(__ops_crypt_t *cryp, const uint8_t *iv)
{
	(void) memcpy(cryp->iv, iv, cryp->blocksize);
	cryp->num = 0;
}

static void 
std_set_key(__ops_crypt_t *cryp, const uint8_t *key)
{
	(void) memcpy(cryp->key, key, cryp->keysize);
}

static void 
std_resync(__ops_crypt_t *decrypt)
{
	if ((size_t) decrypt->num == decrypt->blocksize) {
		return;
	}
	memmove(decrypt->civ + decrypt->blocksize - decrypt->num, decrypt->civ,
		(unsigned)decrypt->num);
	(void) memcpy(decrypt->civ, decrypt->siv + decrypt->num,
	       decrypt->blocksize - decrypt->num);
	decrypt->num = 0;
}

static void 
std_finish(__ops_crypt_t *cryp)
{
	if (cryp->encrypt_key) {
		free(cryp->encrypt_key);
		cryp->encrypt_key = NULL;
	}
	if (cryp->decrypt_key) {
		free(cryp->decrypt_key);
		cryp->decrypt_key = NULL;
	}
}

static void 
cast5_init(__ops_crypt_t *cryp)
{
	if (cryp->encrypt_key) {
		(void) free(cryp->encrypt_key);
	}
	cryp->encrypt_key = __ops_new(sizeof(CAST_KEY));
	CAST_set_key(cryp->encrypt_key, (int)cryp->keysize, cryp->key);
	cryp->decrypt_key = __ops_new(sizeof(CAST_KEY));
	CAST_set_key(cryp->decrypt_key, (int)cryp->keysize, cryp->key);
}

static void 
cast5_block_encrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	CAST_ecb_encrypt(in, out, cryp->encrypt_key, CAST_ENCRYPT);
}

static void 
cast5_block_decrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	CAST_ecb_encrypt(in, out, cryp->encrypt_key, CAST_DECRYPT);
}

static void 
cast5_cfb_encrypt(__ops_crypt_t *cryp, void *out, const void *in, size_t count)
{
	CAST_cfb64_encrypt(in, out, (long)count,
			   cryp->encrypt_key, cryp->iv, &cryp->num,
			   CAST_ENCRYPT);
}

static void 
cast5_cfb_decrypt(__ops_crypt_t *cryp, void *out, const void *in, size_t count)
{
	CAST_cfb64_encrypt(in, out, (long)count,
			   cryp->encrypt_key, cryp->iv, &cryp->num,
			   CAST_DECRYPT);
}

#define TRAILER		"","","","",0,NULL,NULL

static __ops_crypt_t cast5 =
{
	OPS_SA_CAST5,
	CAST_BLOCK,
	CAST_KEY_LENGTH,
	std_set_iv,
	std_set_key,
	cast5_init,
	std_resync,
	cast5_block_encrypt,
	cast5_block_decrypt,
	cast5_cfb_encrypt,
	cast5_cfb_decrypt,
	std_finish,
	TRAILER
};

#ifndef OPENSSL_NO_IDEA
static void 
idea_init(__ops_crypt_t *cryp)
{
	if (cryp->keysize != IDEA_KEY_LENGTH) {
		loggit("idea_init: keysize wrong");
		return;
	}

	if (cryp->encrypt_key) {
		(void) free(cryp->encrypt_key);
	}
	cryp->encrypt_key = __ops_new(sizeof(IDEA_KEY_SCHEDULE));

	/* note that we don't invert the key when decrypting for CFB mode */
	idea_set_encrypt_key(cryp->key, cryp->encrypt_key);

	if (cryp->decrypt_key) {
		(void) free(cryp->decrypt_key);
	}
	cryp->decrypt_key = __ops_new(sizeof(IDEA_KEY_SCHEDULE));

	idea_set_decrypt_key(cryp->encrypt_key, cryp->decrypt_key);
}

static void 
idea_block_encrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	idea_ecb_encrypt(in, out, cryp->encrypt_key);
}

static void 
idea_block_decrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	idea_ecb_encrypt(in, out, cryp->decrypt_key);
}

static void 
idea_cfb_encrypt(__ops_crypt_t *cryp, void *out, const void *in, size_t count)
{
	idea_cfb64_encrypt(in, out, (long)count,
			   cryp->encrypt_key, cryp->iv, &cryp->num,
			   CAST_ENCRYPT);
}

static void 
idea_cfb_decrypt(__ops_crypt_t *cryp, void *out, const void *in, size_t count)
{
	idea_cfb64_encrypt(in, out, (long)count,
			   cryp->decrypt_key, cryp->iv, &cryp->num,
			   CAST_DECRYPT);
}

static const __ops_crypt_t idea =
{
	OPS_SA_IDEA,
	IDEA_BLOCK,
	IDEA_KEY_LENGTH,
	std_set_iv,
	std_set_key,
	idea_init,
	std_resync,
	idea_block_encrypt,
	idea_block_decrypt,
	idea_cfb_encrypt,
	idea_cfb_decrypt,
	std_finish,
	TRAILER
};
#endif				/* OPENSSL_NO_IDEA */

/* AES with 128-bit key (AES) */

#define KEYBITS_AES128 128

static void 
aes128_init(__ops_crypt_t *cryp)
{
	if (cryp->encrypt_key) {
		(void) free(cryp->encrypt_key);
	}
	cryp->encrypt_key = __ops_new(sizeof(AES_KEY));
	if (AES_set_encrypt_key(cryp->key, KEYBITS_AES128,
			cryp->encrypt_key)) {
		loggit("aes128_init: Error setting encrypt_key");
	}

	if (cryp->decrypt_key) {
		(void) free(cryp->decrypt_key);
	}
	cryp->decrypt_key = __ops_new(sizeof(AES_KEY));
	if (AES_set_decrypt_key(cryp->key, KEYBITS_AES128,
				cryp->decrypt_key)) {
		loggit("aes128_init: Error setting decrypt_key");
	}
}

static void 
aes_block_encrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	AES_encrypt(in, out, cryp->encrypt_key);
}

static void 
aes_block_decrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	AES_decrypt(in, out, cryp->decrypt_key);
}

static void 
aes_cfb_encrypt(__ops_crypt_t *cryp, void *out, const void *in, size_t count)
{
	AES_cfb128_encrypt(in, out, (unsigned long)count,
			   cryp->encrypt_key, cryp->iv, &cryp->num,
			   AES_ENCRYPT);
}

static void 
aes_cfb_decrypt(__ops_crypt_t *cryp, void *out, const void *in, size_t count)
{
	AES_cfb128_encrypt(in, out, (unsigned long)count,
			   cryp->encrypt_key, cryp->iv, &cryp->num,
			   AES_DECRYPT);
}

static const __ops_crypt_t aes128 =
{
	OPS_SA_AES_128,
	AES_BLOCK_SIZE,
	KEYBITS_AES128 / 8,
	std_set_iv,
	std_set_key,
	aes128_init,
	std_resync,
	aes_block_encrypt,
	aes_block_decrypt,
	aes_cfb_encrypt,
	aes_cfb_decrypt,
	std_finish,
	TRAILER
};

/* AES with 256-bit key */

#define KEYBITS_AES256 256

static void 
aes256_init(__ops_crypt_t *cryp)
{
	if (cryp->encrypt_key) {
		(void) free(cryp->encrypt_key);
	}
	cryp->encrypt_key = __ops_new(sizeof(AES_KEY));
	if (AES_set_encrypt_key(cryp->key, KEYBITS_AES256,
			cryp->encrypt_key)) {
		loggit("aes256_init: Error setting encrypt_key");
	}

	if (cryp->decrypt_key)
		free(cryp->decrypt_key);
	cryp->decrypt_key = __ops_new(sizeof(AES_KEY));
	if (AES_set_decrypt_key(cryp->key, KEYBITS_AES256,
			cryp->decrypt_key)) {
		loggit("aes256_init: Error setting decrypt_key");
	}
}

static const __ops_crypt_t aes256 =
{
	OPS_SA_AES_256,
	AES_BLOCK_SIZE,
	KEYBITS_AES256 / 8,
	std_set_iv,
	std_set_key,
	aes256_init,
	std_resync,
	aes_block_encrypt,
	aes_block_decrypt,
	aes_cfb_encrypt,
	aes_cfb_decrypt,
	std_finish,
	TRAILER
};

/* Triple DES */

static void 
tripledes_init(__ops_crypt_t *cryp)
{
	DES_key_schedule *keys;
	int             n;

	if (cryp->encrypt_key) {
		(void) free(cryp->encrypt_key);
	}
	keys = cryp->encrypt_key = __ops_new(3 * sizeof(DES_key_schedule));

	for (n = 0; n < 3; ++n) {
		DES_set_key((DES_cblock *)(void *)(cryp->key + n * 8),
			&keys[n]);
	}
}

static void 
tripledes_block_encrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	DES_key_schedule *keys = cryp->encrypt_key;

	DES_ecb3_encrypt(__UNCONST(in), out, &keys[0], &keys[1], &keys[2],
			DES_ENCRYPT);
}

static void 
tripledes_block_decrypt(__ops_crypt_t *cryp, void *out, const void *in)
{
	DES_key_schedule *keys = cryp->encrypt_key;

	DES_ecb3_encrypt(__UNCONST(in), out, &keys[0], &keys[1], &keys[2],
			DES_DECRYPT);
}

static void 
tripledes_cfb_encrypt(__ops_crypt_t *cryp, void *out, const void *in,
			size_t count)
{
	DES_key_schedule *keys = cryp->encrypt_key;

	DES_ede3_cfb64_encrypt(in, out, (long)count,
		&keys[0], &keys[1], &keys[2], (DES_cblock *)(void *)cryp->iv,
		&cryp->num, DES_ENCRYPT);
}

static void 
tripledes_cfb_decrypt(__ops_crypt_t *cryp, void *out, const void *in,
			size_t count)
{
	DES_key_schedule *keys = cryp->encrypt_key;

	DES_ede3_cfb64_encrypt(in, out, (long)count,
		&keys[0], &keys[1], &keys[2], (DES_cblock *)(void *)cryp->iv,
		&cryp->num, DES_DECRYPT);
}

static const __ops_crypt_t tripledes =
{
	OPS_SA_TRIPLEDES,
	8,
	24,
	std_set_iv,
	std_set_key,
	tripledes_init,
	std_resync,
	tripledes_block_encrypt,
	tripledes_block_decrypt,
	tripledes_cfb_encrypt,
	tripledes_cfb_decrypt,
	std_finish,
	TRAILER
};

static const __ops_crypt_t *
get_proto(__ops_symm_alg_t alg)
{
	switch (alg) {
	case OPS_SA_CAST5:
		return &cast5;

#ifndef OPENSSL_NO_IDEA
	case OPS_SA_IDEA:
		return &idea;
#endif				/* OPENSSL_NO_IDEA */

	case OPS_SA_AES_128:
		return &aes128;

	case OPS_SA_AES_256:
		return &aes256;

	case OPS_SA_TRIPLEDES:
		return &tripledes;

	default:
		loggit("Unknown algorithm: %d (%s)",
			alg,
			findtype((int) alg, symm_alg_map));
	}

	return NULL;
}

static unsigned 
__ops_key_size(__ops_symm_alg_t alg)
{
	const __ops_crypt_t *p;

	p = get_proto(alg);
	return (p == NULL) ? 0 : p->keysize;
}

static int 
__ops_crypt_any(__ops_crypt_t *cryp, __ops_symm_alg_t alg)
{
	const __ops_crypt_t *ptr = get_proto(alg);

	if (ptr) {
		*cryp = *ptr;
		return 1;
	} else {
		(void) memset(cryp, 0x0, sizeof(*cryp));
		return 0;
	}
}

static unsigned 
__ops_block_size(__ops_symm_alg_t alg)
{
	const __ops_crypt_t *p;

	p = get_proto(alg);
	return (p == NULL) ? 0 : p->blocksize;
}

static void 
__ops_decrypt_init(__ops_crypt_t *decrypt)
{
	decrypt->base_init(decrypt);
	decrypt->block_encrypt(decrypt, decrypt->siv, decrypt->iv);
	(void) memcpy(decrypt->civ, decrypt->siv, decrypt->blocksize);
	decrypt->num = 0;
}

static void 
__ops_encrypt_init(__ops_crypt_t *encryp)
{
	/* \todo should there be a separate __ops_encrypt_init? */
	__ops_decrypt_init(encryp);
}


static size_t 
__ops_decrypt_se_ip(__ops_crypt_t *cryp, void *out, const void *in,
		  size_t count)
{
	if (!__ops_is_sa_supported(cryp->alg)) {
		return 0;
	}
	cryp->cfb_decrypt(cryp, out, in, count);
	/* \todo check this number was in fact decrypted */
	return count;
}


/**************************************************************************/


#define DECOMPRESS_BUFFER	1024

typedef struct {
	__ops_ztype_t type;
	__ops_region_t   *region;
	uint8_t   in[DECOMPRESS_BUFFER];
	uint8_t   out[DECOMPRESS_BUFFER];
	z_stream        zstream;/* ZIP and ZLIB */
	size_t          offset;
	int             inflate_ret;
} z_decompress_t;

typedef struct {
	__ops_ztype_t type;
	__ops_region_t   *region;
	char            in[DECOMPRESS_BUFFER];
	char            out[DECOMPRESS_BUFFER];
	bz_stream       bzstream;	/* BZIP2 */
	size_t          offset;
	int             inflate_ret;
} bz_decompress_t;

typedef struct {
	z_stream        stream;
	uint8_t  *src;
	uint8_t  *dst;
} compress_t;

static unsigned __ops_stacked_limited_read(uint8_t *, unsigned,
			 __ops_region_t *,
			 __ops_error_t **,
			 __ops_reader_t *,
			 __ops_cbdata_t *);

/*
 * \todo remove code duplication between this and
 * bzip2_compressed_data_reader
 */
static int 
zlib_compressed_data_reader(void *dest, size_t length,
			    __ops_error_t **errors,
			    __ops_reader_t *readinfo,
			    __ops_cbdata_t *cbdata)
{
	z_decompress_t *z = readinfo->arg;
	size_t           len;
	size_t		 cc;
	char		*cdest = dest;

	if (z->type != OPS_C_ZIP && z->type != OPS_C_ZLIB) {
		loggit("zlib_compressed_data_reader: weird type %d",
			z->type);
		return 0;
	}

	if (z->inflate_ret == Z_STREAM_END &&
	    z->zstream.next_out == &z->out[z->offset]) {
		return 0;
	}

	if (z->region->readc == z->region->length) {
		if (z->inflate_ret != Z_STREAM_END) {
			OPS_ERROR(cbdata->errors, OPS_E_P_DECOMPRESSION_ERROR,
			"Compressed data didn't end when region ended.");
		}
	}
	for (cc = 0 ; cc < length ; cc += len) {
		if (&z->out[z->offset] == z->zstream.next_out) {
			int             ret;

			z->zstream.next_out = z->out;
			z->zstream.avail_out = sizeof(z->out);
			z->offset = 0;
			if (z->zstream.avail_in == 0) {
				unsigned        n = z->region->length;

				if (!z->region->indeterm) {
					n -= z->region->readc;
					if (n > sizeof(z->in)) {
						n = sizeof(z->in);
					}
				} else {
					n = sizeof(z->in);
				}
				if (!__ops_stacked_limited_read(z->in, n,
						z->region,
						errors, readinfo, cbdata)) {
					return -1;
				}

				z->zstream.next_in = z->in;
				z->zstream.avail_in = (z->region->indeterm) ?
					z->region->lastread : n;
			}
			ret = inflate(&z->zstream, Z_SYNC_FLUSH);
			if (ret == Z_STREAM_END) {
				if (!z->region->indeterm &&
				    z->region->readc != z->region->length) {
					OPS_ERROR(cbdata->errors,
						OPS_E_P_DECOMPRESSION_ERROR,
						"Compressed stream ended before packet end.");
				}
			} else if (ret != Z_OK) {
				loggit("ret=%d", ret);
				OPS_ERROR(cbdata->errors,
				OPS_E_P_DECOMPRESSION_ERROR, z->zstream.msg);
			}
			z->inflate_ret = ret;
		}
		if (z->zstream.next_out <= &z->out[z->offset]) {
			loggit("Out of memory in buffer");
			return 0;
		}
		len = z->zstream.next_out - &z->out[z->offset];
		if (len > length) {
			len = length;
		}
		(void) memcpy(&cdest[cc], &z->out[z->offset], len);
		z->offset += len;
	}

	return length;
}

/* \todo remove code duplication between this and zlib_compressed_data_reader */
static int 
bzip2_compressed_data_reader(void *dest, size_t length,
			     __ops_error_t **errors,
			     __ops_reader_t *readinfo,
			     __ops_cbdata_t *cbdata)
{
	bz_decompress_t *bz = readinfo->arg;
	size_t		len;
	size_t		 cc;
	char		*cdest = dest;

	if (bz->type != OPS_C_BZIP2) {
		loggit("Weird type %d", bz->type);
		return 0;
	}

	if (bz->inflate_ret == BZ_STREAM_END &&
	    bz->bzstream.next_out == &bz->out[bz->offset]) {
		return 0;
	}
	if (bz->region->readc == bz->region->length) {
		if (bz->inflate_ret != BZ_STREAM_END) {
			OPS_ERROR(cbdata->errors, OPS_E_P_DECOMPRESSION_ERROR,
			"Compressed data didn't end when region ended.");
		}
	}
	for (cc = 0 ; cc < length ; cc += len) {
		if (&bz->out[bz->offset] == bz->bzstream.next_out) {
			int             ret;

			bz->bzstream.next_out = (char *) bz->out;
			bz->bzstream.avail_out = sizeof(bz->out);
			bz->offset = 0;
			if (bz->bzstream.avail_in == 0) {
				unsigned        n = bz->region->length;

				if (!bz->region->indeterm) {
					n -= bz->region->readc;
					if (n > sizeof(bz->in))
						n = sizeof(bz->in);
				} else
					n = sizeof(bz->in);

				if (!__ops_stacked_limited_read(
						(uint8_t *) bz->in,
						n, bz->region,
						errors, readinfo, cbdata))
					return -1;

				bz->bzstream.next_in = bz->in;
				bz->bzstream.avail_in =
					(bz->region->indeterm) ?
					 bz->region->lastread : n;
			}
			ret = BZ2_bzDecompress(&bz->bzstream);
			if (ret == BZ_STREAM_END) {
				if (!bz->region->indeterm &&
				    bz->region->readc != bz->region->length)
					OPS_ERROR(cbdata->errors,
						OPS_E_P_DECOMPRESSION_ERROR,
						"Compressed stream ended before packet end.");
			} else if (ret != BZ_OK) {
				OPS_ERROR_1(cbdata->errors,
					OPS_E_P_DECOMPRESSION_ERROR,
					"Invalid return %d from BZ2_bzDecompress", ret);
			}
			bz->inflate_ret = ret;
		}
		if (bz->bzstream.next_out <= &bz->out[bz->offset]) {
			loggit("Out of bz memroy");
			return 0;
		}
		len = bz->bzstream.next_out - &bz->out[bz->offset];
		if (len > length) {
			len = length;
		}
		(void) memcpy(&cdest[cc], &bz->out[bz->offset], len);
		bz->offset += len;
	}

	return length;
}

/**
 * \ingroup Core_Compress
 *
 * \param *region 	Pointer to a region
 * \param *parse_info 	How to parse
 * \param type Which compression type to expect
*/

static int 
__ops_decompress(__ops_region_t *region, __ops_stream_t *parse_info,
	       __ops_ztype_t ztype)
{
	z_decompress_t z;
	bz_decompress_t bz;
	const int	printerrors = 1;
	int             ret;

	switch (ztype) {
	case OPS_C_ZIP:
	case OPS_C_ZLIB:
		(void) memset(&z, 0x0, sizeof(z));

		z.region = region;
		z.offset = 0;
		z.type = ztype;

		z.zstream.next_in = Z_NULL;
		z.zstream.avail_in = 0;
		z.zstream.next_out = z.out;
		z.zstream.zalloc = Z_NULL;
		z.zstream.zfree = Z_NULL;
		z.zstream.opaque = Z_NULL;

		break;

	case OPS_C_BZIP2:
		(void) memset(&bz, 0x0, sizeof(bz));

		bz.region = region;
		bz.offset = 0;
		bz.type = ztype;

		bz.bzstream.next_in = NULL;
		bz.bzstream.avail_in = 0;
		bz.bzstream.next_out = bz.out;
		bz.bzstream.bzalloc = NULL;
		bz.bzstream.bzfree = NULL;
		bz.bzstream.opaque = NULL;

		break;

	default:
		OPS_ERROR_1(&parse_info->errors,
			OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG,
			"Compression algorithm %d is not yet supported", ztype);
		return 0;
	}

	switch (ztype) {
	case OPS_C_ZIP:
		ret = inflateInit2(&z.zstream, -15);
		break;

	case OPS_C_ZLIB:
		ret = inflateInit(&z.zstream);
		break;

	case OPS_C_BZIP2:
		ret = BZ2_bzDecompressInit(&bz.bzstream, 1, 0);
		break;

	default:
		OPS_ERROR_1(&parse_info->errors,
			OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG,
			"Compression algorithm %d is not yet supported", ztype);
		return 0;
	}

	switch (ztype) {
	case OPS_C_ZIP:
	case OPS_C_ZLIB:
		if (ret != Z_OK) {
			OPS_ERROR_1(&parse_info->errors,
				OPS_E_P_DECOMPRESSION_ERROR,
"Cannot initialise ZIP or ZLIB stream for decompression: error=%d", ret);
			return 0;
		}
		__ops_reader_push(parse_info, zlib_compressed_data_reader,
					NULL, &z);
		break;

	case OPS_C_BZIP2:
		if (ret != BZ_OK) {
			OPS_ERROR_1(&parse_info->errors,
				OPS_E_P_DECOMPRESSION_ERROR,
"Cannot initialise BZIP2 stream for decompression: error=%d", ret);
			return 0;
		}
		__ops_reader_push(parse_info, bzip2_compressed_data_reader,
					NULL, &bz);
		break;

	default:
		OPS_ERROR_1(&parse_info->errors,
			OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG,
			"Compression algorithm %d is not yet supported", ztype);
		return 0;
	}

	ret = __ops_parse(parse_info, !printerrors);

	__ops_reader_pop(parse_info);

	return ret;
}

/**************************************************************************/

#define CALLBACK(cbdata, t, pkt)	do {				\
	(pkt)->tag = (t);						\
	if (__ops_callback(cbdata, pkt) == OPS_RELEASE_MEMORY) {	\
		__ops_pkt_free(pkt);					\
	}								\
} while(/* CONSTCOND */0)

#define ERRP(cbdata, cont, err)	do {					\
	cont.u.error.error = err;					\
	CALLBACK(cbdata, OPS_PARSER_ERROR, &cont);			\
	return 0;							\
	/*NOTREACHED*/							\
} while(/*CONSTCOND*/0)

static unsigned __ops_limited_read(uint8_t *,
			size_t,
			__ops_region_t *,
			__ops_error_t **,
			__ops_reader_t *,
			__ops_cbdata_t *);

/**
 * limread_data reads the specified amount of the subregion's data
 * into a data_t structure
 *
 * \param data	Empty structure which will be filled with data
 * \param len	Number of octets to read
 * \param subregion
 * \param stream	How to parse
 *
 * \return 1 on success, 0 on failure
 */
static int 
limread_data(__ops_data_t *data, unsigned int len,
		  __ops_region_t *subregion, __ops_stream_t *stream)
{
	data->len = len;

	if (subregion->length - subregion->readc < len) {
		loggit("limread_data: bad length");
		return 0;
	}

	data->contents = __ops_new(data->len);
	if (!data->contents) {
		return 0;
	}

	return __ops_limited_read(data->contents, data->len, subregion,
			&stream->errors, &stream->readinfo, &stream->cbdata);
}

/**
 * read_data reads the remainder of the subregion's data
 * into a data_t structure
 *
 * \param data
 * \param subregion
 * \param stream
 *
 * \return 1 on success, 0 on failure
 */
static int 
read_data(__ops_data_t *data, __ops_region_t *region, __ops_stream_t *stream)
{
	int	cc;

	cc = region->length - region->readc;
	return (cc >= 0) ? limread_data(data, (unsigned)cc, region, stream) : 0;
}

/**
 * Reads the remainder of the subregion as a string.
 * It is the user's responsibility to free the memory allocated here.
 */

static int 
read_unsig_str(uint8_t **str, __ops_region_t *subregion,
		     __ops_stream_t *stream)
{
	size_t	len = 0;

	len = subregion->length - subregion->readc;
	if ((*str = __ops_new(len + 1)) == NULL) {
		return 0;
	}
	if (len &&
	    !__ops_limited_read(*str, len, subregion, &stream->errors,
				     &stream->readinfo, &stream->cbdata)) {
		return 0;
	}
	/* ! ensure the string is NULL-terminated */
	(*str)[len] = '\0';
	return 1;
}

static int 
read_string(char **str, __ops_region_t *subregion, __ops_stream_t *stream)
{
	return read_unsig_str((uint8_t **) str, subregion, stream);
}

static void 
__ops_init_subregion(__ops_region_t *subregion, __ops_region_t *region)
{
	(void) memset(subregion, 0x0, sizeof(*subregion));
	subregion->parent = region;
}

/*
 * Read a full size_t's worth. If the return is < than length, then
 * *lastread tells you why - < 0 for an error, == 0 for EOF
 */

static size_t 
full_read(uint8_t *dest,
		size_t length,
		int *lastread,
		__ops_error_t **errors,
		__ops_reader_t *readinfo,
		__ops_cbdata_t *cbdata)
{
	size_t          t;
	int             r = 0;	/* preset in case some loon calls with length
				 * == 0 */

	for (t = 0; t < length;) {
		r = sub_base_read(dest + t, length - t, errors, readinfo,
				cbdata);
		if (r <= 0) {
			*lastread = r;
			return t;
		}
		t += r;
	}

	*lastread = r;

	return t;
}



/** Read a scalar value of selected length from reader.
 *
 * Read an unsigned scalar value from reader in Big Endian representation.
 *
 * This function does not know or care about packet boundaries. It
 * also assumes that an EOF is an error.
 *
 * \param *result	The scalar value is stored here
 * \param *reader	Our reader
 * \param length	How many bytes to read
 * \return		1 on success, 0 on failure
 */
static unsigned 
read_scalar(unsigned *result, unsigned length,
	     __ops_stream_t *stream)
{
	unsigned        t = 0;

	if (length > sizeof(*result)) {
		loggit("read_scalar: bad length");
		return 0;
	}

	while (length--) {
		uint8_t   c;
		int             r;

		r = sub_base_read(&c, 1, &stream->errors, &stream->readinfo,
				     &stream->cbdata);
		if (r != 1)
			return 0;
		t = (t << 8) + c;
	}

	*result = t;
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Read bytes from a region within the packet.
 *
 * Read length bytes into the buffer pointed to by *dest.
 * Make sure we do not read over the packet boundary.
 * Updates the Packet Tag's __ops_ptag_t::readc.
 *
 * If length would make us read over the packet boundary, or if
 * reading fails, we call the callback with an error.
 *
 * Note that if the region is indeterminate, this can return a short
 * read - check region->lastread for the length. EOF is indicated by
 * a success return and region->lastread == 0 in this case (for a
 * region of known length, EOF is an error).
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param dest		The destination buffer
 * \param length	How many bytes to read
 * \param region	Pointer to packet region
 * \param errors    Error stack
 * \param readinfo		Reader info
 * \param cbdata	Callback info
 * \return		1 on success, 0 on error
 */
static unsigned 
__ops_limited_read(uint8_t *dest,
			size_t length,
			__ops_region_t *region,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_cbdata_t *cbdata)
{
	size_t	r;
	int	lr;

	if (!region->indeterm &&
	    region->readc + length > region->length) {
		OPS_ERROR(errors, OPS_E_P_NOT_ENOUGH_DATA, "Not enough data");
		return 0;
	}
	r = full_read(dest, length, &lr, errors, readinfo, cbdata);
	if (lr < 0) {
		OPS_ERROR(errors, OPS_E_R_READ_FAILED, "Read failed");
		return 0;
	}
	if (!region->indeterm && r != length) {
		OPS_ERROR(errors, OPS_E_R_READ_FAILED, "Read failed");
		return 0;
	}
	region->lastread = r;
	do {
		region->readc += r;
		if (region->parent && region->length > region->parent->length) {
			loggit("ops_limited_read: bad length");
			return 0;
		}
	} while ((region = region->parent) != NULL);
	return 1;
}

/**
   \ingroup Core_ReadPackets
   \brief Call __ops_limited_read on next in stack
*/
static unsigned 
__ops_stacked_limited_read(uint8_t *dest, unsigned length,
			 __ops_region_t *region,
			 __ops_error_t **errors,
			 __ops_reader_t *readinfo,
			 __ops_cbdata_t *cbdata)
{
	return __ops_limited_read(dest, length, region, errors,
				readinfo->next, cbdata);
}

static unsigned 
limread(uint8_t *dest, unsigned length,
	     __ops_region_t *region, __ops_stream_t *stream)
{
	return __ops_limited_read(dest, length, region, &stream->errors,
				&stream->readinfo, &stream->cbdata);
}

static unsigned 
exact_limread(uint8_t *dest, unsigned len,
		   __ops_region_t *region,
		   __ops_stream_t *stream)
{
	unsigned   ret;

	stream->exact_read = 1;
	ret = limread(dest, len, region, stream);
	stream->exact_read = 0;
	return ret;
}

/** Skip over length bytes of this packet.
 *
 * Calls limread() to skip over some data.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param length	How many bytes to skip
 * \param *region	Pointer to packet region
 * \param *stream	How to parse
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limread()).
 */
static int 
limskip(unsigned length, __ops_region_t *region, __ops_stream_t *stream)
{
	uint8_t   buf[NETPGP_BUFSIZ];

	while (length > 0) {
		unsigned	n = length % NETPGP_BUFSIZ;

		if (!limread(buf, n, region, stream)) {
			return 0;
		}
		length -= n;
	}
	return 1;
}

/** Read a scalar.
 *
 * Read a big-endian scalar of length bytes, respecting packet
 * boundaries (by calling limread() to read the raw data).
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The scalar value is stored here
 * \param length	How many bytes make up this scalar (at most 4)
 * \param *region	Pointer to current packet region
 * \param *stream	How to parse
 * \param *cb		The callback
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limread()).
 *
 * \see RFC4880 3.1
 */
static int 
limread_scalar(unsigned *dest,
			unsigned len,
			__ops_region_t *region,
			__ops_stream_t *stream)
{
	uint8_t   c[4] = "";
	unsigned        t;
	unsigned        n;

	if (len > 4) {
		loggit("limread_scalar: bad length");
		return 0;
	}
	/*LINTED*/
	if (/*CONSTCOND*/sizeof(*dest) < 4) {
		loggit("limread_scalar: bad dest");
		return 0;
	}
	if (!limread(c, len, region, stream)) {
		return 0;
	}
	for (t = 0, n = 0; n < len; ++n) {
		t = (t << 8) + c[n];
	}
	*dest = t;
	return 1;
}

/** Read a scalar.
 *
 * Read a big-endian scalar of length bytes, respecting packet
 * boundaries (by calling limread() to read the raw data).
 *
 * The value read is stored in a size_t, which is a different size
 * from an unsigned on some platforms.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The scalar value is stored here
 * \param length	How many bytes make up this scalar (at most 4)
 * \param *region	Pointer to current packet region
 * \param *stream	How to parse
 * \param *cb		The callback
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limread()).
 *
 * \see RFC4880 3.1
 */
static int 
limread_size_t(size_t *dest,
		unsigned length,
		__ops_region_t *region,
		__ops_stream_t *stream)
{
	unsigned        tmp;

	/*
	 * Note that because the scalar is at most 4 bytes, we don't care if
	 * size_t is bigger than usigned
	 */
	if (!limread_scalar(&tmp, length, region, stream)) {
		return 0;
	}
	*dest = tmp;
	return 1;
}

/** Read a timestamp.
 *
 * Timestamps in OpenPGP are unix time, i.e. seconds since The Epoch
 * (1.1.1970).  They are stored in an unsigned scalar of 4 bytes.
 *
 * This function reads the timestamp using limread_scalar().
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The timestamp is stored here
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		see limread_scalar()
 *
 * \see RFC4880 3.5
 */
static int 
limited_read_time(time_t *dest, __ops_region_t *region, __ops_stream_t *stream)
{
	uint8_t   c;
	time_t          mytime = 0;
	int             i = 0;

	/*
         * Cannot assume that time_t is 4 octets long -
	 * SunOS 5.10 and NetBSD both have 64-bit time_ts.
         */
	if (/* CONSTCOND */sizeof(time_t) == 4) {
		return limread_scalar((unsigned *)(void *)dest, 4, region,
					stream);
	}
	for (i = 0; i < 4; i++) {
		if (!limread(&c, 1, region, stream)) {
			return 0;
		}
		mytime = (mytime << 8) + c;
	}
	*dest = mytime;
	return 1;
}

/**
 * \ingroup Core_MPI
 * Read a multiprecision integer.
 *
 * Large numbers (multiprecision integers, MPI) are stored in OpenPGP in two parts.  First there is a 2 byte scalar
 * indicating the length of the following MPI in Bits.  Then follow the bits that make up the actual number, most
 * significant bits first (Big Endian).  The most significant bit in the MPI is supposed to be 1 (unless the MPI is
 * encrypted - then it may be different as the bit count refers to the plain text but the bits are encrypted).
 *
 * Unused bits (i.e. those filling up the most significant byte from the left to the first bits that counts) are
 * supposed to be cleared - I guess. XXX - does anything actually say so?
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param **pgn		return the integer there - the BIGNUM is created by BN_bin2bn() and probably needs to be freed
 * 				by the caller XXX right ben?
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error (by limread_scalar() or limread() or if the MPI is not properly formed (XXX
 * 				 see comment below - the callback is called with a OPS_PARSER_ERROR in case of an error)
 *
 * \see RFC4880 3.2
 */
static int 
limread_mpi(BIGNUM **pbn, __ops_region_t *region, __ops_stream_t *stream)
{
	uint8_t   buf[NETPGP_BUFSIZ] = "";
					/* an MPI has a 2 byte length part.
					 * Length is given in bits, so the
					 * largest we should ever need for
					 * the buffer is NETPGP_BUFSIZ bytes. */
	unsigned        length;
	unsigned        nonzero;
	unsigned	ret;

	stream->reading_mpi_len = 1;
	ret = limread_scalar(&length, 2, region, stream);

	stream->reading_mpi_len = 0;
	if (!ret) {
		return 0;
	}

	nonzero = length & 7;	/* there should be this many zero bits in the
				 * MS byte */
	if (!nonzero) {
		nonzero = 8;
	}
	length = (length + 7) / 8;

	if (length == 0) {
		/* if we try to read a length of 0, then fail */
		return 0;
	}
	if (length > NETPGP_BUFSIZ) {
		loggit("limread_mpi: bad length");
		return 0;
	}
	if (!limread(buf, length, region, stream)) {
		return 0;
	}
	if (((unsigned)buf[0] >> nonzero) != 0 ||
	    !((unsigned)buf[0] & (1U << (nonzero - 1U)))) {
		OPS_ERROR(&stream->errors, OPS_E_P_MPI_FORMAT_ERROR,
				"MPI Format error");
		/* XXX: Ben, one part of
		 * this constraint does
		 * not apply to
		 * encrypted MPIs the
		 * draft says. -- peter */
		return 0;
	}
	*pbn = BN_bin2bn(buf, (int)length, NULL);
	return 1;
}

/** Read some data with a New-Format length from reader.
 *
 * \sa Internet-Draft RFC4880.txt Section 4.2.2
 *
 * \param *length	Where the decoded length will be put
 * \param *stream	How to parse
 * \return		1 if OK, else 0
 *
 */

static unsigned 
read_new_length(unsigned *length, __ops_stream_t *stream)
{
	uint8_t   c;

	if (sub_base_read(&c, 1, &stream->errors, &stream->readinfo,
				     &stream->cbdata) != 1) {
		return 0;
	}
	if (c < 192) {
		/* 1. One-octet packet */
		*length = c;
		return 1;
	}
	if (c >= 192 && c <= 223) {
		/* 2. Two-octet packet */
		unsigned        t = (c - 192) << 8;

		if (sub_base_read(&c, 1, &stream->errors, &stream->readinfo,
				     &stream->cbdata) != 1) {
			return 0;
		}
		*length = t + c + 192;
		return 1;
	}
	if (c == 255) {
		/* 3. Five-Octet packet */
		return read_scalar(length, 4, stream);
	}
	if (c >= 224 && c < 255) {
		/* 4. Partial Body Length */
		/* XXX - agc - gpg multi-recipient encryption uses this */
		OPS_ERROR(&stream->errors, OPS_E_UNIMPLEMENTED,
		"New format Partial Body Length fields not yet implemented");
		return 0;
	}
	return 0;
}

/** Read the length information for a new format Packet Tag.
 *
 * New style Packet Tags encode the length in one to five octets.  This function reads the right amount of bytes and
 * decodes it to the proper length information.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *length	return the length here
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error (by limread_scalar() or limread() or if the MPI is not properly formed (XXX
 * 				 see comment below)
 *
 * \see RFC4880 4.2.2
 * \see __ops_ptag_t
 */
static int 
limited_read_new_length(unsigned *length, __ops_region_t *region,
			__ops_stream_t *stream)
{
	uint8_t   c = 0x0;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	if (c < 192) {
		*length = c;
		return 1;
	}
	if (c < 255) {
		unsigned        t = (c - 192) << 8;

		if (!limread(&c, 1, region, stream)) {
			return 0;
		}
		*length = t + c + 192;
		return 1;
	}
	return limread_scalar(length, 4, region, stream);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
data_free(__ops_data_t *data)
{
	(void) free(data->contents);
	data->contents = NULL;
	data->len = 0;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
string_free(char **str)
{
	(void) free(*str);
	*str = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free packet memory, set pointer to NULL */
static void 
__ops_subpacket_free(__ops_subpacket_t *packet)
{
	(void) free(packet->raw);
	packet->raw = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
__ops_headers_free(__ops_headers_t *headers)
{
	unsigned        n;

	for (n = 0; n < headers->headerc; ++n) {
		(void) free(headers->headers[n].key);
		(void) free(headers->headers[n].value);
	}
	(void) free(headers->headers);
	headers->headers = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
cleartext_trailer_free(__ops_cleartext_trailer_t *trailer)
{
	(void) free(trailer->hash);
	trailer->hash = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
__ops_cmd_get_passphrase_free(__ops_seckey_passphrase_t *seckey)
{
	unsigned	len;

	if (seckey->passphrase && *seckey->passphrase) {
		len = strlen(*seckey->passphrase);
		__ops_forget(*seckey->passphrase, len);
		(void) free(*seckey->passphrase);
		*seckey->passphrase = NULL;
	}
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_userdef_free(__ops_ss_userdef_t *ss_userdef)
{
	data_free(&ss_userdef->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_reserved_free(__ops_ss_unknown_t *ss_unknown)
{
	data_free(&ss_unknown->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this packet type
*/
static void 
trust_free(__ops_trust_t *trust)
{
	data_free(&trust->data);
}

/**
 * \ingroup Core_Create
 * \brief Free the memory used when parsing a private/experimental PKA signature
 * \param unknown_sig
 */
static void 
free_unknown_sig_pka(__ops_unknown_sig_t *unknown_sig)
{
	data_free(&unknown_sig->data);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
free_BN(BIGNUM **pp)
{
	BN_free(*pp);
	*pp = NULL;
}

/**
 * \ingroup Core_Create
 * \brief Free the memory used when parsing a signature
 * \param sig
 */
static void 
sig_free(__ops_sig_t *sig)
{
	switch (sig->info.key_alg) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_SIGN_ONLY:
		free_BN(&sig->info.sig.rsa.sig);
		break;

	case OPS_PKA_DSA:
		free_BN(&sig->info.sig.dsa.r);
		free_BN(&sig->info.sig.dsa.s);
		break;

	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		free_BN(&sig->info.sig.elgamal.r);
		free_BN(&sig->info.sig.elgamal.s);
		break;

	case OPS_PKA_PRIVATE00:
	case OPS_PKA_PRIVATE01:
	case OPS_PKA_PRIVATE02:
	case OPS_PKA_PRIVATE03:
	case OPS_PKA_PRIVATE04:
	case OPS_PKA_PRIVATE05:
	case OPS_PKA_PRIVATE06:
	case OPS_PKA_PRIVATE07:
	case OPS_PKA_PRIVATE08:
	case OPS_PKA_PRIVATE09:
	case OPS_PKA_PRIVATE10:
		free_unknown_sig_pka(&sig->info.sig.unknown);
		break;

	default:
		loggit("sig_free: bad sig type");
	}
}

/**
 \ingroup Core_Create
 \brief Free the memory used when parsing this signature sub-packet type
 \param ss_skapref
*/
static void 
ss_skapref_free(__ops_ss_skapref_t *ss_skapref)
{
	data_free(&ss_skapref->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
   \param ss_hashpref
*/
static void 
ss_hashpref_free(__ops_ss_hashpref_t *ss_hashpref)
{
	data_free(&ss_hashpref->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_zpref_free(__ops_ss_zpref_t *ss_zpref)
{
	data_free(&ss_zpref->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_key_flags_free(__ops_ss_key_flags_t *ss_key_flags)
{
	data_free(&ss_key_flags->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_key_server_prefs_free(__ops_ss_key_server_prefs_t *ss_key_server_prefs)
{
	data_free(&ss_key_server_prefs->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_features_free(__ops_ss_features_t *ss_features)
{
	data_free(&ss_features->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_notation_free(__ops_ss_notation_t *ss_notation)
{
	data_free(&ss_notation->name);
	data_free(&ss_notation->value);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this signature sub-packet type */
static void 
ss_regexp_free(__ops_ss_regexp_t *regexp)
{
	string_free(&regexp->regexp);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this signature sub-packet type */
static void 
ss_policy_free(__ops_ss_policy_t *policy)
{
	string_free(&policy->url);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this signature sub-packet type */
static void 
ss_keyserv_free(__ops_ss_keyserv_t *preferred_key_server)
{
	string_free(&preferred_key_server->name);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_revocation_free(__ops_ss_revocation_t *ss_revocation)
{
	string_free(&ss_revocation->reason);
}

static void 
ss_embedded_sig_free(__ops_ss_embedded_sig_t *ss_embedded_sig)
{
	data_free(&ss_embedded_sig->sig);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
__ops_pk_sesskey_free(__ops_pk_sesskey_t *sk)
{
	switch (sk->alg) {
	case OPS_PKA_RSA:
		free_BN(&sk->parameters.rsa.encrypted_m);
		break;

	case OPS_PKA_ELGAMAL:
		free_BN(&sk->parameters.elgamal.g_to_k);
		free_BN(&sk->parameters.elgamal.encrypted_m);
		break;

	default:
		loggit("__ops_pk_sesskey_free: bad alg");
		break;
	}
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing a public key */
static void 
__ops_pubkey_free(__ops_pubkey_t *p)
{
	switch (p->alg) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		free_BN(&p->key.rsa.n);
		free_BN(&p->key.rsa.e);
		break;

	case OPS_PKA_DSA:
		free_BN(&p->key.dsa.p);
		free_BN(&p->key.dsa.q);
		free_BN(&p->key.dsa.g);
		free_BN(&p->key.dsa.y);
		break;

	case OPS_PKA_ELGAMAL:
	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		free_BN(&p->key.elgamal.p);
		free_BN(&p->key.elgamal.g);
		free_BN(&p->key.elgamal.y);
		break;

	case 0:
		/* nothing to free */
		break;

	default:
		loggit("__ops_pubkey_free: bad alg");
	}
}


/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this packet type */
static void 
__ops_userattr_free(__ops_userattr_t *user_att)
{
	data_free(&user_att->data);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this packet type */
static void 
__ops_userid_free(__ops_userid_t *id)
{
	(void) free(id->userid);
	id->userid = NULL;
}

/**
 * \ingroup Core_Create
 *
 * __ops_seckey_free() frees the memory associated with "key". Note that
 * the key itself is not freed.
 *
 * \param key
 */

static void 
__ops_seckey_free(__ops_seckey_t *key)
{
	switch (key->pubkey.alg) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		free_BN(&key->key.rsa.d);
		free_BN(&key->key.rsa.p);
		free_BN(&key->key.rsa.q);
		free_BN(&key->key.rsa.u);
		break;

	case OPS_PKA_DSA:
		free_BN(&key->key.dsa.x);
		break;

	default:
		loggit("__ops_seckey_free: Unknown algorithm: %d (%s)",
			key->pubkey.alg,
			findtype((int) key->pubkey.alg, pubkeyalgs));
	}
	(void) free(key->checkhash);
	__ops_pubkey_free(&key->pubkey);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free any memory allocated when parsing the packet content */
static void 
__ops_pkt_free(__ops_packet_t *c)
{
	switch (c->tag) {
	case OPS_PARSER_PTAG:
	case OPS_PTAG_CT_COMPRESSED:
	case OPS_PTAG_SS_CREATION_TIME:
	case OPS_PTAG_SS_EXPIRATION_TIME:
	case OPS_PTAG_SS_KEY_EXPIRY:
	case OPS_PTAG_SS_TRUST:
	case OPS_PTAG_SS_ISSUER_KEY_ID:
	case OPS_PTAG_CT_1_PASS_SIG:
	case OPS_PTAG_SS_PRIMARY_USER_ID:
	case OPS_PTAG_SS_REVOCABLE:
	case OPS_PTAG_SS_REVOCATION_KEY:
	case OPS_PTAG_CT_LITERAL_DATA_HEADER:
	case OPS_PTAG_CT_LITERAL_DATA_BODY:
	case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
	case OPS_PTAG_CT_UNARMOURED_TEXT:
	case OPS_PTAG_CT_ARMOUR_TRAILER:
	case OPS_PTAG_CT_SIGNATURE_HEADER:
	case OPS_PTAG_CT_SE_DATA_HEADER:
	case OPS_PTAG_CT_SE_IP_DATA_HEADER:
	case OPS_PTAG_CT_SE_IP_DATA_BODY:
	case OPS_PTAG_CT_MDC:
	case OPS_GET_SECKEY:
		break;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
		__ops_headers_free(&c->u.cleartext_head.headers);
		break;

	case OPS_PTAG_CT_ARMOUR_HEADER:
		__ops_headers_free(&c->u.armour_header.headers);
		break;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:
		cleartext_trailer_free(&c->u.cleartext_trailer);
		break;

	case OPS_PTAG_CT_TRUST:
		trust_free(&c->u.trust);
		break;

	case OPS_PTAG_CT_SIGNATURE:
	case OPS_PTAG_CT_SIGNATURE_FOOTER:
		sig_free(&c->u.sig);
		break;

	case OPS_PTAG_CT_PUBLIC_KEY:
	case OPS_PTAG_CT_PUBLIC_SUBKEY:
		__ops_pubkey_free(&c->u.pubkey);
		break;

	case OPS_PTAG_CT_USER_ID:
		__ops_userid_free(&c->u.userid);
		break;

	case OPS_PTAG_SS_SIGNERS_USER_ID:
		__ops_userid_free(&c->u.ss_signer);
		break;

	case OPS_PTAG_CT_USER_ATTR:
		__ops_userattr_free(&c->u.userattr);
		break;

	case OPS_PTAG_SS_PREFERRED_SKA:
		ss_skapref_free(&c->u.ss_skapref);
		break;

	case OPS_PTAG_SS_PREFERRED_HASH:
		ss_hashpref_free(&c->u.ss_hashpref);
		break;

	case OPS_PTAG_SS_PREF_COMPRESS:
		ss_zpref_free(&c->u.ss_zpref);
		break;

	case OPS_PTAG_SS_KEY_FLAGS:
		ss_key_flags_free(&c->u.ss_key_flags);
		break;

	case OPS_PTAG_SS_KEYSERV_PREFS:
		ss_key_server_prefs_free(&c->u.ss_key_server_prefs);
		break;

	case OPS_PTAG_SS_FEATURES:
		ss_features_free(&c->u.ss_features);
		break;

	case OPS_PTAG_SS_NOTATION_DATA:
		ss_notation_free(&c->u.ss_notation);
		break;

	case OPS_PTAG_SS_REGEXP:
		ss_regexp_free(&c->u.ss_regexp);
		break;

	case OPS_PTAG_SS_POLICY_URI:
		ss_policy_free(&c->u.ss_policy);
		break;

	case OPS_PTAG_SS_PREF_KEYSERV:
		ss_keyserv_free(&c->u.ss_keyserv);
		break;

	case OPS_PTAG_SS_USERDEFINED00:
	case OPS_PTAG_SS_USERDEFINED01:
	case OPS_PTAG_SS_USERDEFINED02:
	case OPS_PTAG_SS_USERDEFINED03:
	case OPS_PTAG_SS_USERDEFINED04:
	case OPS_PTAG_SS_USERDEFINED05:
	case OPS_PTAG_SS_USERDEFINED06:
	case OPS_PTAG_SS_USERDEFINED07:
	case OPS_PTAG_SS_USERDEFINED08:
	case OPS_PTAG_SS_USERDEFINED09:
	case OPS_PTAG_SS_USERDEFINED10:
		ss_userdef_free(&c->u.ss_userdef);
		break;

	case OPS_PTAG_SS_RESERVED:
		ss_reserved_free(&c->u.ss_unknown);
		break;

	case OPS_PTAG_SS_REVOCATION_REASON:
		ss_revocation_free(&c->u.ss_revocation);
		break;

	case OPS_PTAG_SS_EMBEDDED_SIGNATURE:
		ss_embedded_sig_free(&c->u.ss_embedded_sig);
		break;

	case OPS_PARSER_PACKET_END:
		__ops_subpacket_free(&c->u.packet);
		break;

	case OPS_PARSER_ERROR:
	case OPS_PARSER_ERRCODE:
		break;

	case OPS_PTAG_CT_SECRET_KEY:
	case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY:
		__ops_seckey_free(&c->u.seckey);
		break;

	case OPS_PTAG_CT_PK_SESSION_KEY:
	case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
		__ops_pk_sesskey_free(&c->u.pk_sesskey);
		break;

	case OPS_GET_PASSPHRASE:
		__ops_cmd_get_passphrase_free(&c->u.skey_passphrase);
		break;

	default:
		loggit("Can't free %d (0x%x)", c->tag, c->tag);
	}
}

/**
   \ingroup Core_ReadPackets
*/
static int 
parse_pubkey_data(__ops_pubkey_t *key, __ops_region_t *region,
		      __ops_stream_t *stream)
{
	uint8_t   c = 0x0;

	if (region->readc != 0) {
		/* We should not have read anything so far */
		loggit("parse_pubkey_data: bad length");
		return 0;
	}

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	key->version = c;
	if (key->version < 2 || key->version > 4) {
		OPS_ERROR_1(&stream->errors, OPS_E_PROTO_BAD_PUBLIC_KEY_VRSN,
			    "Bad public key version (0x%02x)", key->version);
		return 0;
	}
	if (!limited_read_time(&key->birthtime, region, stream)) {
		return 0;
	}

	key->days_valid = 0;
	if ((key->version == 2 || key->version == 3) &&
	    !limread_scalar(&key->days_valid, 2, region, stream)) {
		return 0;
	}

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	key->alg = c;

	switch (key->alg) {
	case OPS_PKA_DSA:
		if (!limread_mpi(&key->key.dsa.p, region, stream) ||
		    !limread_mpi(&key->key.dsa.q, region, stream) ||
		    !limread_mpi(&key->key.dsa.g, region, stream) ||
		    !limread_mpi(&key->key.dsa.y, region, stream)) {
			return 0;
		}
		break;

	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		if (!limread_mpi(&key->key.rsa.n, region, stream) ||
		    !limread_mpi(&key->key.rsa.e, region, stream)) {
			return 0;
		}
		break;

	case OPS_PKA_ELGAMAL:
	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		if (!limread_mpi(&key->key.elgamal.p, region, stream) ||
		    !limread_mpi(&key->key.elgamal.g, region, stream) ||
		    !limread_mpi(&key->key.elgamal.y, region, stream)) {
			return 0;
		}
		break;

	default:
		OPS_ERROR_1(&stream->errors,
			OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,
			"Unsupported Public Key algorithm (%s)",
			findtype((int) key->alg, pubkeyalgs));
		return 0;
	}

	return 1;
}

/**
\ingroup Core_ReadPackets
\brief Calls the parse_cb_info's callback if present
\return Return value from callback, if present; else OPS_FINISHED
*/
static __ops_cb_ret_t 
__ops_callback(__ops_cbdata_t *cbdata, const __ops_packet_t *pkt)
{
	return (cbdata->cbfunc) ? (*cbdata->cbfunc)(cbdata, pkt) : OPS_FINISHED;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a public key packet.
 *
 * This function parses an entire v3 (== v2) or v4 public key packet for RSA, ElGamal, and DSA keys.
 *
 * Once the key has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the current Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.5.2
 */
static int 
parse_pubkey(__ops_content_tag_t tag, __ops_region_t *region,
		 __ops_stream_t *stream)
{
	__ops_packet_t pkt;

	if (!parse_pubkey_data(&pkt.u.pubkey, region, stream))
		return 0;

	/* XXX: this test should be done for all packets, surely? */
	if (region->readc != region->length) {
		OPS_ERROR_1(&stream->errors, OPS_E_R_UNCONSUMED_DATA,
			    "Unconsumed data (%d)",
			    region->length - region->readc);
		return 0;
	}
	CALLBACK(&stream->cbdata, tag, &pkt);

	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse one user attribute packet.
 *
 * User attribute packets contain one or more attribute subpackets.
 * For now, handle the whole packet as raw data.
 */

static int 
parse_userattr(__ops_region_t *region, __ops_stream_t *stream)
{

	__ops_packet_t pkt;

	/*
	 * xxx- treat as raw data for now. Could break down further into
	 * attribute sub-packets later - rachel
	 */

	if (region->readc != 0) {
		/* We should not have read anything so far */
		loggit("parse_userattr: bad length");
		return 0;
	}
	if (!read_data(&pkt.u.userattr.data, region, stream)) {
		return 0;
	}
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_USER_ATTR, &pkt);
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a user id.
 *
 * This function parses an user id packet, which is basically just a char array the size of the packet.
 *
 * The char array is to be treated as an UTF-8 string.
 *
 * The userid gets null terminated by this function.  Freeing it is the responsibility of the caller.
 *
 * Once the userid has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.11
 */
static int 
parse_userid(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_packet_t pkt;

	 if (region->readc != 0) {
		/* We should not have read anything so far */
		loggit("parse_userid: bad length");
		return 0;
	}
	pkt.u.userid.userid = __ops_new(region->length + 1);
	if (region->length &&
	    !limread(pkt.u.userid.userid, region->length, region, stream)) {
		return 0;
	}
	pkt.u.userid.userid[region->length] = '\0';
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_USER_ID, &pkt);
	return 1;
}

static __ops_hash_t     *
parse_hash_find(__ops_stream_t *stream,
		    const uint8_t keyid[OPS_KEY_ID_SIZE])
{
	size_t          n;

	for (n = 0; n < stream->hashc; ++n) {
		if (memcmp(stream->hashes[n].keyid, keyid,
					OPS_KEY_ID_SIZE) == 0) {
			return &stream->hashes[n].hash;
		}
	}
	return NULL;
}

/**
 * \ingroup Core_Parse
 * \brief Parse a version 3 signature.
 *
 * This function parses an version 3 signature packet, handling RSA and DSA signatures.
 *
 * Once the signature has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.2
 */
static int 
parse_v3_sig(__ops_region_t *region,
		   __ops_stream_t *stream)
{
	__ops_packet_t	pkt;
	uint8_t		c = 0x0;

	/* clear signature */
	(void) memset(&pkt.u.sig, 0x0, sizeof(pkt.u.sig));

	pkt.u.sig.info.version = OPS_V3;

	/* hash info length */
	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	if (c != 5) {
		ERRP(&stream->cbdata, pkt, "bad hash info length");
	}

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.type = c;
	/* XXX: check signature type */

	if (!limited_read_time(&pkt.u.sig.info.birthtime, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.gotbirthtime = 1;

	if (!limread(pkt.u.sig.info.signer, OPS_KEY_ID_SIZE, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.gotsigner = 1;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.key_alg = c;
	/* XXX: check algorithm */

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.hash_alg = c;
	/* XXX: check algorithm */

	if (!limread(pkt.u.sig.hash2, 2, region, stream)) {
		return 0;
	}

	switch (pkt.u.sig.info.key_alg) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_SIGN_ONLY:
		if (!limread_mpi(&pkt.u.sig.info.sig.rsa.sig, region, stream)) {
			return 0;
		}
		break;

	case OPS_PKA_DSA:
		if (!limread_mpi(&pkt.u.sig.info.sig.dsa.r, region, stream) ||
		    !limread_mpi(&pkt.u.sig.info.sig.dsa.s, region, stream)) {
			return 0;
		}
		break;

	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		if (!limread_mpi(&pkt.u.sig.info.sig.elgamal.r, region,
				stream) ||
		    !limread_mpi(&pkt.u.sig.info.sig.elgamal.s, region,
		    		stream)) {
			return 0;
		}
		break;

	default:
		OPS_ERROR_1(&stream->errors,
			OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG,
			"Unsupported signature key algorithm (%s)",
			findtype((int) pkt.u.sig.info.key_alg, pubkeyalgs));
		return 0;
	}

	if (region->readc != region->length) {
		OPS_ERROR_1(&stream->errors, OPS_E_R_UNCONSUMED_DATA,
			"Unconsumed data (%d)",
			region->length - region->readc);
		return 0;
	}
	if (pkt.u.sig.info.gotsigner) {
		pkt.u.sig.hash = parse_hash_find(stream, pkt.u.sig.info.signer);
	}
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_SIGNATURE, &pkt);
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse one signature sub-packet.
 *
 * Version 4 signatures can have an arbitrary amount of (hashed and unhashed) subpackets.  Subpackets are used to hold
 * optional attributes of subpackets.
 *
 * This function parses one such signature subpacket.
 *
 * Once the subpacket has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire subpacket.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int 
parse_one_sig_subpacket(__ops_sig_t *sig,
			      __ops_region_t *region,
			      __ops_stream_t *stream)
{
	__ops_region_t	subregion;
	__ops_packet_t	pkt;
	uint8_t   bools = 0x0;
	uint8_t	c = 0x0;
	unsigned	doread = 1;
	unsigned        t8;
	unsigned        t7;

	__ops_init_subregion(&subregion, region);
	if (!limited_read_new_length(&subregion.length, region, stream)) {
		return 0;
	}

	if (subregion.length > region->length) {
		ERRP(&stream->cbdata, pkt, "Subpacket too long");
	}

	if (!limread(&c, 1, &subregion, stream)) {
		return 0;
	}

	t8 = (c & 0x7f) / 8;
	t7 = 1 << (c & 7);

	pkt.critical = (unsigned)c >> 7;
	pkt.tag = OPS_PTAG_SIG_SUBPKT_BASE + (c & 0x7f);

	/* Application wants it delivered raw */
	if (stream->ss_raw[t8] & t7) {
		pkt.u.ss_raw.tag = pkt.tag;
		pkt.u.ss_raw.length = subregion.length - 1;
		pkt.u.ss_raw.raw = __ops_new(pkt.u.ss_raw.length);
		if (!limread(pkt.u.ss_raw.raw, pkt.u.ss_raw.length,
				&subregion, stream)) {
			return 0;
		}
		CALLBACK(&stream->cbdata, OPS_PTAG_RAW_SS, &pkt);
		return 1;
	}
	switch (pkt.tag) {
	case OPS_PTAG_SS_CREATION_TIME:
	case OPS_PTAG_SS_EXPIRATION_TIME:
	case OPS_PTAG_SS_KEY_EXPIRY:
		if (!limited_read_time(&pkt.u.ss_time.time, &subregion, stream))
			return 0;
		if (pkt.tag == OPS_PTAG_SS_CREATION_TIME) {
			sig->info.birthtime = pkt.u.ss_time.time;
			sig->info.gotbirthtime = 1;
		}
		break;

	case OPS_PTAG_SS_TRUST:
		if (!limread(&pkt.u.ss_trust.level, 1, &subregion, stream) ||
		    !limread(&pkt.u.ss_trust.amount, 1, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_REVOCABLE:
		if (!limread(&bools, 1, &subregion, stream)) {
			return 0;
		}
		pkt.u.ss_revocable.revocable = !!bools;
		break;

	case OPS_PTAG_SS_ISSUER_KEY_ID:
		if (!limread(pkt.u.ss_issuer.keyid, OPS_KEY_ID_SIZE,
				&subregion, stream)) {
			return 0;
		}
		(void) memcpy(sig->info.signer, pkt.u.ss_issuer.keyid,
					OPS_KEY_ID_SIZE);
		sig->info.gotsigner = 1;
		break;

	case OPS_PTAG_SS_PREFERRED_SKA:
		if (!read_data(&pkt.u.ss_skapref.data, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_PREFERRED_HASH:
		if (!read_data(&pkt.u.ss_hashpref.data, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_PREF_COMPRESS:
		if (!read_data(&pkt.u.ss_zpref.data,
				&subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_PRIMARY_USER_ID:
		if (!limread(&bools, 1, &subregion, stream)) {
			return 0;
		}
		pkt.u.ss_primary_userid.primary_userid = !!bools;
		break;

	case OPS_PTAG_SS_KEY_FLAGS:
		if (!read_data(&pkt.u.ss_key_flags.data, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_KEYSERV_PREFS:
		if (!read_data(&pkt.u.ss_key_server_prefs.data, &subregion,
				stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_FEATURES:
		if (!read_data(&pkt.u.ss_features.data, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_SIGNERS_USER_ID:
		if (!read_unsig_str(&pkt.u.ss_signer.userid, &subregion,
				stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_EMBEDDED_SIGNATURE:
		/* \todo should do something with this sig? */
		if (!read_data(&pkt.u.ss_embedded_sig.sig, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_NOTATION_DATA:
		if (!limread_data(&pkt.u.ss_notation.flags, 4,
				&subregion, stream)) {
			return 0;
		}
		if (!limread_size_t(&pkt.u.ss_notation.name.len, 2,
				&subregion, stream)) {
			return 0;
		}
		if (!limread_size_t(&pkt.u.ss_notation.value.len, 2,
				&subregion, stream)) {
			return 0;
		}
		if (!limread_data(&pkt.u.ss_notation.name,
				pkt.u.ss_notation.name.len,
				&subregion, stream)) {
			return 0;
		}
		if (!limread_data(&pkt.u.ss_notation.value,
			   pkt.u.ss_notation.value.len,
			   &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_POLICY_URI:
		if (!read_string(&pkt.u.ss_policy.url, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_REGEXP:
		if (!read_string(&pkt.u.ss_regexp.regexp, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_PREF_KEYSERV:
		if (!read_string(&pkt.u.ss_keyserv.name, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_USERDEFINED00:
	case OPS_PTAG_SS_USERDEFINED01:
	case OPS_PTAG_SS_USERDEFINED02:
	case OPS_PTAG_SS_USERDEFINED03:
	case OPS_PTAG_SS_USERDEFINED04:
	case OPS_PTAG_SS_USERDEFINED05:
	case OPS_PTAG_SS_USERDEFINED06:
	case OPS_PTAG_SS_USERDEFINED07:
	case OPS_PTAG_SS_USERDEFINED08:
	case OPS_PTAG_SS_USERDEFINED09:
	case OPS_PTAG_SS_USERDEFINED10:
		if (!read_data(&pkt.u.ss_userdef.data, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_RESERVED:
		if (!read_data(&pkt.u.ss_unknown.data, &subregion, stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_REVOCATION_REASON:
		/* first byte is the machine-readable code */
		if (!limread(&pkt.u.ss_revocation.code, 1, &subregion, stream)) {
			return 0;
		}
		/* the rest is a human-readable UTF-8 string */
		if (!read_string(&pkt.u.ss_revocation.reason, &subregion,
				stream)) {
			return 0;
		}
		break;

	case OPS_PTAG_SS_REVOCATION_KEY:
		/* octet 0 = class. Bit 0x80 must be set */
		if (!limread(&pkt.u.ss_revocation_key.class, 1,
				&subregion, stream)) {
			return 0;
		}
		if (!(pkt.u.ss_revocation_key.class & 0x80)) {
			loggit("Warning: OPS_PTAG_SS_REVOCATION_KEY class: "
			       "Bit 0x80 should be set");
			return 0;
		}
		/* octet 1 = algid */
		if (!limread(&pkt.u.ss_revocation_key.algid, 1,
				&subregion, stream)) {
			return 0;
		}
		/* octets 2-21 = fingerprint */
		if (!limread(&pkt.u.ss_revocation_key.fingerprint[0],
				OPS_FINGERPRINT_SIZE, &subregion, stream)) {
			return 0;
		}
		break;

	default:
		if (stream->ss_parsed[t8] & t7) {
			OPS_ERROR_1(&stream->errors, OPS_E_PROTO_UNKNOWN_SS,
				    "Unknown signature subpacket type (%d)",
				    c & 0x7f);
		}
		doread = 0;
		break;
	}

	/* Application doesn't want it delivered parsed */
	if (!(stream->ss_parsed[t8] & t7)) {
		if (pkt.critical) {
			OPS_ERROR_1(&stream->errors,
				OPS_E_PROTO_CRITICAL_SS_IGNORED,
				"Critical signature subpacket ignored (%d)",
				c & 0x7f);
		}
		if (!doread &&
		    !limskip(subregion.length - 1, &subregion, stream)) {
			return 0;
		}
		if (doread) {
			__ops_pkt_free(&pkt);
		}
		return 1;
	}
	if (doread && subregion.readc != subregion.length) {
		OPS_ERROR_1(&stream->errors, OPS_E_R_UNCONSUMED_DATA,
			    "Unconsumed data (%d)",
			    subregion.length - subregion.readc);
		return 0;
	}
	CALLBACK(&stream->cbdata, pkt.tag, &pkt);
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse several signature subpackets.
 *
 * Hashed and unhashed subpacket sets are preceded by an octet count that specifies the length of the complete set.
 * This function parses this length and then calls parse_one_sig_subpacket() for each subpacket until the
 * entire set is consumed.
 *
 * This function does not call the callback directly, parse_one_sig_subpacket() does for each subpacket.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int 
parse_sig_subpkts(__ops_sig_t *sig,
			   __ops_region_t *region,
			   __ops_stream_t *stream)
{
	__ops_region_t	subregion;
	__ops_packet_t	pkt;

	__ops_init_subregion(&subregion, region);
	if (!limread_scalar(&subregion.length, 2, region, stream)) {
		return 0;
	}

	if (subregion.length > region->length) {
		ERRP(&stream->cbdata, pkt, "Subpacket set too long");
	}

	while (subregion.readc < subregion.length) {
		if (!parse_one_sig_subpacket(sig, &subregion, stream)) {
			return 0;
		}
	}

	if (subregion.readc != subregion.length) {
		if (!limskip(subregion.length - subregion.readc,
				&subregion, stream)) {
			ERRP(&stream->cbdata, pkt,
"parse_sig_subpkts: subpacket length read mismatch");
		}
		ERRP(&stream->cbdata, pkt, "Subpacket length mismatch");
	}
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a version 4 signature.
 *
 * This function parses a version 4 signature including all its hashed and unhashed subpackets.
 *
 * Once the signature packet has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int 
parse_v4_sig(__ops_region_t *region, __ops_stream_t *stream)
{
	uint8_t   c = 0x0;
	__ops_packet_t pkt;

	/* clear signature */
	(void) memset(&pkt.u.sig, 0x0, sizeof(pkt.u.sig));

	/*
	 * We need to hash the packet data from version through the hashed
	 * subpacket data
	 */

	pkt.u.sig.v4_hashstart = stream->readinfo.alength - 1;

	/* Set version,type,algorithms */

	pkt.u.sig.info.version = OPS_V4;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.type = c;
	/* XXX: check signature type */

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.key_alg = c;
	/* XXX: check algorithm */
	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.sig.info.hash_alg = c;
	/* XXX: check algorithm */
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_SIGNATURE_HEADER, &pkt);

	if (!parse_sig_subpkts(&pkt.u.sig, region, stream)) {
		return 0;
	}

	pkt.u.sig.info.v4_hashlen = stream->readinfo.alength
					- pkt.u.sig.v4_hashstart;

	/* copy hashed subpackets */
	if (pkt.u.sig.info.v4_hashed) {
		(void) free(pkt.u.sig.info.v4_hashed);
	}
	pkt.u.sig.info.v4_hashed = __ops_new(pkt.u.sig.info.v4_hashlen);

	if (!stream->readinfo.accumulate) {
		/* We must accumulate, else we can't check the signature */
		loggit("*** ERROR: must set accumulate to 1");
		return 0;
	}
	(void) memcpy(pkt.u.sig.info.v4_hashed,
	       stream->readinfo.accumulated + pkt.u.sig.v4_hashstart,
	       pkt.u.sig.info.v4_hashlen);

	if (!parse_sig_subpkts(&pkt.u.sig, region, stream)) {
		return 0;
	}

	if (!limread(pkt.u.sig.hash2, 2, region, stream)) {
		return 0;
	}

	switch (pkt.u.sig.info.key_alg) {
	case OPS_PKA_RSA:
		if (!limread_mpi(&pkt.u.sig.info.sig.rsa.sig, region, stream)) {
			return 0;
		}
		break;

	case OPS_PKA_DSA:
		if (!limread_mpi(&pkt.u.sig.info.sig.dsa.r, region, stream)) {
			/*
			 * usually if this fails, it just means we've reached
			 * the end of the keyring
			 */
			return 0;
		}
		if (!limread_mpi(&pkt.u.sig.info.sig.dsa.s, region, stream)) {
			ERRP(&stream->cbdata, pkt,
			"Error reading DSA s field in signature");
		}
		break;

	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		if (!limread_mpi(&pkt.u.sig.info.sig.elgamal.r, region,
				stream) ||
		    !limread_mpi(&pkt.u.sig.info.sig.elgamal.s, region,
		    		stream)) {
			return 0;
		}
		break;

	case OPS_PKA_PRIVATE00:
	case OPS_PKA_PRIVATE01:
	case OPS_PKA_PRIVATE02:
	case OPS_PKA_PRIVATE03:
	case OPS_PKA_PRIVATE04:
	case OPS_PKA_PRIVATE05:
	case OPS_PKA_PRIVATE06:
	case OPS_PKA_PRIVATE07:
	case OPS_PKA_PRIVATE08:
	case OPS_PKA_PRIVATE09:
	case OPS_PKA_PRIVATE10:
		if (!read_data(&pkt.u.sig.info.sig.unknown.data, region,
				stream)) {
			return 0;
		}
		break;

	default:
		OPS_ERROR_1(&stream->errors,
			OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG,
			"Bad v4 signature key algorithm (%s)",
			findtype((int) pkt.u.sig.info.key_alg, pubkeyalgs));
		return 0;
	}
	if (region->readc != region->length) {
		OPS_ERROR_1(&stream->errors, OPS_E_R_UNCONSUMED_DATA,
			    "Unconsumed data (%d)",
			    region->length - region->readc);
		return 0;
	}
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_SIGNATURE_FOOTER, &pkt);
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a signature subpacket.
 *
 * This function calls the appropriate function to handle v3 or v4 signatures.
 *
 * Once the signature packet has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 */
static int 
parse_sig(__ops_region_t *region, __ops_stream_t *stream)
{
	uint8_t   c = 0x0;
	__ops_packet_t pkt;

	if (region->readc != 0) {
		/* We should not have read anything so far */
		loggit("parse_sig: bad length");
		return 0;
	}

	(void) memset(&pkt, 0x0, sizeof(pkt));
	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	if (c == 2 || c == 3) {
		return parse_v3_sig(region, stream);
	}
	if (c == 4) {
		return parse_v4_sig(region, stream);
	}
	OPS_ERROR_1(&stream->errors, OPS_E_PROTO_BAD_SIGNATURE_VRSN,
		    "Bad signature version (%d)", c);
	return 0;
}

/**
 \ingroup Core_ReadPackets
 \brief Parse Compressed packet
*/
static int 
parse_compressed(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_packet_t	pkt;
	uint8_t		c = 0x0;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}

	pkt.u.compressed.type = c;

	CALLBACK(&stream->cbdata, OPS_PTAG_CT_COMPRESSED, &pkt);

	/*
	 * The content of a compressed data packet is more OpenPGP packets
	 * once decompressed, so recursively handle them
	 */

	return __ops_decompress(region, stream, pkt.u.compressed.type);
}

/* XXX: this could be improved by sharing all hashes that are the */
/* same, then duping them just before checking the signature. */
static void 
parse_hash_init(__ops_stream_t *stream, __ops_hash_alg_t type,
		    const uint8_t *keyid)
{
	__ops_hashtype_t *hash;

	stream->hashes = realloc(stream->hashes,
			      (stream->hashc + 1) * sizeof(*stream->hashes));
	hash = &stream->hashes[stream->hashc++];

	__ops_hash_any(&hash->hash, type);
	hash->hash.init(&hash->hash);
	(void) memcpy(hash->keyid, keyid, sizeof(hash->keyid));
}

/**
   \ingroup Core_ReadPackets
   \brief Parse a One Pass Signature packet
*/
static int 
parse_one_pass(__ops_region_t * region, __ops_stream_t * stream)
{
	uint8_t   c = 0x0;
	__ops_packet_t pkt;

	if (!limread(&pkt.u.one_pass_sig.version, 1, region, stream)) {
		return 0;
	}
	if (pkt.u.one_pass_sig.version != 3) {
		OPS_ERROR_1(&stream->errors, OPS_E_PROTO_BAD_ONE_PASS_SIG_VRSN,
			    "Bad one-pass signature version (%d)",
			    pkt.u.one_pass_sig.version);
		return 0;
	}
	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.one_pass_sig.sig_type = c;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.one_pass_sig.hash_alg = c;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.one_pass_sig.key_alg = c;

	if (!limread(pkt.u.one_pass_sig.keyid,
			  sizeof(pkt.u.one_pass_sig.keyid), region, stream)) {
		return 0;
	}

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.one_pass_sig.nested = !!c;
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_1_PASS_SIG, &pkt);
	/* XXX: we should, perhaps, let the app choose whether to hash or not */
	parse_hash_init(stream, pkt.u.one_pass_sig.hash_alg,
			    pkt.u.one_pass_sig.keyid);
	return 1;
}

/**
 \ingroup Core_ReadPackets
 \brief Parse a Trust packet
*/
static int
parse_trust(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_packet_t pkt;

	if (!read_data(&pkt.u.trust.data, region, stream)) {
		return 0;
	}
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_TRUST, &pkt);
	return 1;
}

static void 
parse_hash_data(__ops_stream_t *stream, const void *data, size_t len)
{
	size_t          n;

	for (n = 0; n < stream->hashc; ++n) {
		stream->hashes[n].hash.add(&stream->hashes[n].hash, data, len);
	}
}

/**
   \ingroup Core_ReadPackets
   \brief Parse a Literal Data packet
*/
static int 
parse_litdata(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_memory_t	*mem;
	__ops_packet_t	 pkt;
	uint8_t	 c = 0x0;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.litdata_header.format = c;
	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	if (!limread((uint8_t *)pkt.u.litdata_header.filename,
			(unsigned)c, region, stream)) {
		return 0;
	}
	pkt.u.litdata_header.filename[c] = '\0';
	if (!limited_read_time(&pkt.u.litdata_header.mtime, region, stream)) {
		return 0;
	}
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_LITERAL_DATA_HEADER, &pkt);
	mem = pkt.u.litdata_body.mem = __ops_new(sizeof(*mem));
	__ops_memory_init(pkt.u.litdata_body.mem,
			(unsigned)(region->length * 1.01) + 12);
	pkt.u.litdata_body.data = mem->buf;

	while (region->readc < region->length) {
		unsigned        readc = region->length - region->readc;

		if (!limread(mem->buf, readc, region, stream)) {
			return 0;
		}
		pkt.u.litdata_body.length = readc;
		parse_hash_data(stream, pkt.u.litdata_body.data, region->length);
		CALLBACK(&stream->cbdata, OPS_PTAG_CT_LITERAL_DATA_BODY, &pkt);
	}

	/* XXX - get rid of mem here? */

	return 1;
}

static int 
consume_packet(__ops_region_t *region, __ops_stream_t *stream, unsigned warn)
{
	__ops_packet_t	pkt;
	__ops_data_t	remainder;

	if (region->indeterm) {
		ERRP(&stream->cbdata, pkt,
			"Can't consume indeterminate packets");
	}

	if (read_data(&remainder, region, stream)) {
		/* now throw it away */
		data_free(&remainder);
		if (warn) {
			OPS_ERROR(&stream->errors, OPS_E_P_PACKET_CONSUMED,
				"Warning: packet consumer");
		}
		return 1;
	}
	OPS_ERROR(&stream->errors, OPS_E_P_PACKET_NOT_CONSUMED,
			(warn) ? "Warning: Packet was not consumed" :
				"Packet was not consumed");
	return warn;
}

static void __ops_reader_push_decrypt(__ops_stream_t *,
			__ops_crypt_t *,
			__ops_region_t *);
static void __ops_reader_pop_decrypt(__ops_stream_t *);

static int 
hash_reader(void *dest,
		size_t length,
		__ops_error_t **errors,
		__ops_reader_t *readinfo,
		__ops_cbdata_t *cbdata)
{
	__ops_hash_t	*hash = readinfo->arg;
	int		 r;
	
	r = __ops_stacked_read(dest, length, errors, readinfo, cbdata);
	if (r <= 0) {
		return r;
	}
	hash->add(hash, dest, (unsigned)r);
	return r;
}

/**
   \ingroup Internal_Readers_Hash
   \brief Push hashed data reader on stack
*/
static void 
__ops_reader_push_hash(__ops_stream_t *stream, __ops_hash_t *hash)
{
	hash->init(hash);
	__ops_reader_push(stream, hash_reader, NULL, hash);
}

/**
   \ingroup Internal_Readers_Hash
   \brief Pop hashed data reader from stack
*/
static void 
__ops_reader_pop_hash(__ops_stream_t *stream)
{
	__ops_reader_pop(stream);
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a secret key
 */
static int 
parse_seckey(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_packet_t		pkt;
	__ops_region_t		encregion;
	__ops_region_t	       *saved_region = NULL;
	uint8_t		c = 0x0;
	__ops_crypt_t		decrypt;
	__ops_hash_t		checkhash;
	unsigned		blocksize;
	unsigned		crypted;
	int			ret = 1;

	(void) memset(&pkt, 0x0, sizeof(pkt));
	if (!parse_pubkey_data(&pkt.u.seckey.pubkey, region, stream)) {
		return 0;
	}
	stream->reading_v3_secret = pkt.u.seckey.pubkey.version != OPS_V4;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.seckey.s2k_usage = c;

	if (pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED ||
	    pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED) {
		if (!limread(&c, 1, region, stream)) {
			return 0;
		}
		pkt.u.seckey.alg = c;
		if (!limread(&c, 1, region, stream)) {
			return 0;
		}
		pkt.u.seckey.s2k_specifier = c;
		switch (pkt.u.seckey.s2k_specifier) {
		case OPS_S2KS_SIMPLE:
		case OPS_S2KS_SALTED:
		case OPS_S2KS_ITERATED_AND_SALTED:
			break;
		default:
			loggit("parse_seckey: bad seckey");
			return 0;
		}
		if (!limread(&c, 1, region, stream)) {
			return 0;
		}
		pkt.u.seckey.hash_alg = c;
		if (pkt.u.seckey.s2k_specifier != OPS_S2KS_SIMPLE &&
		    !limread(pkt.u.seckey.salt, 8, region, stream)) {
			return 0;
		}
		if (pkt.u.seckey.s2k_specifier ==
					OPS_S2KS_ITERATED_AND_SALTED) {
			if (!limread(&c, 1, region, stream)) {
				return 0;
			}
			pkt.u.seckey.octetc =
				(16 + ((unsigned)c & 15)) <<
						(((unsigned)c >> 4) + 6);
		}
	} else if (pkt.u.seckey.s2k_usage != OPS_S2KU_NONE) {
		/* this is V3 style, looks just like a V4 simple hash */
		pkt.u.seckey.alg = c;
		pkt.u.seckey.s2k_usage = OPS_S2KU_ENCRYPTED;
		pkt.u.seckey.s2k_specifier = OPS_S2KS_SIMPLE;
		pkt.u.seckey.hash_alg = OPS_HASH_MD5;
	}
	crypted = pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED ||
		pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED;

	if (crypted) {
		__ops_packet_t	seckey;
		uint8_t   key[OPS_MAX_KEY_SIZE + OPS_MAX_HASH_SIZE];
		__ops_hash_t	hashes[(OPS_MAX_KEY_SIZE + OPS_MIN_HASH_SIZE - 1) / OPS_MIN_HASH_SIZE];
		size_t          passlen;
		char           *passphrase;
		int             hashsize;
		int             keysize;
		int             n;

		blocksize = __ops_block_size(pkt.u.seckey.alg);
		if (blocksize == 0 || blocksize > OPS_MAX_BLOCK_SIZE) {
			loggit("parse_seckey: bad blocksize");
			return 0;
		}

		if (!limread(pkt.u.seckey.iv, blocksize, region, stream)) {
			return 0;
		}
		(void) memset(&seckey, 0x0, sizeof(seckey));
		passphrase = NULL;
		seckey.u.skey_passphrase.passphrase = &passphrase;
		seckey.u.skey_passphrase.seckey = &pkt.u.seckey;
		CALLBACK(&stream->cbdata, OPS_GET_PASSPHRASE, &seckey);
		if (!passphrase) {
			if (!consume_packet(region, stream, 0)) {
				return 0;
			}

			CALLBACK(&stream->cbdata,
				OPS_PTAG_CT_ENCRYPTED_SECRET_KEY, &pkt);

			return 1;
		}
		keysize = __ops_key_size(pkt.u.seckey.alg);
		if (keysize == 0 || keysize > OPS_MAX_KEY_SIZE) {
			loggit("parse_seckey: bad keysize");
			return 0;
		}

		hashsize = __ops_hash_size(pkt.u.seckey.hash_alg);
		if (hashsize == 0 || hashsize > OPS_MAX_HASH_SIZE) {
			loggit("parse_seckey: bad hashsize");
			return 0;
		}

		for (n = 0; n * hashsize < keysize; ++n) {
			int             i;

			__ops_hash_any(&hashes[n],
				pkt.u.seckey.hash_alg);
			hashes[n].init(&hashes[n]);
			/* preload hashes with zeroes... */
			for (i = 0; i < n; ++i) {
				hashes[n].add(&hashes[n],
					(const uint8_t *) "", 1);
			}
		}
		passlen = strlen(passphrase);
		for (n = 0; n * hashsize < keysize; ++n) {
			unsigned        i;

			switch (pkt.u.seckey.s2k_specifier) {
			case OPS_S2KS_SALTED:
				hashes[n].add(&hashes[n],
					pkt.u.seckey.salt,
					OPS_SALT_SIZE);
				/* FALLTHROUGH */
			case OPS_S2KS_SIMPLE:
				hashes[n].add(&hashes[n],
					(uint8_t *) passphrase, passlen);
				break;

			case OPS_S2KS_ITERATED_AND_SALTED:
				for (i = 0; i < pkt.u.seckey.octetc;
						i += passlen + OPS_SALT_SIZE) {
					unsigned	j;

					j = passlen + OPS_SALT_SIZE;
					if (i + j > pkt.u.seckey.octetc && i != 0) {
						j = pkt.u.seckey.octetc - i;
					}
					hashes[n].add(&hashes[n],
						pkt.u.seckey.salt,
						(unsigned)(j > OPS_SALT_SIZE) ?
							OPS_SALT_SIZE : j);
					if (j > OPS_SALT_SIZE) {
						hashes[n].add(&hashes[n],
						(uint8_t *) passphrase,
						j - OPS_SALT_SIZE);
					}
				}

			}
		}

		for (n = 0; n * hashsize < keysize; ++n) {
			int	r;

			r = hashes[n].finish(&hashes[n], key + n * hashsize);
			if (r != hashsize) {
				loggit("parse_seckey: bad r");
				return 0;
			}
		}

		__ops_forget(passphrase, passlen);
		(void) free(passphrase);

		__ops_crypt_any(&decrypt, pkt.u.seckey.alg);
		decrypt.set_iv(&decrypt, pkt.u.seckey.iv);
		decrypt.set_key(&decrypt, key);

		/* now read encrypted data */

		__ops_reader_push_decrypt(stream, &decrypt, region);

		/*
		 * Since all known encryption for PGP doesn't compress, we
		 * can limit to the same length as the current region (for
		 * now).
		 */
		__ops_init_subregion(&encregion, NULL);
		encregion.length = region->length - region->readc;
		if (pkt.u.seckey.pubkey.version != OPS_V4) {
			encregion.length -= 2;
		}
		saved_region = region;
		region = &encregion;
	}
	if (pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED) {
		pkt.u.seckey.checkhash = __ops_new(OPS_CHECKHASH_SIZE);
		__ops_hash_sha1(&checkhash);
		__ops_reader_push_hash(stream, &checkhash);
	} else {
		__ops_reader_push_sum16(stream);
	}

	switch (pkt.u.seckey.pubkey.alg) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		if (!limread_mpi(&pkt.u.seckey.key.rsa.d, region, stream) ||
		    !limread_mpi(&pkt.u.seckey.key.rsa.p, region, stream) ||
		    !limread_mpi(&pkt.u.seckey.key.rsa.q, region, stream) ||
		    !limread_mpi(&pkt.u.seckey.key.rsa.u, region, stream)) {
			ret = 0;
		}
		break;

	case OPS_PKA_DSA:
		if (!limread_mpi(&pkt.u.seckey.key.dsa.x, region, stream)) {
			ret = 0;
		}
		break;

	default:
		OPS_ERROR_2(&stream->errors,
			OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,
			"Unsupported Public Key algorithm %d (%s)",
			pkt.u.seckey.pubkey.alg,
			findtype((int) pkt.u.seckey.pubkey.alg, pubkeyalgs));
		ret = 0;
	}

	stream->reading_v3_secret = 0;

	if (pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED) {
		uint8_t   hash[OPS_CHECKHASH_SIZE];

		__ops_reader_pop_hash(stream);
		checkhash.finish(&checkhash, hash);

		if (crypted &&
		    pkt.u.seckey.pubkey.version != OPS_V4) {
			__ops_reader_pop_decrypt(stream);
			region = saved_region;
		}
		if (ret) {
			if (!limread(pkt.u.seckey.checkhash,
				OPS_CHECKHASH_SIZE, region, stream)) {
				return 0;
			}

			if (memcmp(hash, pkt.u.seckey.checkhash,
					OPS_CHECKHASH_SIZE) != 0) {
				ERRP(&stream->cbdata, pkt,
					"Hash mismatch in secret key");
			}
		}
	} else {
		uint16_t  sum;

		sum = __ops_reader_pop_sum16(stream);
		if (crypted &&
		    pkt.u.seckey.pubkey.version != OPS_V4) {
			__ops_reader_pop_decrypt(stream);
			region = saved_region;
		}
		if (ret) {
			if (!limread_scalar(&pkt.u.seckey.checksum, 2,
					region, stream))
				return 0;

			if (sum != pkt.u.seckey.checksum) {
				ERRP(&stream->cbdata, pkt,
					"Checksum mismatch in secret key");
			}
		}
	}

	if (crypted && pkt.u.seckey.pubkey.version == OPS_V4) {
		__ops_reader_pop_decrypt(stream);
	}
	if (ret && region->readc != region->length) {
		loggit("parse_seckey: bad length");
		return 0;
	}
	if (!ret) {
		return 0;
	}
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_SECRET_KEY, &pkt);
	return 1;
}

/**
\ingroup Core_MPI
\brief Decrypt and unencode MPI
\param buf Buffer in which to write decrypted unencoded MPI
\param buflen Length of buffer
\param encmpi
\param seckey
\return length of MPI
\note only RSA at present
*/
static int 
__ops_decrypt_decode_mpi(uint8_t *buf,
				unsigned buflen,
				const BIGNUM *encmpi,
				const __ops_seckey_t *seckey)
{
	unsigned	mpisize;
	uint8_t		encmpibuf[NETPGP_BUFSIZ];
	uint8_t		mpibuf[NETPGP_BUFSIZ];
	int		n;
	int		i;

	mpisize = BN_num_bytes(encmpi);
	/* MPI can't be more than 65,536 */
	if (mpisize > sizeof(encmpibuf)) {
		loggit("mpisize too big %u", mpisize);
		return -1;
	}
	BN_bn2bin(encmpi, encmpibuf);

	if (seckey->pubkey.alg != OPS_PKA_RSA) {
		loggit("pubkey algorithm wrong");
		return -1;
	}

	n = __ops_rsa_private_decrypt(mpibuf, encmpibuf,
				(unsigned)(BN_num_bits(encmpi) + 7) / 8,
				&seckey->key.rsa, &seckey->pubkey.key.rsa);
	if (n == -1) {
		loggit("ops_rsa_private_decrypt failure");
		return -1;
	}

	if (n <= 0) {
		return -1;
	}

	/* Decode EME-PKCS1_V1_5 (RFC 2437). */

	if (mpibuf[0] != 0 || mpibuf[1] != 2) {
		return -1;
	}

	/* Skip the random bytes. */
	for (i = 2; i < n && mpibuf[i]; ++i) {
	}

	if (i == n || i < 10) {
		return -1;
	}

	/* Skip the zero */
	i += 1;

	/* this is the unencoded m buf */
	if ((unsigned) (n - i) <= buflen) {
		(void) memcpy(buf, mpibuf + i, (unsigned)(n - i));
	}

	return n - i;
}

/**
   \ingroup Core_ReadPackets
   \brief Parse a Public Key Session Key packet
*/
static int 
parse_pk_sesskey(__ops_region_t *region, __ops_stream_t *stream)
{
	const __ops_seckey_t	*secret;
	__ops_packet_t		 sesskey;
	__ops_packet_t		 pkt;
	uint8_t		*iv;
	uint8_t	   	 c = 0x0;
	uint8_t		 cs[2];
	unsigned		 k;
	BIGNUM			*enc_m;
	int			 n;

	/* Can't rely on it being CAST5 */
	/* \todo FIXME RW */
	/* const size_t sz_unencoded_m_buf=CAST_KEY_LENGTH+1+2; */
	uint8_t		 unencoded_m_buf[1024];

	cs[0] = cs[1] = 0x0;
	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.pk_sesskey.version = c;
	if (pkt.u.pk_sesskey.version != OPS_PKSK_V3) {
		OPS_ERROR_1(&stream->errors, OPS_E_PROTO_BAD_PKSK_VRSN,
			"Bad public-key encrypted session key version (%d)",
			    pkt.u.pk_sesskey.version);
		return 0;
	}
	if (!limread(pkt.u.pk_sesskey.keyid,
			  sizeof(pkt.u.pk_sesskey.keyid), region, stream)) {
		return 0;
	}
	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.pk_sesskey.alg = c;
	switch (pkt.u.pk_sesskey.alg) {
	case OPS_PKA_RSA:
		if (!limread_mpi(&pkt.u.pk_sesskey.parameters.rsa.encrypted_m,
				      region, stream)) {
			return 0;
		}
		enc_m = pkt.u.pk_sesskey.parameters.rsa.encrypted_m;
		break;

	case OPS_PKA_ELGAMAL:
		if (!limread_mpi(&pkt.u.pk_sesskey.parameters.elgamal.g_to_k,
				      region, stream) ||
		    !limread_mpi(
			&pkt.u.pk_sesskey.parameters.elgamal.encrypted_m,
					 region, stream)) {
			return 0;
		}
		enc_m = pkt.u.pk_sesskey.parameters.elgamal.encrypted_m;
		break;

	default:
		OPS_ERROR_1(&stream->errors,
			OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,
			"Unknown public key algorithm in session key (%s)",
			findtype((int) pkt.u.pk_sesskey.alg, pubkeyalgs));
		return 0;
	}

	(void) memset(&sesskey, 0x0, sizeof(sesskey));
	secret = NULL;
	sesskey.u.get_seckey.seckey = &secret;
	sesskey.u.get_seckey.pk_sesskey = &pkt.u.pk_sesskey;

	CALLBACK(&stream->cbdata, OPS_GET_SECKEY, &sesskey);

	if (!secret) {
		CALLBACK(&stream->cbdata, OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY,
			&pkt);
		return 1;
	}
	n = __ops_decrypt_decode_mpi(unencoded_m_buf, sizeof(unencoded_m_buf),
			enc_m, secret);
	if (n < 1) {
		ERRP(&stream->cbdata, pkt, "decrypted message too short");
		return 0;
	}

	/* PKA */
	pkt.u.pk_sesskey.symm_alg = unencoded_m_buf[0];

	if (!__ops_is_sa_supported(pkt.u.pk_sesskey.symm_alg)) {
		/* ERR1P */
		OPS_ERROR_1(&stream->errors,
			OPS_E_ALG_UNSUPPORTED_SYMMETRIC_ALG,
			"Symmetric algorithm %s not supported",
			findtype((int)pkt.u.pk_sesskey.symm_alg, symm_alg_map));
		return 0;
	}
	k = __ops_key_size(pkt.u.pk_sesskey.symm_alg);

	if ((unsigned) n != k + 3) {
		OPS_ERROR_2(&stream->errors,
			OPS_E_PROTO_DECRYPTED_MSG_WRONG_LEN,
			"decrypted message wrong length (got %d expected %d)",
			n, k + 3);
		return 0;
	}
	if (k > sizeof(pkt.u.pk_sesskey.key)) {
		loggit("parse_pk_sesskey: bad keylength");
		return 0;
	}

	(void) memcpy(pkt.u.pk_sesskey.key, unencoded_m_buf + 1, k);

	pkt.u.pk_sesskey.checksum = unencoded_m_buf[k + 1] +
			(unencoded_m_buf[k + 2] << 8);

	/* Check checksum */
	__ops_calc_sesskey_checksum(&pkt.u.pk_sesskey, cs);
	if (unencoded_m_buf[k + 1] != cs[0] ||
	    unencoded_m_buf[k + 2] != cs[1]) {
		OPS_ERROR_4(&stream->errors, OPS_E_PROTO_BAD_SK_CHECKSUM,
		"Session key checksum wrong: expected %2x %2x, got %2x %2x",
		cs[0], cs[1], unencoded_m_buf[k + 1],
		unencoded_m_buf[k + 2]);
		return 0;
	}
	/* all is well */
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_PK_SESSION_KEY, &pkt);

	__ops_crypt_any(&stream->decrypt, pkt.u.pk_sesskey.symm_alg);
	iv = __ops_new(stream->decrypt.blocksize);
	stream->decrypt.set_iv(&stream->decrypt, iv);
	stream->decrypt.set_key(&stream->decrypt, pkt.u.pk_sesskey.key);
	__ops_encrypt_init(&stream->decrypt);
	(void) free(iv);
	return 1;
}

static int 
__ops_decrypt_se_data(__ops_content_tag_t tag, __ops_region_t *region,
		    __ops_stream_t *stream)
{
	__ops_crypt_t	*decrypt;
	const int	 printerrors = 1;
	int		 r = 1;

	if (stream->decrypt.alg) {
		__ops_region_t	encregion;
		uint8_t		buf[OPS_MAX_BLOCK_SIZE + 2] = "";
		size_t          blksize;

		decrypt = &stream->decrypt;
		blksize = decrypt->blocksize;
		__ops_reader_push_decrypt(stream, decrypt, region);
		__ops_init_subregion(&encregion, NULL);
		encregion.length = blksize + 2;
		if (!exact_limread(buf, blksize + 2, &encregion, stream)) {
			return 0;
		}
		if (buf[blksize - 2] != buf[blksize] ||
		    buf[blksize - 1] != buf[blksize + 1]) {
			__ops_reader_pop_decrypt(stream);
			OPS_ERROR_4(&stream->errors,
				OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT,
				"Bad symmetric decrypt (%02x%02x vs %02x%02x)",
				buf[blksize - 2], buf[blksize - 1],
				buf[blksize], buf[blksize + 1]);
			return 0;
		}
		if (tag == OPS_PTAG_CT_SE_DATA_BODY) {
			decrypt->decrypt_resync(decrypt);
			decrypt->block_encrypt(decrypt, decrypt->civ,
					decrypt->civ);
		}
		r = __ops_parse(stream, !printerrors);
		__ops_reader_pop_decrypt(stream);
	} else {
		__ops_packet_t pkt;

		while (region->readc < region->length) {
			unsigned        len;

			len = region->length - region->readc;
			if (len > sizeof(pkt.u.se_data_body.data))
				len = sizeof(pkt.u.se_data_body.data);

			if (!limread(pkt.u.se_data_body.data, len,
					region, stream)) {
				return 0;
			}
			pkt.u.se_data_body.length = len;
			CALLBACK(&stream->cbdata, tag, &pkt);
		}
	}
	return r;
}

static void __ops_reader_push_se_ip_data(__ops_stream_t *,
				__ops_crypt_t *,
				__ops_region_t *);
static void __ops_reader_pop_se_ip_data(__ops_stream_t *);

static int 
__ops_decrypt_se_ip_data(__ops_content_tag_t tag, __ops_region_t *region,
		       __ops_stream_t *stream)
{
	__ops_crypt_t	*decrypt;
	const int	 printerrors = 1;
	int		 r = 1;

	if (stream->decrypt.alg) {
		decrypt = &stream->decrypt;
		__ops_reader_push_decrypt(stream, decrypt, region);
		__ops_reader_push_se_ip_data(stream, decrypt, region);
		r = __ops_parse(stream, !printerrors);
		__ops_reader_pop_se_ip_data(stream);
		__ops_reader_pop_decrypt(stream);
	} else {
		__ops_packet_t pkt;

		while (region->readc < region->length) {
			unsigned        len;

			len = region->length - region->readc;
			if (len > sizeof(pkt.u.se_data_body.data)) {
				len = sizeof(pkt.u.se_data_body.data);
			}
			if (!limread(pkt.u.se_data_body.data,
					len, region, stream)) {
				return 0;
			}
			pkt.u.se_data_body.length = len;
			CALLBACK(&stream->cbdata, tag, &pkt);
		}
	}
	return r;
}

/**
   \ingroup Core_ReadPackets
   \brief Read a Symmetrically Encrypted packet
*/
static int 
parse_se_data(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_packet_t pkt;

	/* there's no info to go with this, so just announce it */
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_SE_DATA_HEADER, &pkt);

	/*
	 * The content of an encrypted data packet is more OpenPGP packets
	 * once decrypted, so recursively handle them
	 */
	return __ops_decrypt_se_data(OPS_PTAG_CT_SE_DATA_BODY, region, stream);
}

/**
   \ingroup Core_ReadPackets
   \brief Read a Symmetrically Encrypted Integrity Protected packet
*/
static int 
parse_se_ip_data(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_packet_t	pkt;
	uint8_t   c = 0x0;

	if (!limread(&c, 1, region, stream)) {
		return 0;
	}
	pkt.u.se_ip_data_header.version = c;

	if (pkt.u.se_ip_data_header.version != OPS_SE_IP_V1) {
		loggit("parse_se_ip_data: bad version");
		return 0;
	}

	/*
	 * The content of an encrypted data packet is more OpenPGP packets
	 * once decrypted, so recursively handle them
	 */
	return __ops_decrypt_se_ip_data(OPS_PTAG_CT_SE_IP_DATA_BODY, region,
			stream);
}

/**
   \ingroup Core_ReadPackets
   \brief Read a MDC packet
*/
static int 
parse_mdc(__ops_region_t *region, __ops_stream_t *stream)
{
	__ops_packet_t pkt;

	pkt.u.mdc.length = OPS_SHA1_HASH_SIZE;
	pkt.u.mdc.data = __ops_new(OPS_SHA1_HASH_SIZE);
	if (!limread(pkt.u.mdc.data, OPS_SHA1_HASH_SIZE, region, stream)) {
		return 0;
	}
	CALLBACK(&stream->cbdata, OPS_PTAG_CT_MDC, &pkt);
	(void) free(pkt.u.mdc.data);
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse one packet.
 *
 * This function parses the packet tag.  It computes the value of the
 * content tag and then calls the appropriate function to handle the
 * content.
 *
 * \param *stream	How to parse
 * \param *pktlen	On return, will contain number of bytes in packet
 * \return 1 on success, 0 on error, -1 on EOF */
static int 
__ops_parse_packet(__ops_stream_t *stream, unsigned long *pktlen)
{
	__ops_packet_t	pkt;
	__ops_region_t	region;
	uint8_t	ptag;
	unsigned	indeterm = 0;
	int		ret;

	pkt.u.ptag.position = stream->readinfo.position;

	ret = sub_base_read(&ptag, 1, &stream->errors, &stream->readinfo,
				     &stream->cbdata);

	/* errors in the base read are effectively EOF. */
	if (ret <= 0) {
		return -1;
	}

	*pktlen = 0;

	if (!(ptag & OPS_PTAG_ALWAYS_SET)) {
		pkt.u.error.error = "Format error (ptag bit not set)";
		CALLBACK(&stream->cbdata, OPS_PARSER_ERROR, &pkt);
		return 0;
	}
	pkt.u.ptag.new_format = !!(ptag & OPS_PTAG_NEW_FORMAT);
	if (pkt.u.ptag.new_format) {
		pkt.u.ptag.type = ptag & OPS_PTAG_NF_CONTENT_TAG_MASK;
		pkt.u.ptag.lentype = 0;
		if (!read_new_length(&pkt.u.ptag.length, stream)) {
			return 0;
		}
	} else {
		unsigned   rb;

		rb = 0;
		pkt.u.ptag.type = ((unsigned)ptag &
				OPS_PTAG_OF_CONTENT_TAG_MASK)
			>> OPS_PTAG_OF_CONTENT_TAG_SHIFT;
		pkt.u.ptag.lentype = ptag & OPS_PTAG_OF_LENGTH_TYPE_MASK;
		switch (pkt.u.ptag.lentype) {
		case OPS_PTAG_OLD_LEN_1:
			rb = read_scalar(&pkt.u.ptag.length, 1, stream);
			break;

		case OPS_PTAG_OLD_LEN_2:
			rb = read_scalar(&pkt.u.ptag.length, 2, stream);
			break;

		case OPS_PTAG_OLD_LEN_4:
			rb = read_scalar(&pkt.u.ptag.length, 4, stream);
			break;

		case OPS_PTAG_OLD_LEN_INDETERMINATE:
			pkt.u.ptag.length = 0;
			indeterm = 1;
			rb = 1;
			break;
		}
		if (!rb) {
			return 0;
		}
	}

	CALLBACK(&stream->cbdata, OPS_PARSER_PTAG, &pkt);

	__ops_init_subregion(&region, NULL);
	region.length = pkt.u.ptag.length;
	region.indeterm = indeterm;
	switch (pkt.u.ptag.type) {
	case OPS_PTAG_CT_SIGNATURE:
		ret = parse_sig(&region, stream);
		break;

	case OPS_PTAG_CT_PUBLIC_KEY:
	case OPS_PTAG_CT_PUBLIC_SUBKEY:
		ret = parse_pubkey(pkt.u.ptag.type, &region, stream);
		break;

	case OPS_PTAG_CT_TRUST:
		ret = parse_trust(&region, stream);
		break;

	case OPS_PTAG_CT_USER_ID:
		ret = parse_userid(&region, stream);
		break;

	case OPS_PTAG_CT_COMPRESSED:
		ret = parse_compressed(&region, stream);
		break;

	case OPS_PTAG_CT_1_PASS_SIG:
		ret = parse_one_pass(&region, stream);
		break;

	case OPS_PTAG_CT_LITERAL_DATA:
		ret = parse_litdata(&region, stream);
		break;

	case OPS_PTAG_CT_USER_ATTR:
		ret = parse_userattr(&region, stream);
		break;

	case OPS_PTAG_CT_SECRET_KEY:
		ret = parse_seckey(&region, stream);
		break;

	case OPS_PTAG_CT_SECRET_SUBKEY:
		ret = parse_seckey(&region, stream);
		break;

	case OPS_PTAG_CT_PK_SESSION_KEY:
		ret = parse_pk_sesskey(&region, stream);
		break;

	case OPS_PTAG_CT_SE_DATA:
		ret = parse_se_data(&region, stream);
		break;

	case OPS_PTAG_CT_SE_IP_DATA:
		ret = parse_se_ip_data(&region, stream);
		break;

	case OPS_PTAG_CT_MDC:
		ret = parse_mdc(&region, stream);
		break;

	default:
		OPS_ERROR_1(&stream->errors, OPS_E_P_UNKNOWN_TAG,
			    "Unknown content tag 0x%x",
			    pkt.u.ptag.type);
		ret = 0;
	}

	/* Ensure that the entire packet has been consumed */

	if (region.length != region.readc && !region.indeterm) {
		if (!consume_packet(&region, stream, 0)) {
			ret = -1;
		}
	}

	/* also consume it if there's been an error? */
	/* \todo decide what to do about an error on an */
	/* indeterminate packet */
	if (ret == 0) {
		if (!consume_packet(&region, stream, 0)) {
			ret = -1;
		}
	}
	/* set pktlen */

	*pktlen = stream->readinfo.alength;

	/* do callback on entire packet, if desired and there was no error */

	if (ret > 0 && stream->readinfo.accumulate) {
		pkt.u.packet.length = stream->readinfo.alength;
		pkt.u.packet.raw = stream->readinfo.accumulated;
		stream->readinfo.accumulated = NULL;
		stream->readinfo.asize = 0;
		CALLBACK(&stream->cbdata, OPS_PARSER_PACKET_END, &pkt);
	}
	stream->readinfo.alength = 0;

	return (ret < 0) ? -1 : (ret) ? 1 : 0;
}

/**
 * \ingroup Core_ReadPackets
 *
 * \brief Parse packets from an input stream until EOF or error.
 *
 * \details Setup the necessary parsing configuration in "stream"
 * before calling __ops_parse().
 *
 * That information includes :
 *
 * - a "reader" function to be used to get the data to be parsed
 *
 * - a "callback" function to be called when this library has identified
 * a parseable object within the data
 *
 * - whether the calling function wants the signature subpackets
 * returned raw, parsed or not at all.
 *
 * After returning, stream->errors holds any errors encountered while parsing.
 *
 * \param stream	Parsing configuration
 * \return		1 on success in all packets, 0 on error in any packet
 *
 * \sa CoreAPI Overview
 *
 * \sa __ops_print_errors()
 *
 */

static int 
__ops_parse(__ops_stream_t *stream, int perrors)
{
	unsigned long   pktlen;
	int             r;

	do {
		r = __ops_parse_packet(stream, &pktlen);
	} while (r != -1);
	if (perrors) {
		__ops_print_errors(stream->errors);
	}
	return (stream->errors == NULL);
}


/**************************************************************************/

#define CRC24_POLY 0x1864cfbL

/**
 * \struct dearmour_t
 */
typedef struct {
	enum {
		OUTSIDE_BLOCK = 0,
		BASE64,
		AT_TRAILER_NAME
	} state;
	enum {
		NONE = 0,
		BEGIN_PGP_MESSAGE,
		BEGIN_PGP_PUBLIC_KEY_BLOCK,
		BEGIN_PGP_PRIVATE_KEY_BLOCK,
		BEGIN_PGP_MULTI,
		BEGIN_PGP_SIGNATURE,

		END_PGP_MESSAGE,
		END_PGP_PUBLIC_KEY_BLOCK,
		END_PGP_PRIVATE_KEY_BLOCK,
		END_PGP_MULTI,
		END_PGP_SIGNATURE,

		BEGIN_PGP_SIGNED_MESSAGE
	} lastseen;
	__ops_stream_t *parse_info;
	unsigned	seen_nl:1;
	unsigned	prev_nl:1;
	unsigned	can_runon_data:1;
			/* !< allow headers in armoured data that are
			* not separated from the data by a blank line
			* */
	unsigned	no_gap_needed:1;
			/* !< allow no blank line at the start of
			* armoured data */
	unsigned	can_trail_ws:1;
			/* !< allow armoured stuff to have trailing
			* whitespace where we wouldn't strictly expect
			* it */
	/* it is an error to get a cleartext message without a sig */
	unsigned   	expect_sig:1;
	unsigned   	got_sig:1;
	/* base64 stuff */
	unsigned        buffered;
	uint8_t   buffer[3];
	unsigned	eof64;
	unsigned long   checksum;
	unsigned long   read_checksum;
	/* unarmoured text blocks */
	uint8_t   unarmoured[NETPGP_BUFSIZ];
	size_t          unarmoredc;
	/* pushed back data (stored backwards) */
	uint8_t  *pushback;
	unsigned        pushbackc;
	/* armoured block headers */
	__ops_headers_t	headers;
} dearmour_t;

static void 
push_back(dearmour_t *dearmour, const uint8_t *buf,
	  unsigned length)
{
	unsigned        n;

	if (dearmour->pushback) {
		loggit("push_back: already pushed back");
	} else {
		dearmour->pushback = __ops_new(length);
		for (n = 0; n < length; ++n) {
			dearmour->pushback[n] = buf[length - n - 1];
		}
		dearmour->pushbackc = length;
	}
}

/* this struct holds a textual header line */
typedef struct headerline_t {
	const char	*s;		/* the header line */
	size_t		 len;		/* its length */
	int		 type;		/* the defined type */
} headerline_t;

static headerline_t	headerlines[] = {
	{ "BEGIN PGP MESSAGE",		17, BEGIN_PGP_MESSAGE },
	{ "BEGIN PGP PUBLIC KEY BLOCK",	26, BEGIN_PGP_PUBLIC_KEY_BLOCK },
	{ "BEGIN PGP PRIVATE KEY BLOCK",27, BEGIN_PGP_PRIVATE_KEY_BLOCK },
	{ "BEGIN PGP MESSAGE, PART ",	25, BEGIN_PGP_MULTI },
	{ "BEGIN PGP SIGNATURE",	19, BEGIN_PGP_SIGNATURE },

	{ "END PGP MESSAGE",		15, END_PGP_MESSAGE },
	{ "END PGP PUBLIC KEY BLOCK",	24, END_PGP_PUBLIC_KEY_BLOCK },
	{ "END PGP PRIVATE KEY BLOCK",	25, END_PGP_PRIVATE_KEY_BLOCK },
	{ "END PGP MESSAGE, PART ",	22, END_PGP_MULTI },
	{ "END PGP SIGNATURE",		17, END_PGP_SIGNATURE },

	{ "BEGIN PGP SIGNED MESSAGE",	24, BEGIN_PGP_SIGNED_MESSAGE },

	{ NULL,				0, -1	}
};

/* search through the table of header lines */
static int
findheaderline(char *headerline)
{
	headerline_t	*hp;

	for (hp = headerlines ; hp->s ; hp++) {
		if (strncmp(headerline, hp->s, hp->len) == 0) {
			break;
		}
	}
	return hp->type;
}

static int 
set_lastseen_headerline(dearmour_t *dearmour, char *hdr, __ops_error_t **errors)
{
	int	lastseen;
	int	prev;

	prev = dearmour->lastseen;
	if ((lastseen = findheaderline(hdr)) == -1) {
		OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
			"Unrecognised Header Line %s", hdr);
		return 0;
	}
	dearmour->lastseen = lastseen;
	switch (dearmour->lastseen) {
	case NONE:
		OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
			"Unrecognised last seen Header Line %s", hdr);
		break;

	case END_PGP_MESSAGE:
		if (prev != BEGIN_PGP_MESSAGE) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
				"Got END PGP MESSAGE, but not after BEGIN");
		}
		break;

	case END_PGP_PUBLIC_KEY_BLOCK:
		if (prev != BEGIN_PGP_PUBLIC_KEY_BLOCK) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
			"Got END PGP PUBLIC KEY BLOCK, but not after BEGIN");
		}
		break;

	case END_PGP_PRIVATE_KEY_BLOCK:
		if (prev != BEGIN_PGP_PRIVATE_KEY_BLOCK) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
			"Got END PGP PRIVATE KEY BLOCK, but not after BEGIN");
		}
		break;

	case BEGIN_PGP_MULTI:
	case END_PGP_MULTI:
		OPS_ERROR(errors, OPS_E_R_UNSUPPORTED,
			"Multi-part messages are not yet supported");
		break;

	case END_PGP_SIGNATURE:
		if (prev != BEGIN_PGP_SIGNATURE) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
			"Got END PGP SIGNATURE, but not after BEGIN");
		}
		break;

	case BEGIN_PGP_MESSAGE:
	case BEGIN_PGP_PUBLIC_KEY_BLOCK:
	case BEGIN_PGP_PRIVATE_KEY_BLOCK:
	case BEGIN_PGP_SIGNATURE:
	case BEGIN_PGP_SIGNED_MESSAGE:
		break;
	}
	return 1;
}

static int 
read_char(dearmour_t *dearmour,
		__ops_error_t **errors,
		__ops_reader_t *readinfo,
		__ops_cbdata_t *cbdata,
		unsigned skip)
{
	uint8_t   c;

	do {
		if (dearmour->pushbackc) {
			c = dearmour->pushback[--dearmour->pushbackc];
			if (dearmour->pushbackc == 0) {
				(void) free(dearmour->pushback);
				dearmour->pushback = NULL;
			}
		} else if (__ops_stacked_read(&c, 1, errors, readinfo,
					cbdata) != 1) {
			return -1;
		}
	} while (skip && c == '\r');
	dearmour->prev_nl = dearmour->seen_nl;
	dearmour->seen_nl = (c == '\n');
	return c;
}

static int 
eat_whitespace(int first,
	       dearmour_t *dearmour,
	       __ops_error_t **errors,
	       __ops_reader_t *readinfo,
	       __ops_cbdata_t *cbdata,
	       unsigned skip)
{
	int             c = first;

	while (c == ' ' || c == '\t') {
		c = read_char(dearmour, errors, readinfo, cbdata, skip);
	}
	return c;
}

static int 
read_and_eat_whitespace(dearmour_t *dearmour,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_cbdata_t *cbdata,
			unsigned skip)
{
	int             c;

	do {
		c = read_char(dearmour, errors, readinfo, cbdata, skip);
	} while (c == ' ' || c == '\t');
	return c;
}

static void 
flush(dearmour_t *dearmour, __ops_cbdata_t *cbdata)
{
	__ops_packet_t	content;

	if (dearmour->unarmoredc > 0) {
		content.u.unarmoured_text.data = dearmour->unarmoured;
		content.u.unarmoured_text.length = dearmour->unarmoredc;
		CALLBACK(cbdata, OPS_PTAG_CT_UNARMOURED_TEXT, &content);
		dearmour->unarmoredc = 0;
	}
}

static int 
unarmoured_read_char(dearmour_t *dearmour,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_cbdata_t *cbdata,
			unsigned skip)
{
	int             c;

	do {
		c = read_char(dearmour, errors, readinfo, cbdata, 0);
		if (c < 0) {
			return c;
		}
		dearmour->unarmoured[dearmour->unarmoredc++] = c;
		if (dearmour->unarmoredc == sizeof(dearmour->unarmoured)) {
			flush(dearmour, cbdata);
		}
	} while (skip && c == '\r');
	return c;
}

/**
 * \param headers
 * \param key
 *
 * \return header value if found, otherwise NULL
 */
static const char *
__ops_find_header(__ops_headers_t *headers, const char *key)
{
	unsigned        n;

	for (n = 0; n < headers->headerc; ++n) {
		if (strcmp(headers->headers[n].key, key) == 0) {
			return headers->headers[n].value;
		}
	}
	return NULL;
}

/**
 * \param dest
 * \param src
 */
static void 
__ops_dup_headers(__ops_headers_t *dest, const __ops_headers_t *src)
{
	unsigned        n;

	dest->headers = __ops_new(src->headerc * sizeof(*dest->headers));
	dest->headerc = src->headerc;
	for (n = 0; n < src->headerc; ++n) {
		dest->headers[n].key = strdup(src->headers[n].key);
		dest->headers[n].value = strdup(src->headers[n].value);
	}
}

/*
 * Note that this skips CRs so implementations always see just straight LFs
 * as line terminators
 */
static int 
process_dash_escaped(dearmour_t *dearmour,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_cbdata_t *cbdata)
{
	__ops_packet_t content;
	__ops_packet_t content2;
	__ops_cleartext_body_t *body = &content.u.cleartext_body;
	__ops_cleartext_trailer_t *trailer = &content2.u.cleartext_trailer;
	const char     *hashstr;
	__ops_hash_t     *hash;
	int             total;

	hash = __ops_new(sizeof(*hash));
	hashstr = __ops_find_header(&dearmour->headers, "Hash");
	if (hashstr) {
		__ops_hash_alg_t alg;

		alg = __ops_str_to_hash_alg(hashstr);
		if (!__ops_is_hash_alg_supported(&alg)) {
			(void) free(hash);
			OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
				"Unsupported hash algorithm '%s'", hashstr);
			return -1;
		}
		if (alg == OPS_HASH_UNKNOWN) {
			(void) free(hash);
			OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
				"Unknown hash algorithm '%s'", hashstr);
			return -1;
		}
		__ops_hash_any(hash, alg);
	} else {
		__ops_hash_md5(hash);
	}

	hash->init(hash);

	body->length = 0;
	total = 0;
	for (;;) {
		int             c;
		unsigned        count;

		c = read_char(dearmour, errors, readinfo, cbdata, 1);
		if (c < 0) {
			return -1;
		}
		if (dearmour->prev_nl && c == '-') {
			if ((c = read_char(dearmour, errors, readinfo, cbdata,
						0)) < 0) {
				return -1;
			}
			if (c != ' ') {
				/* then this had better be a trailer! */
				if (c != '-') {
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"Bad dash-escaping");
				}
				for (count = 2; count < 5; ++count) {
					if ((c = read_char(dearmour, errors,
						readinfo, cbdata, 0)) < 0) {
						return -1;
					}
					if (c != '-') {
						OPS_ERROR(errors,
						OPS_E_R_BAD_FORMAT,
						"Bad dash-escaping (2)");
					}
				}
				dearmour->state = AT_TRAILER_NAME;
				break;
			}
			/* otherwise we read the next character */
			if ((c = read_char(dearmour, errors, readinfo, cbdata,
						0)) < 0) {
				return -1;
			}
		}
		if (c == '\n' && body->length) {
			if (memchr(body->data + 1, '\n', body->length - 1)
						!= NULL) {
				loggit("process_dash_escaped: newline found");
				return -1;
			}
			if (body->data[0] == '\n') {
				hash->add(hash, (const uint8_t *)"\r", 1);
			}
			hash->add(hash, body->data, body->length);
			CALLBACK(cbdata, OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY,
						&content);
			body->length = 0;
		}
		body->data[body->length++] = c;
		total += 1;
		if (body->length == sizeof(body->data)) {
			CALLBACK(cbdata, OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY,
					&content);
			body->length = 0;
		}
	}
	if (body->data[0] != '\n') {
		loggit("process_dash_escaped: no newline in body data");
		return -1;
	}
	if (body->length != 1) {
		loggit("process_dash_escaped: bad body length");
		return -1;
	}

	/* don't send that one character, because it's part of the trailer */
	trailer->hash = hash;
	CALLBACK(cbdata, OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER, &content2);
	return total;
}

static int 
add_header(dearmour_t *dearmour, const char *key, const char *value)
{
	int	n;

	/*
         * Check that the header is valid
         */
	if (strcmp(key, "Version") == 0 ||
	    strcmp(key, "Comment") == 0 ||
	    strcmp(key, "MessageID") == 0 ||
	    strcmp(key, "Hash") == 0 ||
	    strcmp(key, "Charset") == 0) {
		n = dearmour->headers.headerc;
		dearmour->headers.headers = realloc(dearmour->headers.headers,
				(n + 1) * sizeof(*dearmour->headers.headers));
		dearmour->headers.headers[n].key = strdup(key);
		dearmour->headers.headers[n].value = strdup(value);
		dearmour->headers.headerc = n + 1;
		return 1;
	}
	return 0;
}

/* \todo what does a return value of 0 indicate? 1 is good, -1 is bad */
static int 
parse_headers(dearmour_t *dearmour, __ops_error_t **errors,
	      __ops_reader_t * readinfo, __ops_cbdata_t * cbdata)
{
	unsigned        nbuf;
	unsigned        size;
	unsigned	first = 1;
	char           *buf;
	int             ret = 1;

	buf = NULL;
	nbuf = size = 0;

	for (;;) {
		int             c;

		c = read_char(dearmour, errors, readinfo, cbdata, 1);
		if (c < 0) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "Unexpected EOF");
			ret = -1;
			break;
		}
		if (c == '\n') {
			char           *s;

			if (nbuf == 0) {
				break;
			}
			if (nbuf >= size) {
				loggit("parse_headers: bad size");
				return -1;
			}
			buf[nbuf] = '\0';
			if ((s = strchr(buf, ':')) == NULL) {
				if (!first && !dearmour->can_runon_data) {
					/*
					 * then we have seriously malformed
					 * armour
					 */
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"No colon in armour header");
					ret = -1;
					break;
				} else {
					if (first &&
					    !(dearmour->can_runon_data ||
					      dearmour->no_gap_needed)) {
						OPS_ERROR(errors,
							OPS_E_R_BAD_FORMAT,
					"No colon in armour header (2)");
						/*
						 * then we have a nasty
						 * armoured block with no
						 * headers, not even a blank
						 * line.
						 */
						buf[nbuf] = '\n';
						push_back(dearmour,
							(uint8_t *) buf,
							nbuf + 1);
						ret = -1;
						break;
					}
				}
			} else {
				*s = '\0';
				if (s[1] != ' ') {
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"No space in armour header");
					(void) free(buf);
					return -1;
				}
				if (!add_header(dearmour, buf, s + 2)) {
					OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
						"Invalid header %s", buf);
					(void) free(buf);
					return -1;
				}
				nbuf = 0;
			}
			first = 0;
		} else {
			if (size <= nbuf + 1) {
				size += size + 80;
				buf = realloc(buf, size);
			}
			buf[nbuf++] = c;
		}
	}
	(void) free(buf);
	return ret;
}

static int 
read4(dearmour_t *dearmour, __ops_error_t **errors,
      __ops_reader_t *readinfo, __ops_cbdata_t *cbdata,
      int *pc, unsigned *pn, unsigned long *pl)
{
	int             n, c;
	unsigned long   l = 0;

	for (n = 0; n < 4; ++n) {
		c = read_char(dearmour, errors, readinfo, cbdata, 1);
		if (c < 0) {
			dearmour->eof64 = 1;
			return -1;
		}
		if (c == '-' || c == '=') {
			break;
		}
		l <<= 6;
		if (c >= 'A' && c <= 'Z') {
			l += c - 'A';
		} else if (c >= 'a' && c <= 'z') {
			l += c - 'a' + 26;
		} else if (c >= '0' && c <= '9') {
			l += c - '0' + 52;
		} else if (c == '+') {
			l += 62;
		} else if (c == '/') {
			l += 63;
		} else {
			--n;
			l >>= 6;
		}
	}

	*pc = c;
	*pn = n;
	*pl = l;

	return 4;
}

static unsigned 
__ops_crc24(unsigned checksum, uint8_t c)
{
	unsigned        i;

	checksum ^= c << 16;
	for (i = 0; i < 8; i++) {
		checksum <<= 1;
		if (checksum & 0x1000000)
			checksum ^= CRC24_POLY;
	}
	return checksum & 0xffffffL;
}

static int 
decode64(dearmour_t *dearmour, __ops_error_t **errors,
	 __ops_reader_t *readinfo, __ops_cbdata_t *cbdata)
{
	unsigned long   l;
	unsigned        n;
	int             n2;
	int             c;
	int             ret;

	if (dearmour->buffered) {
		loggit("decode64: bad dearmour->buffered");
		return 0;
	}

	ret = read4(dearmour, errors, readinfo, cbdata, &c, &n, &l);
	if (ret < 0) {
		OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "Badly formed base64");
		return 0;
	}
	if (n == 3) {
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64 (2)");
			return 0;
		}
		dearmour->buffered = 2;
		dearmour->eof64 = 1;
		l >>= 2;
	} else if (n == 2) {
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64 (3)");
			return 0;
		}
		dearmour->buffered = 1;
		dearmour->eof64 = 1;
		l >>= 4;
		c = read_char(dearmour, errors, readinfo, cbdata, 0);
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64");
			return 0;
		}
	} else if (n == 0) {
		if (!dearmour->prev_nl || c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64 (4)");
			return 0;
		}
		dearmour->buffered = 0;
	} else {
		if (n != 4) {
			loggit("decode64: bad n (!= 4)");
			return 0;
		}
		dearmour->buffered = 3;
		if (c == '-' || c == '=') {
			loggit("decode64: bad c");
			return 0;
		}
	}

	if (dearmour->buffered < 3 && dearmour->buffered > 0) {
		/* then we saw padding */
		if (c != '=') {
			loggit("decode64: bad c (=)");
			return 0;
		}
		c = read_and_eat_whitespace(dearmour, errors, readinfo, cbdata,
				1);
		if (c != '\n') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
				"No newline at base64 end");
			return 0;
		}
		c = read_char(dearmour, errors, readinfo, cbdata, 0);
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
				"No checksum at base64 end");
			return 0;
		}
	}
	if (c == '=') {
		/* now we are at the checksum */
		ret = read4(dearmour, errors, readinfo, cbdata, &c, &n,
				&dearmour->read_checksum);
		if (ret < 0 || n != 4) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Error in checksum");
			return 0;
		}
		c = read_char(dearmour, errors, readinfo, cbdata, 1);
		if (dearmour->can_trail_ws)
			c = eat_whitespace(c, dearmour, errors, readinfo, cbdata,
					1);
		if (c != '\n') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated checksum");
			return 0;
		}
		c = read_char(dearmour, errors, readinfo, cbdata, 0);
		if (c != '-') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Bad base64 trailer (2)");
			return 0;
		}
	}
	if (c == '-') {
		for (n = 0; n < 4; ++n)
			if (read_char(dearmour, errors, readinfo, cbdata,
						0) != '-') {
				OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"Bad base64 trailer");
				return 0;
			}
		dearmour->eof64 = 1;
	} else {
		if (!dearmour->buffered) {
			loggit("decode64: not buffered");
			return 0;
		}
	}

	for (n = 0; n < dearmour->buffered; ++n) {
		dearmour->buffer[n] = (uint8_t)l;
		l >>= 8;
	}

	for (n2 = dearmour->buffered - 1; n2 >= 0; --n2)
		dearmour->checksum = __ops_crc24((unsigned)dearmour->checksum,
					dearmour->buffer[n2]);

	if (dearmour->eof64 && dearmour->read_checksum != dearmour->checksum) {
		OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "Checksum mismatch");
		return 0;
	}
	return 1;
}

static void 
base64(dearmour_t *dearmour)
{
	dearmour->state = BASE64;
	dearmour->checksum = CRC24_INIT;
	dearmour->eof64 = 0;
	dearmour->buffered = 0;
}

/* This reader is rather strange in that it can generate callbacks for */
/* content - this is because plaintext is not encapsulated in PGP */
/* packets... it also calls back for the text between the blocks. */

static int 
armoured_data_reader(void *dest_, size_t length, __ops_error_t **errors,
		     __ops_reader_t *readinfo,
		     __ops_cbdata_t *cbdata)
{
	dearmour_t *dearmour = readinfo->arg;
	__ops_packet_t content;
	int             ret;
	unsigned   first;
	uint8_t  *dest = dest_;
	int             saved = length;

	if (dearmour->eof64 && !dearmour->buffered) {
		if (dearmour->state != OUTSIDE_BLOCK &&
		    dearmour->state != AT_TRAILER_NAME) {
			loggit("armoured_data_reader: bad dearmour state");
			return 0;
		}
	}

	while (length > 0) {
		unsigned        count;
		unsigned        n;
		char            buf[1024];
		int             c;

		flush(dearmour, cbdata);
		switch (dearmour->state) {
		case OUTSIDE_BLOCK:
			/*
			 * This code returns EOF rather than EARLY_EOF
			 * because if we don't see a header line at all, then
			 * it is just an EOF (and not a BLOCK_END)
			 */
			while (!dearmour->seen_nl) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbdata, 1)) < 0) {
					return 0;
				}
			}

			/*
			 * flush at this point so we definitely have room for
			 * the header, and so we can easily erase it from the
			 * buffer
			 */
			flush(dearmour, cbdata);
			/* Find and consume the 5 leading '-' */
			for (count = 0; count < 5; ++count) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbdata, 0)) < 0) {
					return 0;
				}
				if (c != '-') {
					goto reloop;
				}
			}

			/* Now find the block type */
			for (n = 0; n < sizeof(buf) - 1;) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbdata, 0)) < 0) {
					return 0;
				}
				if (c == '-') {
					goto got_minus;
				}
				buf[n++] = c;
			}
			/* then I guess this wasn't a proper header */
			break;

got_minus:
			buf[n] = '\0';

			/* Consume trailing '-' */
			for (count = 1; count < 5; ++count) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbdata, 0)) < 0) {
					return 0;
				}
				if (c != '-') {
					/* wasn't a header after all */
					goto reloop;
				}
			}

			/* Consume final NL */
			if ((c = unarmoured_read_char(dearmour, errors, readinfo,
						cbdata, 1)) < 0) {
				return 0;
			}
			if (dearmour->can_trail_ws) {
				if ((c = eat_whitespace(c, dearmour, errors,
						readinfo, cbdata, 1)) < 0) {
					return 0;
				}
			}
			if (c != '\n') {
				/* wasn't a header line after all */
				break;
			}

			/*
			 * Now we've seen the header, scrub it from the
			 * buffer
			 */
			dearmour->unarmoredc = 0;

			/*
			 * But now we've seen a header line, then errors are
			 * EARLY_EOF
			 */
			if ((ret = parse_headers(dearmour, errors, readinfo,
					cbdata)) <= 0) {
				return -1;
			}

			if (!set_lastseen_headerline(dearmour, buf, errors)) {
				return -1;
			}

			if (strcmp(buf, "BEGIN PGP SIGNED MESSAGE") == 0) {
				__ops_dup_headers(
					&content.u.cleartext_head.headers,
					&dearmour->headers);
				CALLBACK(cbdata,
					OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER,
					&content);
				ret = process_dash_escaped(dearmour, errors,
						readinfo, cbdata);
				if (ret <= 0) {
					return ret;
				}
			} else {
				content.u.armour_header.type = buf;
				content.u.armour_header.headers =
						dearmour->headers;
				(void) memset(&dearmour->headers, 0x0,
						sizeof(dearmour->headers));
				CALLBACK(cbdata, OPS_PTAG_CT_ARMOUR_HEADER,
						&content);
				base64(dearmour);
			}
			break;

		case BASE64:
			first = 1;
			while (length > 0) {
				if (!dearmour->buffered) {
					if (!dearmour->eof64) {
						ret = decode64(dearmour,
							errors, readinfo, cbdata);
						if (ret <= 0) {
							return ret;
						}
					}
					if (!dearmour->buffered) {
						if (!dearmour->eof64) {
							loggit(
"armoured_data_reader: bad dearmour eof64");
							return 0;
						}
						if (first) {
							dearmour->state =
								AT_TRAILER_NAME;
							goto reloop;
						}
						return -1;
					}
				}
				if (!dearmour->buffered) {
					loggit(
			"armoured_data_reader: bad dearmour buffered");
					return 0;
				}
				*dest = dearmour->buffer[--dearmour->buffered];
				++dest;
				--length;
				first = 0;
			}
			if (dearmour->eof64 && !dearmour->buffered) {
				dearmour->state = AT_TRAILER_NAME;
			}
			break;

		case AT_TRAILER_NAME:
			for (n = 0; n < sizeof(buf) - 1;) {
				if ((c = read_char(dearmour, errors, readinfo,
						cbdata, 0)) < 0) {
					return -1;
				}
				if (c == '-') {
					goto got_minus2;
				}
				buf[n++] = c;
			}
			/* then I guess this wasn't a proper trailer */
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Bad ASCII armour trailer");
			break;

got_minus2:
			buf[n] = '\0';

			if (!set_lastseen_headerline(dearmour, buf, errors)) {
				return -1;
			}

			/* Consume trailing '-' */
			for (count = 1; count < 5; ++count) {
				if ((c = read_char(dearmour, errors, readinfo,
						cbdata, 0)) < 0) {
					return -1;
				}
				if (c != '-') {
					/* wasn't a trailer after all */
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"Bad ASCII armour trailer (2)");
				}
			}

			/* Consume final NL */
			if ((c = read_char(dearmour, errors, readinfo, cbdata,
						1)) < 0) {
				return -1;
			}
			if (dearmour->can_trail_ws) {
				if ((c = eat_whitespace(c, dearmour, errors,
						readinfo, cbdata, 1)) < 0) {
					return 0;
				}
			}
			if (c != '\n') {
				/* wasn't a trailer line after all */
				OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Bad ASCII armour trailer (3)");
			}

			if (strncmp(buf, "BEGIN ", 6) == 0) {
				if (!set_lastseen_headerline(dearmour, buf,
						errors)) {
					return -1;
				}
				if ((ret = parse_headers(dearmour, errors,
						readinfo, cbdata)) <= 0) {
					return ret;
				}
				content.u.armour_header.type = buf;
				content.u.armour_header.headers =
						dearmour->headers;
				(void) memset(&dearmour->headers, 0x0,
						sizeof(dearmour->headers));
				CALLBACK(cbdata, OPS_PTAG_CT_ARMOUR_HEADER,
						&content);
				base64(dearmour);
			} else {
				content.u.armour_trailer.type = buf;
				CALLBACK(cbdata, OPS_PTAG_CT_ARMOUR_TRAILER,
						&content);
				dearmour->state = OUTSIDE_BLOCK;
			}
			break;
		}
reloop:
		continue;
	}

	return saved;
}

static void 
armoured_data_destroyer(__ops_reader_t *readinfo)
{
	(void) free(readinfo->arg);
}

/**
 * \ingroup Core_Readers_Armour
 * \brief Pushes dearmouring reader onto stack
 * \param parse_info Usual structure containing information about to how to do the parse
 * \sa __ops_reader_pop_dearmour()
 */
static void 
__ops_reader_push_dearmour(__ops_stream_t *parse_info)
/*
 * This function originally had these parameters to cater for packets which
 * didn't strictly match the RFC. The initial 0.5 release is only going to
 * support strict checking. If it becomes desirable to support loose checking
 * of armoured packets and these params are reinstated, parse_headers() must
 * be fixed so that these flags work correctly.
 * 
 * // Allow headers in armoured data that are not separated from the data by a
 * blank line unsigned without_gap,
 * 
 * // Allow no blank line at the start of armoured data unsigned no_gap,
 * 
 * //Allow armoured data to have trailing whitespace where we strictly would not
 * expect it			      unsigned trailing_whitespace
 */
{
	dearmour_t *dearmour;

	dearmour = __ops_new(sizeof(*dearmour));
	dearmour->seen_nl = 1;
	/*
	    dearmour->can_runon_data=without_gap;
	    dearmour->no_gap_needed=no_gap;
	    dearmour->can_trail_ws=trailing_whitespace;
	*/
	dearmour->expect_sig = 0;
	dearmour->got_sig = 0;

	__ops_reader_push(parse_info, armoured_data_reader,
			armoured_data_destroyer, dearmour);
}

/**
 * \ingroup Core_Readers_Armour
 * \brief Pops dearmour reader from stock
 * \param stream
 * \sa __ops_reader_push_dearmour()
 */
static void 
__ops_reader_pop_dearmour(__ops_stream_t *stream)
{
	dearmour_t *dearmour;

	dearmour = stream->readinfo.arg;
	(void) free(dearmour);
	__ops_reader_pop(stream);
}

/**************************************************************************/

/* this is actually used for *decrypting* */
typedef struct {
	uint8_t	 v[1024 * 15];
	size_t		 c;
	size_t		 off;
	__ops_crypt_t	*decrypt;
	__ops_region_t	*region;
	unsigned	 prevplain:1;
} encrypted_t;

static int 
encrypted_reader(void *dest,
			size_t length,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_cbdata_t *cbdata)
{
	encrypted_t	*enc;
	char		*cdest;
	int		 saved;

	enc = readinfo->arg;
	saved = length;
	/*
	 * V3 MPIs have the count plain and the cipher is reset after each
	 * count
	 */
	if (enc->prevplain && !readinfo->parent->reading_mpi_len) {
		if (!readinfo->parent->reading_v3_secret) {
			loggit("encrypted_reader: bad v3 secret");
			return -1;
		}
		enc->decrypt->decrypt_resync(enc->decrypt);
		enc->prevplain = 0;
	} else if (readinfo->parent->reading_v3_secret &&
		   readinfo->parent->reading_mpi_len) {
		enc->prevplain = 1;
	}
	while (length > 0) {
		if (enc->c) {
			unsigned        n;

			/*
			 * if we are reading v3 we should never read
			 * more than we're asked for */
			if (length < enc->c &&
			     (readinfo->parent->reading_v3_secret ||
			      readinfo->parent->exact_read)) {
				loggit("encrypted_reader: bad v3 read");
				return 0;
			}
			n = MIN(length, enc->c);
			(void) memcpy(dest, enc->v + enc->off, n);
			enc->c -= n;
			enc->off += n;
			length -= n;
			cdest = dest;
			cdest += n;
			dest = cdest;
		} else {
			unsigned        n = enc->region->length;
			uint8_t   buf[1024];

			if (!n) {
				return -1;
			}
			if (!enc->region->indeterm) {
				n -= enc->region->readc;
				if (n == 0) {
					return saved - length;
				}
				if (n > sizeof(buf)) {
					n = sizeof(buf);
				}
			} else {
				n = sizeof(buf);
			}

			/*
			 * we can only read as much as we're asked for
			 * in v3 keys because they're partially
			 * unencrypted!  */
			if ((readinfo->parent->reading_v3_secret ||
			     readinfo->parent->exact_read) && n > length) {
				n = length;
			}

			if (!__ops_stacked_limited_read(buf, n,
				enc->region, errors, readinfo, cbdata)) {
				return -1;
			}
			if (!readinfo->parent->reading_v3_secret ||
			    !readinfo->parent->reading_mpi_len) {
				enc->c = __ops_decrypt_se_ip(enc->decrypt,
						enc->v, buf, n);

			} else {
				(void) memcpy(&enc->v[enc->off], buf, n);
				enc->c = n;
			}

			if (enc->c == 0) {
				loggit("encrypted_reader: 0 decrypted count");
				return 0;
			}

			enc->off = 0;
		}
	}

	return saved;
}

static void 
nuke_encrypted(__ops_reader_t *readinfo)
{
	(void) free(readinfo->arg);
}

/**
 * \ingroup Core_Readers_SE
 * \brief Pushes decryption reader onto stack
 * \sa __ops_reader_pop_decrypt()
 */
static void 
__ops_reader_push_decrypt(__ops_stream_t *stream, __ops_crypt_t *decrypt,
			__ops_region_t *region)
{
	encrypted_t	*enc;
	
	enc = __ops_new(sizeof(*enc));
	enc->decrypt = decrypt;
	enc->region = region;
	__ops_decrypt_init(enc->decrypt);
	__ops_reader_push(stream, encrypted_reader, nuke_encrypted, enc);
}

/**
 * \ingroup Core_Readers_Encrypted
 * \brief Pops decryption reader from stack
 * \sa __ops_reader_push_decrypt()
 */
static void 
__ops_reader_pop_decrypt(__ops_stream_t *stream)
{
	encrypted_t	*enc;

	enc = stream->readinfo.arg;
	enc->decrypt->decrypt_finish(enc->decrypt);
	(void) free(enc);
	__ops_reader_pop(stream);
}

/**************************************************************************/

typedef struct {
	/* boolean: 0 once we've done the preamble/MDC checks */
	/* and are reading from the plaintext */
	int             passed_checks;
	uint8_t  *plaintext;
	size_t          plaintext_available;
	size_t          plaintext_offset;
	__ops_region_t   *region;
	__ops_crypt_t    *decrypt;
} decrypt_se_ip_t;

/*
  Gets entire SE_IP data packet.
  Verifies leading preamble
  Verifies trailing MDC packet
  Then passes up plaintext as requested
*/
static int 
se_ip_data_reader(void *dest_,
			size_t len,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_cbdata_t *cbdata)
{
	decrypt_se_ip_t	*se_ip;
	__ops_region_t	 decrypted_region;
	unsigned int	 n = 0;

	se_ip = readinfo->arg;
	if (!se_ip->passed_checks) {
		uint8_t  *buf = NULL;
		__ops_hash_t      hash;
		uint8_t   hashed[OPS_SHA1_HASH_SIZE];
		size_t          b;
		size_t          sz_preamble;
		size_t          sz_mdc_hash;
		size_t          sz_mdc;
		size_t          sz_plaintext;
		uint8_t  *preamble;
		uint8_t  *plaintext;
		uint8_t  *mdc;
		uint8_t  *mdc_hash;

		__ops_hash_any(&hash, OPS_HASH_SHA1);
		hash.init(&hash);

		__ops_init_subregion(&decrypted_region, NULL);
		decrypted_region.length =
			se_ip->region->length - se_ip->region->readc;
		buf = __ops_new(decrypted_region.length);

		/* read entire SE IP packet */
		if (!__ops_stacked_limited_read(buf, decrypted_region.length,
				&decrypted_region, errors, readinfo, cbdata)) {
			(void) free(buf);
			return -1;
		}
		/* verify leading preamble */

		b = se_ip->decrypt->blocksize;
		if (buf[b - 2] != buf[b] || buf[b - 1] != buf[b + 1]) {
			loggit(
			"Bad symmetric decrypt (%02x%02x vs %02x%02x)",
				buf[b - 2], buf[b - 1], buf[b], buf[b + 1]);
			OPS_ERROR(errors, OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT,
			"Bad symmetric decrypt when parsing SE IP packet");
			(void) free(buf);
			return -1;
		}
		/* Verify trailing MDC hash */

		sz_preamble = se_ip->decrypt->blocksize + 2;
		sz_mdc_hash = OPS_SHA1_HASH_SIZE;
		sz_mdc = 1 + 1 + sz_mdc_hash;
		sz_plaintext = decrypted_region.length - sz_preamble - sz_mdc;

		preamble = buf;
		plaintext = buf + sz_preamble;
		mdc = plaintext + sz_plaintext;
		mdc_hash = mdc + 2;

		__ops_calc_mdc_hash(preamble, sz_preamble, plaintext,
				sz_plaintext, hashed);

		if (memcmp(mdc_hash, hashed, OPS_SHA1_HASH_SIZE) != 0) {
			OPS_ERROR(errors, OPS_E_V_BAD_HASH,
					"Bad hash in MDC packet");
			(void) free(buf);
			return 0;
		}
		/* all done with the checks */
		/* now can start reading from the plaintext */
		if (se_ip->plaintext) {
			loggit("se_ip_data_reader: bad plaintext");
			return 0;
		}
		se_ip->plaintext = __ops_new(sz_plaintext);
		memcpy(se_ip->plaintext, plaintext, sz_plaintext);
		se_ip->plaintext_available = sz_plaintext;

		se_ip->passed_checks = 1;

		(void) free(buf);
	}
	n = len;
	if (n > se_ip->plaintext_available) {
		n = se_ip->plaintext_available;
	}

	memcpy(dest_, se_ip->plaintext + se_ip->plaintext_offset, n);
	se_ip->plaintext_available -= n;
	se_ip->plaintext_offset += n;
	len -= n;

	return n;
}

static void 
se_ip_data_destroyer(__ops_reader_t *readinfo)
{
	decrypt_se_ip_t	*se_ip;

	se_ip = readinfo->arg;
	(void) free(se_ip->plaintext);
	(void) free(se_ip);
}

/**
   \ingroup Internal_Readers_SEIP
*/
static void 
__ops_reader_push_se_ip_data(__ops_stream_t *stream, __ops_crypt_t *decrypt,
			   __ops_region_t * region)
{
	decrypt_se_ip_t *se_ip = __ops_new(sizeof(*se_ip));

	se_ip->region = region;
	se_ip->decrypt = decrypt;
	__ops_reader_push(stream, se_ip_data_reader, se_ip_data_destroyer,
				se_ip);
}

/**
   \ingroup Internal_Readers_SEIP
 */
static void 
__ops_reader_pop_se_ip_data(__ops_stream_t *stream)
{
	decrypt_se_ip_t *se_ip;

	se_ip = stream->readinfo.arg;
	(void) free(se_ip);
	__ops_reader_pop(stream);
}

/**************************************************************************/

/** Arguments for reader_fd
 */
typedef struct mmap_reader_t {
	void		*mem;		/* memory mapped file */
	uint64_t	 size;		/* size of file */
	uint64_t	 offset;	/* current offset in file */
	int		 fd;		/* file descriptor */
} mmap_reader_t;


/**
 * \ingroup Core_Readers
 *
 * __ops_reader_fd() attempts to read up to "plength" bytes from the file
 * descriptor in "parse_info" into the buffer starting at "dest" using the
 * rules contained in "flags"
 *
 * \param	dest	Pointer to previously allocated buffer
 * \param	plength Number of bytes to try to read
 * \param	flags	Rules about reading to use
 * \param	readinfo	Reader info
 * \param	cbdata	Callback info
 *
 * \return	n	Number of bytes read
 *
 * OPS_R_EARLY_EOF and OPS_R_ERROR push errors on the stack
 */
static int 
fd_reader(void *dest, size_t length, __ops_error_t **errors,
	  __ops_reader_t *readinfo, __ops_cbdata_t *cbdata)
{
	mmap_reader_t	*reader;
	int              n;

	__OPS_USED(cbdata);
	reader = readinfo->arg;
	if ((n = read(reader->fd, dest, length)) == 0) {
		return 0;
	}
	if (n < 0) {
		OPS_SYSTEM_ERROR_1(errors, OPS_E_R_READ_FAILED, "read",
				   "file descriptor %d", reader->fd);
		return -1;
	}
	return n;
}

static void 
reader_fd_destroyer(__ops_reader_t *readinfo)
{
	(void) free(readinfo->arg);
}

/**************************************************************************/

typedef struct {
	const uint8_t	*buffer;
	size_t			 length;
	size_t			 offset;
} reader_mem_t;

static int 
mem_reader(void *dest, size_t length, __ops_error_t **errors,
	   __ops_reader_t *readinfo, __ops_cbdata_t *cbdata)
{
	reader_mem_t *reader = readinfo->arg;
	unsigned        n;

	__OPS_USED(cbdata);
	__OPS_USED(errors);

	if (reader->offset + length > reader->length) {
		n = reader->length - reader->offset;
	} else {
		n = length;
	}
	if (n == 0) {
		return 0;
	}
	(void) memcpy(dest, reader->buffer + reader->offset, n);
	reader->offset += n;
	return n;
}

static void 
mem_destroyer(__ops_reader_t *readinfo)
{
	(void) free(readinfo->arg);
}

/**
   \ingroup Core_Readers_First
   \brief Starts stack with memory reader
*/

static void 
__ops_reader_set_memory(__ops_stream_t *stream, const void *buffer,
		      size_t length)
{
	reader_mem_t *mem = __ops_new(sizeof(*mem));

	mem->buffer = buffer;
	mem->length = length;
	mem->offset = 0;
	__ops_reader_set(stream, mem_reader, mem_destroyer, mem);
}

/* --- */

/* File manipulation here */

/**************************************************************************/


/**
   \ingroup Core_Readers
   \brief Create parse_info and sets to read from memory
   \param stream Address where new parse_info will be set
   \param mem Memory to read from
   \param arg Reader-specific arg
   \param callback Callback to use with reader
   \param accumulate Set if we need to accumulate as we read. (Usually 0 unless doing signature verification)
   \note It is the caller's responsiblity to free parse_info
   \sa __ops_teardown_memory_read()
*/
static void 
__ops_setup_memory_read(__ops_io_t *io,
			__ops_stream_t **stream,
			__ops_memory_t *mem,
			void *vp,
			__ops_cb_ret_t callback(__ops_cbdata_t *,
						const __ops_packet_t *),
			unsigned accumulate)
{
	*stream = __ops_new(sizeof(*stream));
	(*stream)->io = (*stream)->cbdata.io = io;
	__ops_set_callback(*stream, callback, vp);
	__ops_reader_set_memory(*stream, mem->buf, mem->length);
	if (accumulate) {
		(*stream)->readinfo.accumulate = 1;
	}
}

/**
   \ingroup Core_Readers
   \brief Frees stream and mem
   \param stream
   \param mem
   \sa __ops_setup_memory_read()
*/
static void 
__ops_teardown_memory_read(__ops_stream_t *stream, __ops_memory_t *mem)
{
	__ops_stream_delete(stream);
	__ops_memory_free(mem);
}

/* read memory from the previously mmap-ed file */
static int 
mmap_reader(void *dest, size_t length, __ops_error_t **errors,
	  __ops_reader_t *readinfo, __ops_cbdata_t *cbdata)
{
	mmap_reader_t	*mem;
	unsigned	 n;
	char		*cmem;

	__OPS_USED(errors);
	__OPS_USED(cbdata);
	mem = readinfo->arg;
	cmem = mem->mem;
	n = MIN(length, (unsigned)(mem->size - mem->offset));
	if (n > 0) {
		(void) memcpy(dest, &cmem[(int)mem->offset], (unsigned)n);
		mem->offset += n;
	}
	return n;
}

/* tear down the mmap, close the fd */
static void 
mmap_destroyer(__ops_reader_t *readinfo)
{
	mmap_reader_t *mem;

	mem = readinfo->arg;
	(void) munmap(mem->mem, (unsigned)mem->size);
	(void) close(mem->fd);
	(void) free(mem);
}

/* set up the file to use mmap-ed memory if available, file IO otherwise */
static void 
__ops_reader_set_mmap(__ops_stream_t *stream, int fd)
{
	mmap_reader_t	*mem = __ops_new(sizeof(*mem));
	struct stat	 st;

	if (fstat(fd, &st) == 0) {
		mem->size = st.st_size;
		mem->offset = 0;
		mem->fd = fd;
		mem->mem = mmap(NULL, (size_t)st.st_size, PROT_READ,
				MAP_FILE | MAP_PRIVATE, fd, 0);
		if (mem->mem == MAP_FAILED) {
			__ops_reader_set(stream, fd_reader, reader_fd_destroyer,
					mem);
		} else {
			__ops_reader_set(stream, mmap_reader, mmap_destroyer,
					mem);
		}
	}
}

/* small function to pretty print an 8-character raw userid */
static char    *
userid_to_id(const uint8_t *userid, char *id)
{
	static const char *hexes = "0123456789abcdef";
	int		   i;

	for (i = 0; i < 8 ; i++) {
		id[i * 2] = hexes[(unsigned)(userid[i] & 0xf0) >> 4];
		id[(i * 2) + 1] = hexes[userid[i] & 0xf];
	}
	id[8 * 2] = 0x0;
	return id;
}

static void __ops_print_pubkeydata(__ops_io_t *, const __ops_key_t *);

/* print out the successful signature information */
static void
resultp(__ops_io_t *io,
	const char *f,
	__ops_validation_t *res,
	__ops_keyring_t *ring)
{
	const __ops_key_t	*pubkey;
	unsigned		 i;
	char			 id[MAX_ID_LENGTH + 1];

	for (i = 0; i < res->validc; i++) {
		loggit("Good signature for %s made %susing %s key %s",
			f,
			ctime(&res->valids[i].birthtime),
			findtype((int) res->valids[i].key_alg, pubkeyalgs),
			userid_to_id(res->valids[i].signer, id));
		pubkey = __ops_getkeybyid(ring,
			(const uint8_t *) res->valids[i].signer);
		__ops_print_pubkeydata(io, pubkey);
	}
}

/* static functions */

static void 
ptime(FILE *fp, time_t t)
{
	struct tm      *tm;

	tm = gmtime(&t);
	printf("%04d-%02d-%02d",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday);
}

/* return the number of bits in the public key */
static int
numkeybits(const __ops_pubkey_t *pubkey)
{
	switch(pubkey->alg) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		return BN_num_bytes(pubkey->key.rsa.n) * 8;
	case OPS_PKA_DSA:
		switch(BN_num_bytes(pubkey->key.dsa.q)) {
		case 20:
			return 1024;
		case 28:
			return 2048;
		case 32:
			return 3072;
		default:
			return 0;
		}
	case OPS_PKA_ELGAMAL:
		return BN_num_bytes(pubkey->key.elgamal.y) * 8;
	default:
		return -1;
	}
}

static void 
hexdump(FILE *fp, const uint8_t *src, size_t length, const char *sep)
{
	unsigned i;

	for (i = 0 ; i < length ; i += 2) {
		printf("%02x", *src++);
		printf("%02x%s", *src++, sep);
	}
}

/**
   \ingroup Core_Print

   Prints a public key in succinct detail

   \param key Ptr to public key
*/
static void
__ops_print_pubkeydata(__ops_io_t *io, const __ops_key_t *key)
{
	unsigned int    i;

	printf("pub %d/%s ",
		numkeybits(&key->key.pubkey),
		findtype((int) key->key.pubkey.alg, pubkeyalgs));
	hexdump(io->errs, key->keyid, OPS_KEY_ID_SIZE, "");
	printf(" ");
	ptime(io->errs, key->key.pubkey.birthtime);
	printf("\nKey fingerprint: ");
	hexdump(io->errs, key->fingerprint.fingerprint, OPS_FINGERPRINT_SIZE,
		" ");
	printf("\n");
	for (i = 0; i < key->uidc; i++) {
		printf("uid              %s\n",
			key->uids[i].userid);
	}
}

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

/**
   \ingroup Core_Readers
   \brief Creates parse_info, opens file, and sets to read from file
   \param stream Address where new parse_info will be set
   \param filename Name of file to read
   \param vp Reader-specific arg
   \param callback Callback to use when reading
   \param accumulate Set if we need to accumulate as we read. (Usually 0 unless doing signature verification)
   \note It is the caller's responsiblity to free parse_info and to close fd
   \sa __ops_teardown_file_read()
*/
static int 
__ops_setup_file_read(__ops_io_t *io,
		__ops_stream_t **stream,
		const char *filename,
		void *vp,
		__ops_cb_ret_t callback(__ops_cbdata_t *,
					const __ops_packet_t *),
		unsigned accumulate)
{
	int	fd;

#ifdef O_BINARY
	fd = open(filename, O_RDONLY | O_BINARY);
#else
	fd = open(filename, O_RDONLY);
#endif
	if (fd < 0) {
		loggit("can't open \"%s\"", filename);
		return fd;
	}
	*stream = __ops_new(sizeof(*stream));
	(*stream)->io = (*stream)->cbdata.io = io;
	__ops_set_callback(*stream, callback, vp);
	__ops_reader_set_mmap(*stream, fd);
	if (accumulate) {
		(*stream)->readinfo.accumulate = 1;
	}
	return fd;
}

/**
   \ingroup Core_Readers
   \brief Frees stream and closes fd
   \param stream
   \param fd
   \sa __ops_setup_file_read()
*/
static void 
__ops_teardown_file_read(__ops_stream_t *stream, int fd)
{
	(void) close(fd);
	__ops_stream_delete(stream);
}

/**************************************************************************/

/* \todo check where pkt pointers are copied */
static __ops_cb_ret_t
cb_keyring_read(__ops_cbdata_t *cbdata, const __ops_packet_t *pkt)
{
	__OPS_USED(cbdata);

	switch (pkt->tag) {
	case OPS_PARSER_PTAG:
	case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY:	/* we get these because we
						 * didn't prompt */
	case OPS_PTAG_CT_SIGNATURE_HEADER:
	case OPS_PTAG_CT_SIGNATURE_FOOTER:
	case OPS_PTAG_CT_SIGNATURE:
	case OPS_PTAG_CT_TRUST:
	case OPS_PARSER_ERRCODE:
		break;

	default:
		break;
	}

	return OPS_RELEASE_MEMORY;
}

/**
   \ingroup HighLevel_KeyringRead

   \brief Reads a keyring from a file

   \param keyring Pointer to an existing __ops_keyring_t struct
   \param armour 1 if file is armoured; else 0
   \param filename Filename of keyring to be read

   \return __ops 1 if OK; 0 on error

   \note Keyring struct must already exist.

   \note Can be used with either a public or secret keyring.

   \note You must call __ops_keyring_free() after usage to free alloc-ed memory.

   \note If you call this twice on the same keyring struct, without calling
   __ops_keyring_free() between these calls, you will introduce a memory leak.

   \sa __ops_keyring_read_from_mem()
   \sa __ops_keyring_free()

*/
static unsigned 
__ops_keyring_fileread(__ops_keyring_t *keyring,
			const unsigned armour,
			const char *filename)
{
	__ops_stream_t	*stream;
	unsigned	 ret = 1;
	int		 fd;

	stream = __ops_new(sizeof(*stream));
	/* add this for the moment, */
	/*
	 * \todo need to fix the problems with reading signature subpackets
	 * later
	 */
	__ops_stream_options(stream, OPS_PTAG_SS_ALL, OPS_PARSE_PARSED);
#ifdef O_BINARY
	fd = open(filename, O_RDONLY | O_BINARY);
#else
	fd = open(filename, O_RDONLY);
#endif
	if (fd < 0) {
		__ops_stream_delete(stream);
		perror(filename);
		return 0;
	}
	__ops_reader_set_mmap(stream, fd);
	__ops_set_callback(stream, cb_keyring_read, NULL);
	if (armour) {
		__ops_reader_push_dearmour(stream);
	}
	ret = __ops_parse_and_accumulate(keyring, stream);
	__ops_print_errors(stream->errors);
	if (armour) {
		__ops_reader_pop_dearmour(stream);
	}
	(void) close(fd);
	__ops_stream_delete(stream);
	return ret;
}

/**
   \ingroup HighLevel_KeyringRead

   \brief Frees keyring's contents (but not keyring itself)

   \param keyring Keyring whose data is to be freed

   \note This does not free keyring itself, just the memory alloc-ed in it.
 */
static void 
__ops_keyring_free(__ops_keyring_t *keyring)
{
	(void)free(keyring->keys);
	keyring->keys = NULL;
	keyring->keyc = 0;
	keyring->keysize = 0;
}

/* -- */

#include <regex.h>

/* check there's enough space in the arrays */
static void
size_arrays(netpgp_t *netpgp, unsigned needed)
{
	if (netpgp->size == 0) {
		/* only get here first time around */
		netpgp->size = needed;
		netpgp->name = __ops_new(sizeof(char *) * needed);
		netpgp->value = __ops_new(sizeof(char *) * needed);
	} else if (netpgp->c == netpgp->size) {
		/* only uses 'needed' when filled array */
		netpgp->size += needed;
		netpgp->name = realloc(netpgp->name, sizeof(char *) * needed);
		netpgp->value = realloc(netpgp->value, sizeof(char *) * needed);
	}
}

/* find the name in the array */
static int
findvar(netpgp_t *netpgp, const char *name)
{
	unsigned	i;

	for (i = 0 ; i < netpgp->c && strcmp(netpgp->name[i], name) != 0; i++) {
	}
	return (i == netpgp->c) ? -1 : (int)i;
}

/* read a keyring and return it */
static void *
readkeyring(netpgp_t *netpgp, const char *name)
{
	__ops_keyring_t	*keyring;
	const unsigned	 noarmor = 0;
	char		 f[MAXPATHLEN];
	char		*filename;
	char		*homedir;

	homedir = netpgp_getvar(netpgp, "homedir");
	if ((filename = netpgp_getvar(netpgp, name)) == NULL) {
		(void) snprintf(f, sizeof(f), "%s/%s.gpg", homedir, name);
		filename = f;
	}
	keyring = __ops_new(sizeof(*keyring));
	if (!__ops_keyring_fileread(keyring, noarmor, filename)) {
		loggit("Can't read %s %s", name, filename);
		return NULL;
	}
	netpgp_setvar(netpgp, name, filename);
	return keyring;
}

/* read any gpg config file */
static int
conffile(netpgp_t *netpgp, char *homedir, char *userid, size_t length)
{
	regmatch_t	 matchv[10];
	regex_t		 keyre;
	char		 buf[BUFSIZ];
	FILE		*fp;

	__OPS_USED(netpgp);
	(void) snprintf(buf, sizeof(buf), "%s/gpg.conf", homedir);
	if ((fp = fopen(buf, "r")) == NULL) {
		loggit("conffile: can't open '%s'", buf);
		return 0;
	}
	(void) memset(&keyre, 0x0, sizeof(keyre));
	(void) regcomp(&keyre, "^[ \t]*default-key[ \t]+([0-9a-zA-F]+)",
		REG_EXTENDED);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (regexec(&keyre, buf, 10, matchv, 0) == 0) {
			(void) memcpy(userid, &buf[(int)matchv[1].rm_so],
				MIN((unsigned)(matchv[1].rm_eo -
						matchv[1].rm_so), length));
			loggit("netpgp: default key set to \"%.*s\"",
				(int)(matchv[1].rm_eo - matchv[1].rm_so),
				&buf[(int)matchv[1].rm_so]);
		}
	}
	(void) fclose(fp);
	return 1;
}

/**
   \ingroup HighLevel_Verify
   \brief Verifies the signatures in a signed file
   \param result Where to put the result
   \param filename Name of file to be validated
   \param armoured Treat file as armoured, if set
   \param keyring Keyring to use
   \return 1 if signatures validate successfully;
   	0 if signatures fail or there are no signatures
   \note After verification, result holds the details of all keys which
   have passed, failed and not been recognised.
   \note It is the caller's responsiblity to call
   	__ops_validate_result_free(result) after use.
*/
static unsigned 
__ops_validate_file(__ops_io_t *io,
			__ops_validation_t *result,
			const char *infile,
			const char *outfile,
			const int armoured,
			const __ops_keyring_t *keyring)
{
	validate_data_cb_t	 validation;
	__ops_stream_t		*parse = NULL;
	struct stat		 st;
	const int		 printerrors = 1;
	unsigned		 ret;
	int64_t		 	 sigsize;
	char			 origfile[MAXPATHLEN];
	char			*detachname;
	int			 outfd = 0;
	int			 infd;
	int			 cc;

#define SIG_OVERHEAD	284 /* XXX - depends on sig size? */

	if (stat(infile, &st) < 0) {
		loggit("can't validate \"%s\"", infile);
		return 0;
	}
	sigsize = st.st_size;
	detachname = NULL;
	cc = snprintf(origfile, sizeof(origfile), "%s", infile);
	if (strcmp(&origfile[cc - 4], ".sig") == 0) {
		origfile[cc - 4] = 0x0;
		if (stat(origfile, &st) == 0 &&
		    st.st_size > sigsize - SIG_OVERHEAD) {
			detachname = strdup(origfile);
		}
	}

	(void) memset(&validation, 0x0, sizeof(validation));

	infd = __ops_setup_file_read(io, &parse, infile, &validation,
				validate_data_cb, 1);
	if (infd < 0) {
		return 0;
	}

	validation.detachname = detachname;

	/* Set verification reader and handling options */
	validation.result = result;
	validation.keyring = keyring;
	validation.mem = __ops_new(sizeof(*validation.mem));
	__ops_memory_init(validation.mem, 128);
	/* Note: Coverity incorrectly reports an error that validation.reader */
	/* is never used. */
	validation.reader = parse->readinfo.arg;

	if (armoured) {
		__ops_reader_push_dearmour(parse);
	}

	/* Do the verification */
	__ops_parse(parse, !printerrors);

	/* Tidy up */
	if (armoured) {
		__ops_reader_pop_dearmour(parse);
	}
	__ops_teardown_file_read(parse, infd);

	ret = validate_result_status(result);

	/* this is triggered only for --cat output */
	if (outfile) {
		/* need to send validated output somewhere */
		if (strcmp(outfile, "-") == 0) {
			outfd = STDOUT_FILENO;
		} else {
			outfd = open(outfile, O_WRONLY | O_CREAT, 0666);
		}
		if (outfd < 0) {
			/* even if the signature was good, we can't
			* write the file, so send back a bad return
			* code */
			ret = 0;
		} else if (validate_result_status(result)) {
			uint8_t	*cp;
			unsigned	 len;
			int		 i;

			len = validation.mem->length;
			cp = validation.mem->buf;
			for (i = 0 ; i < (int)len ; i += cc) {
				cc = write(outfd, &cp[i], len - i);
				if (cc < 0) {
					loggit("netpgp: short write");
					ret = 0;
					break;
				}
			}
			if (strcmp(outfile, "-") != 0) {
				(void) close(outfd);
			}
		}
	}
	__ops_memory_free(validation.mem);
	return ret;
}

/**
   \ingroup HighLevel_Verify
   \brief Verifies the signatures in a __ops_memory_t struct
   \param result Where to put the result
   \param mem Memory to be validated
   \param armoured Treat data as armoured, if set
   \param keyring Keyring to use
   \return 1 if signature validates successfully; 0 if not
   \note After verification, result holds the details of all keys which
   have passed, failed and not been recognised.
   \note It is the caller's responsiblity to call
   	__ops_validate_result_free(result) after use.
*/

static unsigned 
__ops_validate_mem(__ops_io_t *io,
			__ops_validation_t *result,
			__ops_memory_t *mem,
			const int armoured,
			const __ops_keyring_t *keyring)
{
	validate_data_cb_t	 validation;
	__ops_stream_t		*stream = NULL;
	const unsigned		 accumulate = 1;
	const int		 printerrors = 1;

	__ops_setup_memory_read(io, &stream, mem, &validation,
				validate_data_cb, accumulate);
	/* Set verification reader and handling options */
	(void) memset(&validation, 0x0, sizeof(validation));
	validation.result = result;
	validation.keyring = keyring;
	validation.mem = __ops_new(sizeof(*validation.mem));
	__ops_memory_init(validation.mem, 128);
	/* Note: Coverity incorrectly reports an error that validation.reader */
	/* is never used. */
	validation.reader = stream->readinfo.arg;
	if (armoured) {
		__ops_reader_push_dearmour(stream);
	}
	/* Do the verification */
	__ops_parse(stream, !printerrors);
	/* Tidy up */
	if (armoured) {
		__ops_reader_pop_dearmour(stream);
	}
	__ops_teardown_memory_read(stream, mem);
	__ops_memory_free(validation.mem);
	return validate_result_status(result);
}

/***************************************************************************/
/* exported functions start here */
/***************************************************************************/

/* verify a file */
int
netpgp_verify_file(netpgp_t *netpgp, const char *in, const char *out, int armored)
{
	__ops_validation_t	result;
	__ops_io_t		*io;

	(void) memset(&result, 0x0, sizeof(result));
	io = netpgp->io;
	if (__ops_validate_file(io, &result, in, out, armored,
						netpgp->pubring)) {
		resultp(io, in, &result, netpgp->pubring);
		return 1;
	}
	if (result.validc + result.invalidc + result.unknownc == 0) {
		loggit("\"%s\": No signatures found - is this a signed file?",
			in);
	} else {
		loggit(
"\"%s\": verification failure: %d invalid signatures, %d unknown signatures",
			in, result.invalidc, result.unknownc);
	}
	return 0;
}

/* verify memory */
int
netpgp_verify_mem(netpgp_t *netpgp, const char *in, size_t len, int armored)
{
	__ops_validation_t	result;
	__ops_memory_t		*mem;
	__ops_io_t		*io;

	(void) memset(&result, 0x0, sizeof(result));
	mem = __ops_new(sizeof(*mem));
	__ops_memory_init(mem, len);
	__ops_memory_add(mem, (const uint8_t *)in, len);
	io = netpgp->io;
	if (__ops_validate_mem(io, &result, mem, armored, netpgp->pubring)) {
		resultp(io, "", &result, netpgp->pubring);
		return 1;
	}
	if (result.validc + result.invalidc + result.unknownc == 0) {
		loggit("No signatures found - is this memory signed?");
	} else {
		loggit(
"verification failure: %d invalid signatures, %d unknown signatures",
			result.invalidc, result.unknownc);
	}
	__ops_memory_free(mem);
	return 0;
}

/* return the version for the library */
const char *
netpgp_get_info(const char *type)
{
	return __ops_get_info(type);
}


/* set a variable */
int
netpgp_setvar(netpgp_t *netpgp, const char *name, const char *value)
{
	int	i;

	if ((i = findvar(netpgp, name)) < 0) {
		/* add the element to the array */
		size_arrays(netpgp, netpgp->size + 15);
		netpgp->name[i = netpgp->c++] = strdup(name);
	} else {
		/* replace the element in the array */
		if (netpgp->value[i]) {
			(void) free(netpgp->value[i]);
			netpgp->value[i] = NULL;
		}
	}
	/* sanity checks for range of values */
	if (strcmp(name, "hash") == 0 || strcmp(name, "algorithm") == 0) {
		if (__ops_str_to_hash_alg(value) == OPS_HASH_UNKNOWN) {
			return 0;
		}
	}
	netpgp->value[i] = strdup(value);
	return 1;
}

/* get a variable's value (NULL if not set) */
char *
netpgp_getvar(netpgp_t *netpgp, const char *name)
{
	int	i;

	return ((i = findvar(netpgp, name)) < 0) ? NULL : netpgp->value[i];
}

/* initialise a netpgp_t structure */
int
netpgp_init(netpgp_t *netpgp)
{
	__ops_io_t	*io;
	char		 id[MAX_ID_LENGTH];
	char		*homedir;
	char		*userid;
	char		*stream;
	int		 coredumps;

#ifdef HAVE_SYS_RESOURCE_H
	struct rlimit	limit;

	coredumps = netpgp_getvar(netpgp, "coredumps") != NULL;
	if (!coredumps) {
		(void) memset(&limit, 0x0, sizeof(limit));
		if (setrlimit(RLIMIT_CORE, &limit) != 0) {
			loggit(
			"netpgp_init: warning - can't turn off core dumps");
			coredumps = 1;
		}
	}
#else
	coredumps = 1;
#endif
	io = __ops_new(sizeof(*io));
	io->outs = stdout;
	if ((stream = netpgp_getvar(netpgp, "stdout")) != NULL &&
	    strcmp(stream, "stderr") == 0) {
		io->outs = stderr;
	}
	io->errs = stderr;
	if ((stream = netpgp_getvar(netpgp, "stderr")) != NULL &&
	    strcmp(stream, "stdout") == 0) {
		io->errs = stdout;
	}
	io->errs = stderr;
	netpgp->io = io;
	if (coredumps) {
		loggit("netpgp: warning: core dumps enabled");
	}
	if ((homedir = netpgp_getvar(netpgp, "homedir")) == NULL) {
		loggit("netpgp: bad homedir");
		return 0;
	}
	if ((userid = netpgp_getvar(netpgp, "userid")) == NULL) {
		(void) memset(id, 0x0, sizeof(id));
		(void) conffile(netpgp, homedir, id, sizeof(id));
		if (id[0] != 0x0) {
			netpgp_setvar(netpgp, "userid", userid = id);
		}
	}
	if (userid == NULL) {
		if (netpgp_getvar(netpgp, "userid checks") == NULL) {
			loggit("Cannot find user id");
			return 0;
		}
		loggit("Skipping user id check");
	} else {
		(void) netpgp_setvar(netpgp, "userid", id);
	}
	if ((netpgp->pubring = readkeyring(netpgp, "pubring")) == NULL) {
		loggit("Can't read pub keyring");
		return 0;
	}
	if ((netpgp->secring = readkeyring(netpgp, "secring")) == NULL) {
		loggit("Can't read sec keyring");
		return 0;
	}
	return 1;
}

/* finish off with the netpgp_t struct */
int
netpgp_end(netpgp_t *netpgp)
{
	unsigned	i;

	for (i = 0 ; i < netpgp->c ; i++) {
		if (netpgp->name[i] != NULL) {
			(void) free(netpgp->name[i]);
		}
		if (netpgp->value[i] != NULL) {
			(void) free(netpgp->value[i]);
		}
	}
	if (netpgp->name != NULL) {
		(void) free(netpgp->name);
	}
	if (netpgp->value != NULL) {
		(void) free(netpgp->value);
	}
	if (netpgp->pubring != NULL) {
		__ops_keyring_free(netpgp->pubring);
	}
	if (netpgp->secring != NULL) {
		__ops_keyring_free(netpgp->secring);
	}
	(void) free(netpgp->io);
	return 1;
}

/* ----- */

#include <getopt.h>

/*
 * 2048 is the absolute minimum, really - we should really look at
 * bumping this to 4096 or even higher - agc, 20090522
 */
#define DEFAULT_NUMBITS 2048

/*
 * Similraily, SHA1 is now looking as though it should not be used.
 * Let's pre-empt this by specifying SHA256 - gpg interoperates just
 * fine with SHA256 - agc, 20090522
 */
#define DEFAULT_HASH_ALG	"SHA256"

static const char *usage =
	" --help OR\n"
	"\t--encrypt [--output=file] [options] files... OR\n"
	"\t--decrypt [--output=file] [options] files... OR\n\n"
	"\t--sign [--armor] [--detach] [--hash=alg] [--output=file]\n"
		"\t\t[options] files... OR\n"
	"\t--verify [options] files... OR\n"
	"\t--cat [--output=file] [options] files... OR\n"
	"\t--clearsign [--output=file] [options] files... OR\n\n"
	"\t--export-keys [options] OR\n"
	"\t--find-key [options] OR\n"
	"\t--generate-key [options] OR\n"
	"\t--import-key [options] OR\n"
	"\t--list-keys [options] OR\n\n"
	"\t--list-packets [options] OR\n"
	"\t--version\n"
	"where options are:\n"
	"\t[--coredumps] AND/OR\n"
	"\t[--homedir=<homedir>] AND/OR\n"
	"\t[--keyring=<keyring>] AND/OR\n"
	"\t[--userid=<userid>] AND/OR\n"
	"\t[--verbose]\n";

enum optdefs {
	/* commands */
	LIST_KEYS = 1,
	FIND_KEY,
	EXPORT_KEY,
	IMPORT_KEY,
	GENERATE_KEY,
	ENCRYPT,
	DECRYPT,
	SIGN,
	CLEARSIGN,
	VERIFY,
	VERIFY_CAT,
	LIST_PACKETS,
	VERSION_CMD,
	HELP_CMD,

	/* options */
	KEYRING,
	USERID,
	HOMEDIR,
	NUMBITS,
	DETACHED,
	HASH_ALG,
	OUTPUT,
	VERBOSE,
	COREDUMPS,

	/* debug */
	OPS_DEBUG

};

#define EXIT_ERROR	2

static struct option options[] = {
	/* debugging commands */
	{"help",	no_argument,		NULL,	HELP_CMD},
	{"version",	no_argument,		NULL,	VERSION_CMD},
	{"debug",	required_argument, 	NULL,	OPS_DEBUG},
	/* options */
	{"coredumps",	no_argument, 		NULL,	COREDUMPS},
	{"keyring",	required_argument, 	NULL,	KEYRING},
	{"userid",	required_argument, 	NULL,	USERID},
	{"home",	required_argument, 	NULL,	HOMEDIR},
	{"homedir",	required_argument, 	NULL,	HOMEDIR},
	{"verbose",	no_argument, 		NULL,	VERBOSE},
	{"output",	required_argument, 	NULL,	OUTPUT},
	{ NULL,		0,			NULL,	0},
};

/* gather up program variables into one struct */
typedef struct prog_t {
	char	 keyring[MAXPATHLEN + 1];	/* name of keyring */
	char	*progname;			/* program name */
	char	*output;			/* output file name */
	int	 overwrite;			/* overwrite files? */
	int	 numbits;			/* # of bits */
	int	 armour;			/* ASCII armor */
	int	 detached;			/* use separate file */
	int	 cmd;				/* netpgp command */
} prog_t;


/* print a usage message */
static void
print_usage(const char *usagemsg, char *progname)
{
	loggit("%s\nAll bug reports, praise and chocolate, please, to:\n%s",
				netpgp_get_info("version"),
				netpgp_get_info("maintainer"));
	loggit("Usage: %s COMMAND OPTIONS:\n%s %s",
		progname, progname, usagemsg);
}

/* get even more lippy */
static void
give_it_large(netpgp_t *netpgp)
{
	char	*cp;
	char	 num[16];
	int	 val;

	val = 0;
	if ((cp = netpgp_getvar(netpgp, "verbose")) != NULL) {
		val = atoi(cp);
	}
	(void) snprintf(num, sizeof(num), "%d", val + 1);
	netpgp_setvar(netpgp, "verbose", num);
}

/* set the home directory value to "home/subdir" */
static int
set_homedir(netpgp_t *netpgp, char *home, const char *subdir)
{
	struct stat	st;
	char		d[MAXPATHLEN];

	if (home == NULL) {
		loggit("NULL HOME directory");
		return 0;
	}
	(void) snprintf(d, sizeof(d), "%s%s", home, (subdir) ? subdir : "");
	if (stat(d, &st) == 0) {
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			netpgp_setvar(netpgp, "homedir", d);
			return 1;
		}
		loggit("netpgp: homedir \"%s\" is not a dir", d);
		return 0;
	}
	loggit("netpgp: warning homedir \"%s\" not found", d);
	return 1;
}

int
main(int argc, char **argv)
{
	netpgp_t	netpgp;
	prog_t          p;
	int             optindex;
	int             ret;
	int             ch;
	int             i;

	(void) memset(&p, 0x0, sizeof(p));
	(void) memset(&netpgp, 0x0, sizeof(netpgp));
	p.progname = argv[0];
	p.numbits = DEFAULT_NUMBITS;
	p.overwrite = 1;
	p.output = NULL;
	if (argc < 2) {
		print_usage(usage, p.progname);
		exit(EXIT_ERROR);
	}
	/* set some defaults */
	netpgp_setvar(&netpgp, "hash", DEFAULT_HASH_ALG);
	set_homedir(&netpgp, getenv("HOME"), "/.gnupg");
	optindex = 0;
	while ((ch = getopt_long(argc, argv, "", options, &optindex)) != -1) {
		switch (options[optindex].val) {
		case COREDUMPS:
			netpgp_setvar(&netpgp, "coredumps", "allowed");
			p.cmd = options[optindex].val;
			break;
		case VERSION_CMD:
			loggit(
"%s\nAll bug reports, praise and chocolate, please, to:\n%s",
				netpgp_get_info("version"),
				netpgp_get_info("maintainer"));
			exit(EXIT_SUCCESS);
			/* options */
		case KEYRING:
			if (optarg == NULL) {
				loggit("No keyring argument provided");
				exit(EXIT_ERROR);
			}
			snprintf(p.keyring, sizeof(p.keyring), "%s", optarg);
			break;
		case USERID:
			if (optarg == NULL) {
				loggit("No userid argument provided");
				exit(EXIT_ERROR);
			}
			netpgp_setvar(&netpgp, "userid", optarg);
			break;
		case VERBOSE:
			give_it_large(&netpgp);
			break;
		case HOMEDIR:
			if (optarg == NULL) {
				loggit("No home directory argument provided");
				exit(EXIT_ERROR);
			}
			set_homedir(&netpgp, optarg, NULL);
			break;
		case OUTPUT:
			if (optarg == NULL) {
				loggit("No output filename argument provided");
				exit(EXIT_ERROR);
			}
			if (p.output) {
				(void) free(p.output);
			}
			p.output = strdup(optarg);
			break;
		default:
			p.cmd = HELP_CMD;
			break;
		}
	}
	/* initialise, and read keys from file */
	if (!netpgp_init(&netpgp)) {
		loggit("can't initialise");
		exit(EXIT_ERROR);
	}
	/* now do the required action for each of the command line args */
	ret = EXIT_SUCCESS;
	for (i = optind; i < argc; i++) {
		if (!netpgp_verify_file(&netpgp, argv[i], p.output, p.armour)) {
			ret = EXIT_FAILURE;
		}
	}
	netpgp_end(&netpgp);
	exit(ret);
}
