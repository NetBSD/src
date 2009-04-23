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

#ifndef OPS_SIGNATURE_H
#define OPS_SIGNATURE_H

#include "packet.h"
#include "create.h"

typedef struct __ops_create_signature __ops_create_signature_t;

__ops_create_signature_t *__ops_create_signature_new(void);
void            __ops_create_signature_delete(__ops_create_signature_t *);

bool
__ops_check_user_id_certification_signature(const __ops_public_key_t *,
					  const __ops_user_id_t *,
					  const __ops_signature_t *,
					  const __ops_public_key_t *,
					  const unsigned char *);
bool
__ops_check_user_attribute_certification_signature(const __ops_public_key_t *,
				     const __ops_user_attribute_t *,
						 const __ops_signature_t *,
					    const __ops_public_key_t *,
					   const unsigned char *);
bool
__ops_check_subkey_signature(const __ops_public_key_t *,
			   const __ops_public_key_t *,
			   const __ops_signature_t *,
			   const __ops_public_key_t *,
			   const unsigned char *);
bool
__ops_check_direct_signature(const __ops_public_key_t *,
			   const __ops_signature_t *,
			   const __ops_public_key_t *,
			   const unsigned char *);
bool
__ops_check_hash_signature(__ops_hash_t *,
			 const __ops_signature_t *,
			 const __ops_public_key_t *);
void 
__ops_signature_start_key_signature(__ops_create_signature_t *,
				  const __ops_public_key_t *,
				  const __ops_user_id_t *,
				  __ops_sig_type_t);
void 
__ops_signature_start_cleartext_signature(__ops_create_signature_t *,
					const __ops_secret_key_t *,
					const __ops_hash_algorithm_t,
					const __ops_sig_type_t);
void 
__ops_signature_start_message_signature(__ops_create_signature_t *,
				      const __ops_secret_key_t *,
				      const __ops_hash_algorithm_t,
				      const __ops_sig_type_t);

void 
__ops_signature_add_data(__ops_create_signature_t *, const void *, size_t);
__ops_hash_t     *__ops_signature_get_hash(__ops_create_signature_t *);
bool   __ops_signature_hashed_subpackets_end(__ops_create_signature_t *);
bool 
__ops_write_signature(__ops_create_signature_t *, const __ops_public_key_t *,
		    const __ops_secret_key_t *, __ops_create_info_t *);
bool   __ops_signature_add_creation_time(__ops_create_signature_t *, time_t);
bool __ops_signature_add_issuer_key_id(__ops_create_signature_t *, const unsigned char *);
void            __ops_signature_add_primary_user_id(__ops_create_signature_t *, bool);

/* Standard Interface */
bool   __ops_sign_file_as_cleartext(const char *, const char *, const __ops_secret_key_t *, const bool);
bool   __ops_sign_file(const char *, const char *, const __ops_secret_key_t *, const bool, const bool);

/* armoured stuff */
unsigned        __ops_crc24(unsigned, unsigned char);

void            __ops_reader_push_dearmour(__ops_parse_info_t *);

void            __ops_reader_pop_dearmour(__ops_parse_info_t *);
bool __ops_writer_push_clearsigned(__ops_create_info_t *, __ops_create_signature_t *);
void            __ops_writer_push_armoured_message(__ops_create_info_t *);
bool   __ops_writer_switch_to_armoured_signature(__ops_create_info_t *);

typedef enum {
	OPS_PGP_MESSAGE = 1,
	OPS_PGP_PUBLIC_KEY_BLOCK,
	OPS_PGP_PRIVATE_KEY_BLOCK,
	OPS_PGP_MULTIPART_MESSAGE_PART_X_OF_Y,
	OPS_PGP_MULTIPART_MESSAGE_PART_X,
	OPS_PGP_SIGNATURE
} __ops_armor_type_t;

#define CRC24_INIT 0xb704ceL

bool 
__ops_writer_push_clearsigned(__ops_create_info_t *, __ops_create_signature_t *);
void            __ops_writer_push_armoured_message(__ops_create_info_t *);
bool   __ops_writer_switch_to_armoured_signature(__ops_create_info_t *);

void            __ops_writer_push_armoured(__ops_create_info_t *, __ops_armor_type_t);

#endif
