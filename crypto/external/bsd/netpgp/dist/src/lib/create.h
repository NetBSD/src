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

#ifndef OPS_CREATE_H
#define OPS_CREATE_H

#include "types.h"
#include "packet.h"
#include "crypto.h"
#include "errors.h"
#include "keyring.h"
#include "writer.h"

/**
 * \ingroup Create
 * This struct contains the required information about how to write this stream
 */
struct __ops_create_info {
	__ops_writer_info_t winfo;
	__ops_error_t    *errors;	/* !< an error stack */
};

__ops_create_info_t *__ops_create_info_new(void);
void            __ops_create_info_delete(__ops_create_info_t *);

int             __ops_write_file_from_buf(const char *, const char *, const size_t, const bool);

bool   __ops_calc_session_key_checksum(__ops_pk_session_key_t *, unsigned char *);
bool __ops_write_struct_user_id(__ops_user_id_t *, __ops_create_info_t *);
bool __ops_write_struct_public_key(const __ops_public_key_t *, __ops_create_info_t *);

bool __ops_write_ss_header(unsigned, __ops_content_tag_t, __ops_create_info_t *);
bool __ops_write_struct_secret_key(const __ops_secret_key_t *,
			    const unsigned char *,
			    const size_t,
			    __ops_create_info_t *);
bool 
__ops_write_one_pass_sig(const __ops_secret_key_t *,
		       const __ops_hash_algorithm_t,
		       const __ops_sig_type_t,
		       __ops_create_info_t *);
bool 
__ops_write_literal_data_from_buf(const unsigned char *,
				const int,
				const __ops_literal_data_type_t,
				__ops_create_info_t *);
__ops_pk_session_key_t *__ops_create_pk_session_key(const __ops_keydata_t *);
bool __ops_write_pk_session_key(__ops_create_info_t *, __ops_pk_session_key_t *);
bool   __ops_write_transferable_public_key(const __ops_keydata_t *, bool, __ops_create_info_t *);
bool   __ops_write_transferable_secret_key(const __ops_keydata_t *, const unsigned char *, const size_t, bool, __ops_create_info_t *);

void            __ops_fast_create_user_id(__ops_user_id_t *, unsigned char *);
bool   __ops_write_user_id(const unsigned char *, __ops_create_info_t *);
void            __ops_fast_create_rsa_public_key(__ops_public_key_t *, time_t, BIGNUM *, BIGNUM *);
bool   __ops_write_rsa_public_key(time_t, const BIGNUM *, const BIGNUM *, __ops_create_info_t *);
void            __ops_fast_create_rsa_secret_key(__ops_secret_key_t *, time_t, BIGNUM *, BIGNUM *, BIGNUM *, BIGNUM *, BIGNUM *, BIGNUM *);
bool   encode_m_buf(const unsigned char *, size_t, const __ops_public_key_t *, unsigned char *);
bool   __ops_write_literal_data_from_file(const char *, const __ops_literal_data_type_t, __ops_create_info_t *);
bool   __ops_write_symmetrically_encrypted_data(const unsigned char *, const int, __ops_create_info_t *);

#endif				/* OPS_CREATE_H */
