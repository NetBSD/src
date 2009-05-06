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
 */
#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <openssl/rand.h>

#include "errors.h"
#include "packet.h"
#include "crypto.h"
#include "create.h"
#include "packet-parse.h"
#include "packet-show.h"
#include "signature.h"
#include "netpgpsdk.h"
#include "netpgpdefs.h"
#include "memory.h"
#include "keyring_local.h"
#include "parse_local.h"
#include "readerwriter.h"
#include "loccreate.h"
#include "version.h"

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

typedef struct {
	__ops_keyring_t  *keyring;
}               accumulate_t;

/**
 * \ingroup Core_Callbacks
 */
static          __ops_parse_cb_return_t
accumulate_cb(const __ops_packet_t * pkt, __ops_callback_data_t * cbinfo)
{
	accumulate_t *accumulate = __ops_parse_cb_get_arg(cbinfo);
	const __ops_parser_content_union_t *content = &pkt->u;
	__ops_keyring_t  *keyring = accumulate->keyring;
	__ops_keydata_t  *cur = NULL;
	const __ops_pubkey_t *pubkey;

	if (keyring->nkeys >= 0)
		cur = &keyring->keys[keyring->nkeys];

	switch (pkt->tag) {
	case OPS_PTAG_CT_PUBLIC_KEY:
	case OPS_PTAG_CT_SECRET_KEY:
	case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY:
		if (__ops_get_debug_level(__FILE__)) {
			(void) fprintf(stderr, "New key - tag %d\n", pkt->tag);
		}
		++keyring->nkeys;
		EXPAND_ARRAY(keyring, keys);

		if (pkt->tag == OPS_PTAG_CT_PUBLIC_KEY)
			pubkey = &content->pubkey;
		else
			pubkey = &content->seckey.pubkey;

		(void) memset(&keyring->keys[keyring->nkeys], 0x0,
		       sizeof(keyring->keys[keyring->nkeys]));

		__ops_keyid(keyring->keys[keyring->nkeys].key_id,
			OPS_KEY_ID_SIZE, OPS_KEY_ID_SIZE, pubkey);
		__ops_fingerprint(&keyring->keys[keyring->nkeys].fingerprint, pubkey);

		keyring->keys[keyring->nkeys].type = pkt->tag;

		if (pkt->tag == OPS_PTAG_CT_PUBLIC_KEY)
			keyring->keys[keyring->nkeys].key.pubkey = *pubkey;
		else
			keyring->keys[keyring->nkeys].key.seckey = content->seckey;
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_USER_ID:
		if (__ops_get_debug_level(__FILE__)) {
			(void) fprintf(stderr, "User ID: %s\n", content->user_id.user_id);
		}
		if (!cur) {
			OPS_ERROR(cbinfo->errors, OPS_E_P_NO_USERID, "No user id found");
			return OPS_KEEP_MEMORY;
		}
		__ops_add_userid_to_keydata(cur, &content->user_id);
		return OPS_KEEP_MEMORY;

	case OPS_PARSER_PACKET_END:
		if (!cur)
			return OPS_RELEASE_MEMORY;
		__ops_add_packet_to_keydata(cur, &content->packet);
		return OPS_KEEP_MEMORY;

	case OPS_PARSER_ERROR:
		(void) fprintf(stderr, "Error: %s\n", content->error.error);
		return OPS_FINISHED;

	case OPS_PARSER_ERRCODE:
		switch (content->errcode.errcode) {
		default:
			fprintf(stderr, "parse error: %s\n",
				__ops_errcode(content->errcode.errcode));
		}
		break;

	default:
		break;
	}

	/* XXX: we now exclude so many things, we should either drop this or */
	/* do something to pass on copies of the stuff we keep */
	return __ops_parse_stacked_cb(pkt, cbinfo);
}

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

