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
#include "memory.h"
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

typedef struct ops_writer_info ops_writer_info_t;
typedef bool 
ops_writer_t(const unsigned char *,
	     unsigned,
	     ops_error_t **,
	     ops_writer_info_t *);
typedef bool ops_writer_finaliser_t(ops_error_t **, ops_writer_info_t *);
typedef void    ops_writer_destroyer_t(ops_writer_info_t *);

/** Writer settings */
struct ops_writer_info {
	ops_writer_t   *writer;	/* !< the writer itself */
	ops_writer_finaliser_t *finaliser;	/* !< the writer's finaliser */
	ops_writer_destroyer_t *destroyer;	/* !< the writer's destroyer */
	void           *arg;	/* writer-specific argument */
	ops_writer_info_t *next;/* !< next writer in the stack */
};


void           *ops_writer_get_arg(ops_writer_info_t *);
bool 
ops_stacked_write(const void *, unsigned, ops_error_t **, ops_writer_info_t *);

void 
ops_writer_set(ops_create_info_t *,
	       ops_writer_t *,
	       ops_writer_finaliser_t *,
	       ops_writer_destroyer_t *,
	       void *);
void 
ops_writer_push(ops_create_info_t *,
		ops_writer_t *,
		ops_writer_finaliser_t *,
		ops_writer_destroyer_t *,
		void *);
void            ops_writer_pop(ops_create_info_t *);
void            ops_writer_generic_destroyer(ops_writer_info_t *);
bool 
ops_writer_passthrough(const unsigned char *,
		       unsigned,
		       ops_error_t **,
		       ops_writer_info_t *);

void            ops_writer_set_fd(ops_create_info_t *, int);
bool   ops_writer_close(ops_create_info_t *);

bool 
ops_write(const void *, unsigned, ops_create_info_t *);
bool   ops_write_length(unsigned, ops_create_info_t *);
bool   ops_write_ptag(ops_content_tag_t, ops_create_info_t *);
bool ops_write_scalar(unsigned, unsigned, ops_create_info_t *);
bool   ops_write_mpi(const BIGNUM *, ops_create_info_t *);
bool   ops_write_encrypted_mpi(const BIGNUM *, ops_crypt_t *, ops_create_info_t *);

void            writer_info_delete(ops_writer_info_t *);
bool   writer_info_finalise(ops_error_t **, ops_writer_info_t *);

#endif
