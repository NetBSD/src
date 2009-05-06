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

#ifndef __OPS_READERWRITER_H__
#define __OPS_READERWRITER_H__

#include "create.h"

#include "memory.h"

/* if this is defined, we'll use mmap in preference to file __ops */
#define USE_MMAP_FOR_FILES      1

void            __ops_reader_set_fd(__ops_parse_info_t *, int);
void            __ops_reader_set_mmap(__ops_parse_info_t *, int);
void 
__ops_reader_set_memory(__ops_parse_info_t *, const void *, size_t);

/* Do a sum mod 65536 of all bytes read (as needed for secret keys) */
void            __ops_reader_push_sum16(__ops_parse_info_t *);
unsigned short  __ops_reader_pop_sum16(__ops_parse_info_t *);

void 
__ops_reader_push_se_ip_data(__ops_parse_info_t *, __ops_crypt_t *, __ops_region_t *);
void            __ops_reader_pop_se_ip_data(__ops_parse_info_t *);

/* */
bool 
__ops_write_mdc(const unsigned char *, __ops_create_info_t *);
bool 
__ops_write_se_ip_pktset(const unsigned char *,
		       const unsigned int,
		       __ops_crypt_t *,
		       __ops_create_info_t *);
void __ops_writer_push_encrypt_crypt(__ops_create_info_t *, __ops_crypt_t *);
void __ops_writer_push_encrypt_se_ip(__ops_create_info_t *, const __ops_keydata_t *);
/* Secret Key checksum */

void            __ops_push_skey_checksum_writer(__ops_create_info_t *, __ops_seckey_t *);
bool   __ops_pop_skey_checksum_writer(__ops_create_info_t *);


/* memory writing */
void            __ops_setup_memory_write(__ops_create_info_t **, __ops_memory_t **, size_t);
void            __ops_teardown_memory_write(__ops_create_info_t *, __ops_memory_t *);

/* memory reading */
void 
__ops_setup_memory_read(__ops_parse_info_t **, __ops_memory_t *,
		      void *,
		      __ops_parse_cb_return_t callback(const __ops_packet_t *, __ops_callback_data_t *), bool);
void            __ops_teardown_memory_read(__ops_parse_info_t *, __ops_memory_t *);

/* file writing */
int             __ops_setup_file_write(__ops_create_info_t **, const char *, bool);
void            __ops_teardown_file_write(__ops_create_info_t *, int);

/* file appending */
int             __ops_setup_file_append(__ops_create_info_t **, const char *);
void            __ops_teardown_file_append(__ops_create_info_t *, int);

/* file reading */
int             __ops_setup_file_read(__ops_parse_info_t **, const char *, void *,
		    __ops_parse_cb_return_t callback(const __ops_packet_t *, __ops_callback_data_t *), bool);
void            __ops_teardown_file_read(__ops_parse_info_t *, int);

bool   __ops_reader_set_accumulate(__ops_parse_info_t *, bool);

/* useful callbacks */
__ops_parse_cb_return_t
litdata_cb(const __ops_packet_t *, __ops_callback_data_t *);
__ops_parse_cb_return_t
pk_session_key_cb(const __ops_packet_t *, __ops_callback_data_t *);
__ops_parse_cb_return_t
get_seckey_cb(const __ops_packet_t *, __ops_callback_data_t *);

/* from reader_fd.c */
void            __ops_reader_set_fd(__ops_parse_info_t *, int);

#endif				/* OPS_READERWRITER_H__ */