int 
__ops_parse_and_accumulate(__ops_keyring_t *keyring, __ops_parse_info_t *parse)
{
	accumulate_t	accumulate;
	int             rtn;

	if (parse->rinfo.accumulate) {
		(void) fprintf(stderr,
			"__ops_parse_and_accumulate: already init\n");
		return 0;
	}

	(void) memset(&accumulate, 0x0, sizeof(accumulate));

	accumulate.keyring = keyring;
	/* Kinda weird, but to do with counting, and we put it back after */
	keyring->nkeys -= 1;

	__ops_parse_cb_push(parse, accumulate_cb, &accumulate);
	parse->rinfo.accumulate = true;
	rtn = __ops_parse(parse, 0);

	keyring->nkeys += 1;

	return rtn;
}

static void 
dump_one_keydata(const __ops_keydata_t * key)
{
	unsigned        n;

	printf("Key ID: ");
	hexdump(key->key_id, OPS_KEY_ID_SIZE, "");

	printf("\nFingerpint: ");
	hexdump(key->fingerprint.fingerprint, key->fingerprint.length, "");

	printf("\n\nUIDs\n====\n\n");
	for (n = 0; n < key->nuids; ++n)
		printf("%s\n", key->uids[n].user_id);

	printf("\nPackets\n=======\n");
	for (n = 0; n < key->npackets; ++n) {
		printf("\n%03d: ", n);
		hexdump(key->packets[n].raw, key->packets[n].length, "");
	}
	printf("\n\n");
}

/* XXX: not a maintained part of the API - use __ops_keyring_list() */
/** __ops_dump_keyring
*/
void 
__ops_dump_keyring(const __ops_keyring_t * keyring)
{
	int             n;

	for (n = 0; n < keyring->nkeys; ++n) {
		dump_one_keydata(&keyring->keys[n]);
	}
}


/** \file
 * \brief Error Handling
 */
#define ERRNAME(code)	{ code, #code }

static __ops_errcode_name_map_t errcode_name_map[] = {
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

	{0x00, NULL},		/* this is the end-of-array marker */
};

/**
 * \ingroup Core_Errors
 * \brief returns error code name
 * \param errcode
 * \return error code name or "Unknown"
 */
const char     *
__ops_errcode(const __ops_errcode_t errcode)
{
	return (__ops_str_from_map((int) errcode, (__ops_map_t *) errcode_name_map));
}

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

