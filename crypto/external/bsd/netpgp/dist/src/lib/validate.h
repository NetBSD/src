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
#ifndef VALIDATE_H_
#define VALIDATE_H_	1

typedef struct {
	const __ops_keydata_t *key;
	unsigned        packet;
	unsigned        offset;
}               validate_reader_t;

/** Struct used with the validate_key_cb callback */
typedef struct {
	__ops_public_key_t pkey;
	__ops_public_key_t subkey;
	__ops_secret_key_t skey;
	enum {
		ATTRIBUTE = 1,
		ID
	}               last_seen;
	__ops_user_id_t   user_id;
	__ops_user_attribute_t user_attribute;
	unsigned char   hash[OPS_MAX_HASH_SIZE];
	const __ops_keyring_t *keyring;
	validate_reader_t *rarg;
	__ops_validation_t *result;
	__ops_parse_cb_return_t(*cb_get_passphrase) (const __ops_parser_content_t *, __ops_parse_cb_info_t *);
}               validate_key_cb_t;

/** Struct use with the validate_data_cb callback */
typedef struct {
	enum {
		LITERAL_DATA,
		SIGNED_CLEARTEXT
	}               use;	/* <! this is set to indicate what
				 * kind of data we have */
	union {
		__ops_literal_data_body_t literal_data_body;	/* <! Used to hold
								 * Literal Data */
		__ops_signed_cleartext_body_t signed_cleartext_body;	/* <! Used to hold
									 * Signed Cleartext */
	}               data;	/* <! the data itself */
	unsigned char   hash[OPS_MAX_HASH_SIZE];	/* <! the hash */
	__ops_memory_t   *mem;
	const __ops_keyring_t *keyring;	/* <! keyring to use */
	validate_reader_t *rarg;	/* <! reader-specific arg */
	__ops_validation_t *result;	/* <! where to put the result */
}               validate_data_cb_t;	/* <! used with
					 * validate_data_cb callback */

void            __ops_keydata_reader_set(__ops_parse_info_t *, const __ops_keydata_t *);

__ops_parse_cb_return_t __ops_validate_key_cb(const __ops_parser_content_t *, __ops_parse_cb_info_t *);

bool   __ops_validate_file(__ops_validation_t *, const char *, const int, const __ops_keyring_t *);
bool   __ops_validate_mem(__ops_validation_t *, __ops_memory_t *, const int, const __ops_keyring_t *);

__ops_parse_cb_return_t validate_data_cb(const __ops_parser_content_t *, __ops_parse_cb_info_t *);

#endif /* !VALIDATE_H_ */
