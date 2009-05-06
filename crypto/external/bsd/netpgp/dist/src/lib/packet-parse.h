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
 * Parser for OpenPGP packets - headers.
 */

#ifndef OPS_PACKET_PARSE_H
#define OPS_PACKET_PARSE_H

#include "types.h"
#include "packet.h"

/** __ops_region_t */
typedef struct __ops_region_t {
	struct __ops_region_t *parent;
	unsigned        length;
	unsigned        length_read;
	unsigned        last_read;	/* !< length of last read, only valid
					 * in deepest child */
	unsigned   indeterminate:1;
}               __ops_region_t;

void            __ops_init_subregion(__ops_region_t * subregion, __ops_region_t * region);

/** __ops_parse_callback_return_t */
typedef enum {
	OPS_RELEASE_MEMORY,
	OPS_KEEP_MEMORY,
	OPS_FINISHED
}               __ops_parse_cb_return_t;

typedef struct __ops_parse_cb_info __ops_callback_data_t;

typedef         __ops_parse_cb_return_t __ops_parse_cb_t(const __ops_packet_t *, __ops_callback_data_t *);

__ops_parse_cb_return_t
get_passphrase_cb(const __ops_packet_t *, __ops_callback_data_t *);

typedef struct __ops_parse_info __ops_parse_info_t;
typedef struct __ops_reader_info __ops_reader_info_t;
typedef struct __ops_crypt_info __ops_crypt_info_t;

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

typedef int __ops_reader_t(void *, size_t, __ops_error_t **, __ops_reader_info_t *, __ops_callback_data_t *);

typedef void    __ops_reader_destroyer_t(__ops_reader_info_t *);

__ops_parse_info_t *__ops_parse_info_new(void);
void            __ops_parse_info_delete(__ops_parse_info_t *);
__ops_error_t    *__ops_parse_info_get_errors(__ops_parse_info_t *);
__ops_crypt_t    *__ops_parse_get_decrypt(__ops_parse_info_t *);

void            __ops_parse_cb_set(__ops_parse_info_t *, __ops_parse_cb_t *, void *);
void            __ops_parse_cb_push(__ops_parse_info_t *, __ops_parse_cb_t *, void *);
void           *__ops_parse_cb_get_arg(__ops_callback_data_t *);
void           *__ops_parse_cb_get_errors(__ops_callback_data_t *);
void            __ops_reader_set(__ops_parse_info_t *, __ops_reader_t *, __ops_reader_destroyer_t *, void *);
void            __ops_reader_push(__ops_parse_info_t *, __ops_reader_t *, __ops_reader_destroyer_t *, void *);
void            __ops_reader_pop(__ops_parse_info_t *);

void           *__ops_reader_get_arg(__ops_reader_info_t *);

__ops_parse_cb_return_t __ops_parse_cb(const __ops_packet_t *, __ops_callback_data_t *);
__ops_parse_cb_return_t __ops_parse_stacked_cb(const __ops_packet_t *, __ops_callback_data_t *);
__ops_reader_info_t *__ops_parse_get_rinfo(__ops_parse_info_t *);

int             __ops_parse(__ops_parse_info_t *, int);

void            __ops_parse_and_validate(__ops_parse_info_t *);

/** Used to specify whether subpackets should be returned raw, parsed or ignored.
 */
typedef enum {
	OPS_PARSE_RAW,		/* !< Callback Raw */
	OPS_PARSE_PARSED,	/* !< Callback Parsed */
	OPS_PARSE_IGNORE	/* !< Don't callback */
}               __ops_parse_type_t;

void 
__ops_parse_options(__ops_parse_info_t *, __ops_content_tag_t, __ops_parse_type_t);

bool __ops_limited_read(unsigned char *, size_t, __ops_region_t *, __ops_error_t **, __ops_reader_info_t *, __ops_callback_data_t *);
bool __ops_stacked_limited_read(unsigned char *, unsigned, __ops_region_t *, __ops_error_t **, __ops_reader_info_t *, __ops_callback_data_t *);
void __ops_parse_hash_init(__ops_parse_info_t *, __ops_hash_algorithm_t, const unsigned char *);
void __ops_parse_hash_data(__ops_parse_info_t *, const void *, size_t);
void            __ops_parse_hash_finish(__ops_parse_info_t *);
__ops_hash_t     *__ops_parse_hash_find(__ops_parse_info_t *, const unsigned char *);

__ops_reader_t    __ops_stacked_read;

int __ops_decompress(__ops_region_t *, __ops_parse_info_t *, __ops_compression_type_t);
bool __ops_write_compressed(const unsigned char *, const unsigned int, __ops_create_info_t *);

#endif