void 
__ops_push_error(__ops_error_t ** errstack, __ops_errcode_t errcode, int sys_errno,
	       const char *file, int line, const char *fmt,...)
{
	/* first get the varargs and generate the comment */
	__ops_error_t  *err;
	unsigned	maxbuf = 128;
	va_list		args;
	char           *comment;

	if ((comment = calloc(1, maxbuf + 1)) == NULL) {
		(void) fprintf(stderr, "calloc comment failure\n");
		return;
	}

	va_start(args, fmt);
	vsnprintf(comment, maxbuf + 1, fmt, args);
	va_end(args);

	/* alloc a new error and add it to the top of the stack */

	if ((err = calloc(1, sizeof(__ops_error_t))) == NULL) {
		(void) fprintf(stderr, "calloc comment failure\n");
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
\ingroup Core_Errors
\brief print this error
\param err Error to print
*/
void 
__ops_print_error(__ops_error_t * err)
{
	printf("%s:%d: ", err->file, err->line);
	if (err->errcode == OPS_E_SYSTEM_ERROR)
		printf("system error %d returned from %s()\n", err->sys_errno,
		       err->comment);
	else
		printf("%s, %s\n", __ops_errcode(err->errcode), err->comment);
}

/**
\ingroup Core_Errors
\brief Print all errors on stack
\param errstack Error stack to print
*/
void 
__ops_print_errors(__ops_error_t * errstack)
{
	__ops_error_t    *err;

	for (err = errstack; err != NULL; err = err->next)
		__ops_print_error(err);
}

/**
\ingroup Core_Errors
\brief Return true if given error is present anywhere on stack
\param errstack Error stack to check
\param errcode Error code to look for
\return 1 if found; else 0
*/
int 
__ops_has_error(__ops_error_t * errstack, __ops_errcode_t errcode)
{
	__ops_error_t    *err;
	for (err = errstack; err != NULL; err = err->next) {
		if (err->errcode == errcode)
			return 1;
	}
	return 0;
}

/**
\ingroup Core_Errors
\brief Frees all errors on stack
\param errstack Error stack to free
*/
void 
__ops_free_errors(__ops_error_t * errstack)
{
	__ops_error_t    *next;
	while (errstack != NULL) {
		next = errstack->next;
		free(errstack->comment);
		free(errstack);
		errstack = next;
	}
}

/** \file
 */

/**
 * \ingroup Core_Keys
 * \brief Calculate a public key fingerprint.
 * \param fp Where to put the calculated fingerprint
 * \param key The key for which the fingerprint is calculated
 */

void 
__ops_fingerprint(__ops_fingerprint_t * fp, const __ops_pubkey_t * key)
{
	if (key->version == 2 || key->version == 3) {
		unsigned char  *bn;
		size_t		n;
		__ops_hash_t	md5;

		if (key->algorithm != OPS_PKA_RSA &&
		    key->algorithm != OPS_PKA_RSA_ENCRYPT_ONLY &&
		    key->algorithm != OPS_PKA_RSA_SIGN_ONLY) {
			(void) fprintf(stderr,
				"__ops_fingerprint: bad algorithm\n");
			return;
		}

		__ops_hash_md5(&md5);
		md5.init(&md5);

		n = BN_num_bytes(key->key.rsa.n);
		bn = calloc(1, n);
		BN_bn2bin(key->key.rsa.n, bn);
		md5.add(&md5, bn, n);
		(void) free(bn);

		n = BN_num_bytes(key->key.rsa.e);
		bn = calloc(1, n);
		BN_bn2bin(key->key.rsa.e, bn);
		md5.add(&md5, bn, n);
		(void) free(bn);

		md5.finish(&md5, fp->fingerprint);
		fp->length = 16;
	} else {
		__ops_memory_t   *mem = __ops_memory_new();
		__ops_hash_t      sha1;
		size_t          l;

		__ops_build_pubkey(mem, key, false);

		if (__ops_get_debug_level(__FILE__)) {
			fprintf(stderr, "--- creating key fingerprint\n");
		}
		__ops_hash_sha1(&sha1);
		sha1.init(&sha1);

		l = __ops_memory_get_length(mem);

		__ops_hash_add_int(&sha1, 0x99, 1);
		__ops_hash_add_int(&sha1, l, 2);
		sha1.add(&sha1, __ops_memory_get_data(mem), l);
		sha1.finish(&sha1, fp->fingerprint);

		if (__ops_get_debug_level(__FILE__)) {
			fprintf(stderr, "--- finished creating key fingerprint\n");
		}
		fp->length = 20;

		__ops_memory_free(mem);
	}
}

/**
 * \ingroup Core_Keys
 * \brief Calculate the Key ID from the public key.
 * \param keyid Space for the calculated ID to be stored
 * \param key The key for which the ID is calculated
 */

void 
__ops_keyid(unsigned char *keyid, const size_t idlen, const int last,
	const __ops_pubkey_t *key)
{
	if (key->version == 2 || key->version == 3) {
		unsigned char   bn[NETPGP_BUFSIZ];
		unsigned        n = BN_num_bytes(key->key.rsa.n);

		if (n > sizeof(bn)) {
			(void) fprintf(stderr, "__ops_keyid: bad num bytes\n");
			return;
		}
		if (key->algorithm != OPS_PKA_RSA &&
		    key->algorithm != OPS_PKA_RSA_ENCRYPT_ONLY &&
		    key->algorithm != OPS_PKA_RSA_SIGN_ONLY) {
			(void) fprintf(stderr, "__ops_keyid: bad algorithm\n");
			return;
		}
		BN_bn2bin(key->key.rsa.n, bn);
		(void) memcpy(keyid, (last == 0) ? bn : bn + n - idlen, idlen);
	} else {
		__ops_fingerprint_t finger;

		__ops_fingerprint(&finger, key);
		(void) memcpy(keyid,
			(last == 0) ? finger.fingerprint :
				finger.fingerprint + finger.length - idlen,
			idlen);
	}
}

/**
\ingroup Core_Hashes
\brief Add to the hash
\param hash Hash to add to
\param n Int to add
\param length Length of int in bytes
*/
void 
__ops_hash_add_int(__ops_hash_t * hash, unsigned n, unsigned length)
{
	while (length--) {
		unsigned char   c[1];

		c[0] = n >> (length * 8);
		hash->add(hash, c, 1);
	}
}

/**
\ingroup Core_Hashes
\brief Setup hash for given hash algorithm
\param hash Hash to set up
\param alg Hash algorithm to use
*/
void 
__ops_hash_any(__ops_hash_t * hash, __ops_hash_algorithm_t alg)
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

	case OPS_HASH_SHA384:
		__ops_hash_sha384(hash);
		break;

	case OPS_HASH_SHA512:
		__ops_hash_sha512(hash);
		break;

	case OPS_HASH_SHA224:
		__ops_hash_sha224(hash);
		break;

	default:
		(void) fprintf(stderr, "__ops_hash_any: bad algorithm\n");
	}
}

