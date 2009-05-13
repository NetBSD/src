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

#ifndef OPS_WRITER_H
#define OPS_WRITER_H

#include "types.h"
#include "packet.h"
#include "crypto.h"
#include "errors.h"
#include "keyring.h"

/**
 * \ingroup Create
 * This struct contains the required information about one writer
 */
/**
 * \ingroup Writer
 * the writer function prototype
 */

typedef struct __ops_writer_info __ops_writer_info_t;
typedef bool 
__ops_writer_t(const unsigned char *,
	     unsigned,
	     __ops_error_t **,
	     __ops_writer_info_t *);
typedef bool 
__ops_writer_finaliser_t(__ops_error_t **, __ops_writer_info_t *);
typedef void    __ops_writer_destroyer_t(__ops_writer_info_t *);

/** Writer settings */
struct __ops_writer_info {
	__ops_writer_t   *writer;	/* !< the writer itself */
	__ops_writer_finaliser_t *finaliser;	/* !< the writer's finaliser */
	__ops_writer_destroyer_t *destroyer;	/* !< the writer's destroyer */
	void           *arg;	/* writer-specific argument */
	__ops_writer_info_t *next;/* !< next writer in the stack */
};


void           *__ops_writer_get_arg(__ops_writer_info_t *);
bool 
__ops_stacked_write(const void *, unsigned,
		  __ops_error_t **,
		  __ops_writer_info_t *);

void 
__ops_writer_set(__ops_create_info_t *,
	       __ops_writer_t *,
	       __ops_writer_finaliser_t *,
	       __ops_writer_destroyer_t *,
	       void *);
void 
__ops_writer_push(__ops_create_info_t *,
		__ops_writer_t *,
		__ops_writer_finaliser_t *,
		__ops_writer_destroyer_t *,
		void *);
void            __ops_writer_pop(__ops_create_info_t *);
void            __ops_writer_generic_destroyer(__ops_writer_info_t *);
bool 
__ops_writer_passthrough(const unsigned char *,
		       unsigned,
		       __ops_error_t **,
		       __ops_writer_info_t *);

void            __ops_writer_set_fd(__ops_create_info_t *, int);
bool   __ops_writer_close(__ops_create_info_t *);

bool 
__ops_write(const void *, unsigned, __ops_create_info_t *);
bool   __ops_write_length(unsigned, __ops_create_info_t *);
bool   __ops_write_ptag(__ops_content_tag_t, __ops_create_info_t *);
bool __ops_write_scalar(unsigned, unsigned, __ops_create_info_t *);
bool   __ops_write_mpi(const BIGNUM *, __ops_create_info_t *);
bool   __ops_write_encrypted_mpi(const BIGNUM *, __ops_crypt_t *, __ops_create_info_t *);

void            writer_info_delete(__ops_writer_info_t *);
bool   writer_info_finalise(__ops_error_t **, __ops_writer_info_t *);

void __ops_writer_push_stream_encrypt_se_ip(__ops_create_info_t *, const __ops_keydata_t *);

#endif
