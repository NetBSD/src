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

#ifndef OPS_TYPES_H
#define OPS_TYPES_H

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
#define	bool	unsigned
#define false	0
#define	true	1
#endif

/** Special type for intermediate function casting, avoids warnings on
    some platforms
*/
typedef void    (*__ops_void_fptr) (void);
#define __ops_fcast(f) ((__ops_void_fptr)f)

/** __ops_map_t
 */
typedef struct {
	int             type;
	const char     *string;
}               __ops_map_t;

/** __ops_errcode_name_map_t */
typedef __ops_map_t __ops_errcode_name_map_t;

typedef struct _ops_crypt_t __ops_crypt_t;

/** __ops_hash_t */
typedef struct _ops_hash_t __ops_hash_t;

/** Revocation Reason type */
typedef unsigned char __ops_ss_rr_code_t;

/** __ops_parser_content_t */
typedef struct __ops_parser_content_t __ops_parser_content_t;

/** Writer flags */
typedef enum {
	OPS_WF_DUMMY
}               __ops_writer_flags_t;

/**
 * \ingroup Create
 * Contains the required information about how to write
 */
typedef struct __ops_create_info __ops_create_info_t;

#endif