/**
\ingroup Core_Hashes
\brief Returns size of hash for given hash algorithm
\param alg Hash algorithm to use
\return Size of hash algorithm in bytes
*/
unsigned 
__ops_hash_size(__ops_hash_algorithm_t alg)
{
	switch (alg) {
	case OPS_HASH_MD5:
		return 16;

	case OPS_HASH_SHA1:
		return 20;

	case OPS_HASH_SHA256:
		return 32;

	case OPS_HASH_SHA224:
		return 28;

	case OPS_HASH_SHA512:
		return 64;

	case OPS_HASH_SHA384:
		return 48;

	default:
		(void) fprintf(stderr, "__ops_hash_size: bad algorithm\n");
	}

	return 0;
}

/**
\ingroup Core_Hashes
\brief Returns hash enum corresponding to given string
\param hash Text name of hash algorithm i.e. "SHA1"
\returns Corresponding enum i.e. OPS_HASH_SHA1
*/
__ops_hash_algorithm_t 
__ops_hash_algorithm_from_text(const char *hash)
{
	if (!strcmp(hash, "SHA1"))
		return OPS_HASH_SHA1;
	else if (!strcmp(hash, "MD5"))
		return OPS_HASH_MD5;
	else if (!strcmp(hash, "SHA256"))
		return OPS_HASH_SHA256;
	/*
        else if (!strcmp(hash,"SHA224"))
            return OPS_HASH_SHA224;
        */
	else if (!strcmp(hash, "SHA512"))
		return OPS_HASH_SHA512;
	else if (!strcmp(hash, "SHA384"))
		return OPS_HASH_SHA384;

	return OPS_HASH_UNKNOWN;
}

