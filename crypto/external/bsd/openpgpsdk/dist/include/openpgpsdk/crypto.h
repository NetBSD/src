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

#ifndef OPS_CRYPTO_H
#define OPS_CRYPTO_H

#include "keyring.h"
#include "util.h"
#include "packet.h"
#include "packet-parse.h"
#include <openssl/dsa.h>

#define OPS_MIN_HASH_SIZE	16

typedef void    ops_hash_init_t(ops_hash_t *);
typedef void 
ops_hash_add_t(ops_hash_t *, const unsigned char *, unsigned);
typedef unsigned ops_hash_finish_t(ops_hash_t *, unsigned char *);

/** _ops_hash_t */
struct _ops_hash_t {
	ops_hash_algorithm_t algorithm;
	size_t          size;
	const char     *name;
	ops_hash_init_t *init;
	ops_hash_add_t *add;
	ops_hash_finish_t *finish;
	void           *data;
};

typedef void 
ops_crypt_set_iv_t(ops_crypt_t *, const unsigned char *);
typedef void 
ops_crypt_set_key_t(ops_crypt_t *, const unsigned char *);
typedef void    ops_crypt_init_t(ops_crypt_t *);
typedef void    ops_crypt_resync_t(ops_crypt_t *);
typedef void 
ops_crypt_block_encrypt_t(ops_crypt_t *, void *, const void *);
typedef void 
ops_crypt_block_decrypt_t(ops_crypt_t *, void *, const void *);
typedef void 
ops_crypt_cfb_encrypt_t(ops_crypt_t *, void *, const void *, size_t);
typedef void 
ops_crypt_cfb_decrypt_t(ops_crypt_t *, void *, const void *, size_t);
typedef void    ops_crypt_finish_t(ops_crypt_t *);

/** _ops_crypt_t */
struct _ops_crypt_t {
	ops_symmetric_algorithm_t algorithm;
	size_t          blocksize;
	size_t          keysize;
	ops_crypt_set_iv_t *set_iv;	/* Call this before decrypt init! */
	ops_crypt_set_key_t *set_key;	/* Call this before init! */
	ops_crypt_init_t *base_init;
	ops_crypt_resync_t *decrypt_resync;
	/* encrypt/decrypt one block  */
	ops_crypt_block_encrypt_t *block_encrypt;
	ops_crypt_block_decrypt_t *block_decrypt;

	/* Standard CFB encrypt/decrypt (as used by Sym Enc Int Prot packets) */
	ops_crypt_cfb_encrypt_t *cfb_encrypt;
	ops_crypt_cfb_decrypt_t *cfb_decrypt;

	ops_crypt_finish_t *decrypt_finish;
	unsigned char   iv[OPS_MAX_BLOCK_SIZE];
	unsigned char   civ[OPS_MAX_BLOCK_SIZE];
	unsigned char   siv[OPS_MAX_BLOCK_SIZE];	/* Needed for weird v3
							 * resync */
	unsigned char   key[OPS_MAX_KEY_SIZE];
	int             num;	/* Offset - see openssl _encrypt doco */
	void           *encrypt_key;
	void           *decrypt_key;
};

void            ops_crypto_init(void);
void            ops_crypto_finish(void);
void            ops_hash_md5(ops_hash_t *);
void            ops_hash_sha1(ops_hash_t *);
void            ops_hash_sha256(ops_hash_t *);
void            ops_hash_sha512(ops_hash_t *);
void            ops_hash_sha384(ops_hash_t *);
void            ops_hash_sha224(ops_hash_t *);
void            ops_hash_any(ops_hash_t *, ops_hash_algorithm_t);
ops_hash_algorithm_t ops_hash_algorithm_from_text(const char *);
const char     *ops_text_from_hash(ops_hash_t *);
unsigned        ops_hash_size(ops_hash_algorithm_t);
unsigned 
ops_hash(unsigned char *, ops_hash_algorithm_t, const void *, size_t);

void            ops_hash_add_int(ops_hash_t *, unsigned, unsigned);

bool 
ops_dsa_verify(const unsigned char *, size_t,
	       const ops_dsa_signature_t *,
	       const ops_dsa_public_key_t *);
int 
ops_rsa_public_decrypt(unsigned char *, const unsigned char *,
		       size_t, const ops_rsa_public_key_t *);
int 
ops_rsa_public_encrypt(unsigned char *, const unsigned char *,
		       size_t, const ops_rsa_public_key_t *);
int 
ops_rsa_private_encrypt(unsigned char *, const unsigned char *,
			size_t, const ops_rsa_secret_key_t *,
			const ops_rsa_public_key_t *);
int 
ops_rsa_private_decrypt(unsigned char *, const unsigned char *,
			size_t, const ops_rsa_secret_key_t *,
			const ops_rsa_public_key_t *);

unsigned        ops_block_size(ops_symmetric_algorithm_t);
unsigned        ops_key_size(ops_symmetric_algorithm_t);

int 
ops_decrypt_data(ops_content_tag_t, ops_region_t *, ops_parse_info_t *);

int             ops_crypt_any(ops_crypt_t *, ops_symmetric_algorithm_t);
void            ops_decrypt_init(ops_crypt_t *);
void            ops_encrypt_init(ops_crypt_t *);
size_t 
ops_decrypt_se(ops_crypt_t *, void *, const void *, size_t);
size_t 
ops_encrypt_se(ops_crypt_t *, void *, const void *, size_t);
size_t 
ops_decrypt_se_ip(ops_crypt_t *, void *, const void *, size_t);
size_t 
ops_encrypt_se_ip(ops_crypt_t *, void *, const void *, size_t );
bool   ops_is_sa_supported(ops_symmetric_algorithm_t);

void 
ops_reader_push_decrypt(ops_parse_info_t *, ops_crypt_t *, ops_region_t *);
void            ops_reader_pop_decrypt(ops_parse_info_t *);

/* Hash everything that's read */
void            ops_reader_push_hash(ops_parse_info_t *, ops_hash_t *);
void            ops_reader_pop_hash(ops_parse_info_t *);

int 
ops_decrypt_and_unencode_mpi(unsigned char *, unsigned, const BIGNUM *,
			     const ops_secret_key_t *);
bool 
ops_rsa_encrypt_mpi(const unsigned char *, const size_t,
		    const ops_public_key_t *,
		    ops_pk_session_key_parameters_t *);


/* Encrypt everything that's written */
struct ops_key_data;
void 
ops_writer_push_encrypt(ops_create_info_t *, const struct ops_key_data *);

bool   ops_encrypt_file(const char *, const char *, const ops_keydata_t *, const bool, const bool );
bool   ops_decrypt_file(const char *, const char *, ops_keyring_t *, const bool , const bool, ops_parse_cb_t *);

/* Keys */
bool   ops_rsa_generate_keypair(const int , const unsigned long, ops_keydata_t *);
ops_keydata_t  *ops_rsa_create_selfsigned_keypair(const int, const unsigned long, ops_user_id_t *);

int             ops_dsa_size(const ops_dsa_public_key_t * );
DSA_SIG        *ops_dsa_sign(unsigned char *, unsigned, const ops_dsa_secret_key_t *, const ops_dsa_public_key_t *);
#endif