/**
\ingroup Core_Hashes
\brief Hash given data
\param out Where to write the hash
\param alg Hash algorithm to use
\param in Data to hash
\param length Length of data
\return Size of hash created
*/
unsigned 
__ops_hash(unsigned char *out, __ops_hash_algorithm_t alg, const void *in,
	 size_t length)
{
	__ops_hash_t      hash;

	__ops_hash_any(&hash, alg);
	hash.init(&hash);
	hash.add(&hash, in, length);
	return hash.finish(&hash, out);
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
void 
__ops_calc_mdc_hash(const unsigned char *preamble, const size_t sz_preamble, const unsigned char *plaintext, const unsigned int sz_plaintext, unsigned char *hashed)
{
	__ops_hash_t      hash;
	unsigned char   c[1];

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;
		fprintf(stderr, "__ops_calc_mdc_hash():\n");

		fprintf(stderr, "\npreamble: ");
		for (i = 0; i < sz_preamble; i++)
			fprintf(stderr, " 0x%02x", preamble[i]);
		fprintf(stderr, "\n");

		fprintf(stderr, "\nplaintext (len=%d): ", sz_plaintext);
		for (i = 0; i < sz_plaintext; i++)
			fprintf(stderr, " 0x%02x", plaintext[i]);
		fprintf(stderr, "\n");
	}
	/* init */
	__ops_hash_any(&hash, OPS_HASH_SHA1);
	hash.init(&hash);

	/* preamble */
	hash.add(&hash, preamble, sz_preamble);
	/* plaintext */
	hash.add(&hash, plaintext, sz_plaintext);
	/* MDC packet tag */
	c[0] = 0xD3;
	hash.add(&hash, &c[0], 1);
	/* MDC packet len */
	c[0] = 0x14;
	hash.add(&hash, &c[0], 1);

	/* finish */
	hash.finish(&hash, hashed);

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;
		fprintf(stderr, "\nhashed (len=%d): ", OPS_SHA1_HASH_SIZE);
		for (i = 0; i < OPS_SHA1_HASH_SIZE; i++)
			fprintf(stderr, " 0x%02x", hashed[i]);
		fprintf(stderr, "\n");
	}
}

/**
\ingroup HighLevel_Supported
\brief Is this Hash Algorithm supported?
\param hash_alg Hash Algorithm to check
\return true if supported; else false
*/
bool 
__ops_is_hash_alg_supported(const __ops_hash_algorithm_t * hash_alg)
{
	switch (*hash_alg) {
	case OPS_HASH_MD5:
	case OPS_HASH_SHA1:
	case OPS_HASH_SHA256:
		return true;

	default:
		return false;
	}
}

void 
__ops_random(void *dest, size_t length)
{
	RAND_bytes(dest, (int)length);
}

/**
\ingroup HighLevel_Memory
\brief Memory to initialise
\param mem memory to initialise
\param needed Size to initialise to
*/
void 
__ops_memory_init(__ops_memory_t * mem, size_t needed)
{
	mem->length = 0;
	if (mem->buf) {
		if (mem->allocated < needed) {
			mem->buf = realloc(mem->buf, needed);
			mem->allocated = needed;
		}
		return;
	}
	mem->buf = calloc(1, needed);
	mem->allocated = needed;
}

/**
\ingroup HighLevel_Memory
\brief Pad memory to required length
\param mem Memory to use
\param length New size
*/
void 
__ops_memory_pad(__ops_memory_t * mem, size_t length)
{
	if (mem->allocated < mem->length) {
		(void) fprintf(stderr, "__ops_memory_pad: bad alloc in\n");
		return;
	}
	if (mem->allocated < mem->length + length) {
		mem->allocated = mem->allocated * 2 + length;
		mem->buf = realloc(mem->buf, mem->allocated);
	}
	if (mem->allocated < mem->length + length) {
		(void) fprintf(stderr, "__ops_memory_pad: bad alloc out\n");
	}
}

/**
\ingroup HighLevel_Memory
\brief Add data to memory
\param mem Memory to which to add
\param src Data to add
\param length Length of data to add
*/
void 
__ops_memory_add(__ops_memory_t * mem, const unsigned char *src, size_t length)
{
	__ops_memory_pad(mem, length);
	(void) memcpy(mem->buf + mem->length, src, length);
	mem->length += length;
}

/* XXX: this could be refactored via the writer, but an awful lot of */
/* hoops to jump through for 2 lines of code! */
void 
__ops_memory_place_int(__ops_memory_t * mem, unsigned offset, unsigned n,
		     size_t length)
{
	if (mem->allocated < offset + length) {
		(void) fprintf(stderr,
			"__ops_memory_place_int: bad alloc\n");
	} else {
		while (length--) {
			mem->buf[offset++] = n >> (length * 8);
		}
	}
}

/**
 * \ingroup HighLevel_Memory
 * \brief Retains allocated memory and set length of stored data to zero.
 * \param mem Memory to clear
 * \sa __ops_memory_release()
 * \sa __ops_memory_free()
 */
void 
__ops_memory_clear(__ops_memory_t * mem)
{
	mem->length = 0;
}

/**
\ingroup HighLevel_Memory
\brief Free memory and associated data
\param mem Memory to free
\note This does not free mem itself
\sa __ops_memory_clear()
\sa __ops_memory_free()
*/
void 
__ops_memory_release(__ops_memory_t * mem)
{
	free(mem->buf);
	mem->buf = NULL;
	mem->length = 0;
}

void 
__ops_memory_make_packet(__ops_memory_t * out, __ops_content_tag_t tag)
{
	size_t          extra;

	if (out->length < 192)
		extra = 1;
	else if (out->length < 8384)
		extra = 2;
	else
		extra = 5;

	__ops_memory_pad(out, extra + 1);
	memmove(out->buf + extra + 1, out->buf, out->length);

	out->buf[0] = OPS_PTAG_ALWAYS_SET | OPS_PTAG_NEW_FORMAT | tag;

	if (out->length < 192)
		out->buf[1] = out->length;
	else if (out->length < 8192 + 192) {
		out->buf[1] = ((out->length - 192) >> 8) + 192;
		out->buf[2] = out->length - 192;
	} else {
		out->buf[1] = 0xff;
		out->buf[2] = out->length >> 24;
		out->buf[3] = out->length >> 16;
		out->buf[4] = out->length >> 8;
		out->buf[5] = out->length;
	}

	out->length += extra + 1;
}

/**
   \ingroup HighLevel_Memory
   \brief Create a new zeroed __ops_memory_t
   \return Pointer to new __ops_memory_t
   \note Free using __ops_memory_free() after use.
   \sa __ops_memory_free()
*/

__ops_memory_t   *
__ops_memory_new(void)
{
	return calloc(1, sizeof(__ops_memory_t));
}

/**
   \ingroup HighLevel_Memory
   \brief Free memory ptr and associated memory
   \param mem Memory to be freed
   \sa __ops_memory_release()
   \sa __ops_memory_clear()
*/

void 
__ops_memory_free(__ops_memory_t * mem)
{
	__ops_memory_release(mem);
	free(mem);
}

/**
   \ingroup HighLevel_Memory
   \brief Get length of data stored in __ops_memory_t struct
   \return Number of bytes in data
*/
size_t 
__ops_memory_get_length(const __ops_memory_t * mem)
{
	return mem->length;
}

/**
   \ingroup HighLevel_Memory
   \brief Get data stored in __ops_memory_t struct
   \return Pointer to data
*/
void           *
__ops_memory_get_data(__ops_memory_t * mem)
{
	return mem->buf;
}

typedef struct {
	unsigned short  sum;
}               sum16_t;


/**
 * Searches the given map for the given type.
 * Returns a human-readable descriptive string if found,
 * returns NULL if not found
 *
 * It is the responsibility of the calling function to handle the
 * error case sensibly (i.e. don't just print out the return string.
 *
 */
static const char *
str_from_map_or_null(int type, __ops_map_t * map)
{
	__ops_map_t      *row;

	for (row = map; row->string != NULL; row++) {
		if (row->type == type) {
			return row->string;
		}
	}
	return NULL;
}

/**
 * \ingroup Core_Print
 *
 * Searches the given map for the given type.
 * Returns a readable string if found, "Unknown" if not.
 */

const char     *
__ops_str_from_map(int type, __ops_map_t * map)
{
	const char     *str;

	str = str_from_map_or_null(type, map);
	return (str) ? str : "Unknown";
}

void 
hexdump(const unsigned char *src, size_t length, const char *sep)
{
	unsigned i;

	for (i = 0 ; i < length ; i += 2) {
		printf("%02x", *src++);
		printf("%02x%s", *src++, sep);
	}
}

/**
 * \ingroup HighLevel_Functions
 * \brief Initialises OpenPGP::SDK. To be called before any other OPS function.
 *
 * Initialises OpenPGP::SDK and the underlying openssl library.
 */

void 
__ops_init(void)
{
	__ops_crypto_init();
}

/**
 * \ingroup HighLevel_Functions
 * \brief Closes down OpenPGP::SDK.
 *
 * Close down OpenPGP:SDK, release any resources under the control of
 * the library. No OpenPGP:SDK function other than __ops_init() should
 * be called after this function.
 */

void 
__ops_finish(void)
{
	__ops_crypto_finish();
}

static int 
sum16_reader(void *dest_, size_t length, __ops_error_t ** errors,
	     __ops_reader_info_t * rinfo, __ops_callback_data_t * cbinfo)
{
	const unsigned char *dest = dest_;
	sum16_t    *arg = __ops_reader_get_arg(rinfo);
	int             r = __ops_stacked_read(dest_, length, errors, rinfo, cbinfo);
	int             n;

	if (r < 0) {
		return r;
	}

	for (n = 0; n < r; ++n) {
		arg->sum = (arg->sum + dest[n]) & 0xffff;
	}

	return r;
}

static void 
sum16_destroyer(__ops_reader_info_t * rinfo)
{
	free(__ops_reader_get_arg(rinfo));
}

/**
   \ingroup Internal_Readers_Sum16
   \param pinfo Parse settings
*/

void 
__ops_reader_push_sum16(__ops_parse_info_t * pinfo)
{
	sum16_t    *arg = calloc(1, sizeof(*arg));

	__ops_reader_push(pinfo, sum16_reader, sum16_destroyer, arg);
}

/**
   \ingroup Internal_Readers_Sum16
   \param pinfo Parse settings
   \return sum
*/
unsigned short 
__ops_reader_pop_sum16(__ops_parse_info_t * pinfo)
{
	sum16_t    *arg = __ops_reader_get_arg(__ops_parse_get_rinfo(pinfo));
	unsigned short  sum = arg->sum;

	__ops_reader_pop(pinfo);
	free(arg);

	return sum;
}

/* small useful functions for setting the file-level debugging levels */
/* if the debugv list contains the filename in question, we're debugging it */

enum {
	MAX_DEBUG_NAMES = 32
};

static int      debugc;
static char    *debugv[MAX_DEBUG_NAMES];

/* set the debugging level per filename */
int
__ops_set_debug_level(const char *f)
{
	const char     *name;
	int             i;

	if (f == NULL) {
		f = "all";
	}
	if ((name = strrchr(f, '/')) == NULL) {
		name = f;
	} else {
		name += 1;
	}
	for (i = 0; i < debugc && i < MAX_DEBUG_NAMES; i++) {
		if (strcmp(debugv[i], name) == 0) {
			return 1;
		}
	}
	if (i == MAX_DEBUG_NAMES) {
		return 0;
	}
	debugv[debugc++] = strdup(name);
	return 1;
}

/* get the debugging level per filename */
int
__ops_get_debug_level(const char *f)
{
	const char     *name;
	int             i;

	if ((name = strrchr(f, '/')) == NULL) {
		name = f;
	} else {
		name += 1;
	}
	for (i = 0; i < debugc; i++) {
		if (strcmp(debugv[i], "all") == 0 ||
		    strcmp(debugv[i], name) == 0) {
			return 1;
		}
	}
	return 0;
}

/* return the version for the library */
const char *
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
