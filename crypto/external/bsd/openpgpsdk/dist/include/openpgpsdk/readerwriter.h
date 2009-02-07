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

#include <openpgpsdk/memory.h>
#include <openpgpsdk/create.h>


void            ops_reader_set_fd(ops_parse_info_t *, int);
void 
ops_reader_set_memory(ops_parse_info_t *, const void *, size_t);

/* Do a sum mod 65536 of all bytes read (as needed for secret keys) */
void            ops_reader_push_sum16(ops_parse_info_t *);
unsigned short  ops_reader_pop_sum16(ops_parse_info_t *);

void 
ops_reader_push_se_ip_data(ops_parse_info_t *, ops_crypt_t *, ops_region_t *);
void            ops_reader_pop_se_ip_data(ops_parse_info_t *);

/* */
bool 
ops_write_mdc(const unsigned char *, ops_create_info_t *);
bool 
ops_write_se_ip_pktset(const unsigned char *,
		       const unsigned int,
		       ops_crypt_t *,
		       ops_create_info_t *);
void ops_writer_push_encrypt_crypt(ops_create_info_t *, ops_crypt_t *);
void ops_writer_push_encrypt_se_ip(ops_create_info_t *, const ops_keydata_t *);
/* Secret Key checksum */

void            ops_push_skey_checksum_writer(ops_create_info_t *, ops_secret_key_t *);
bool   ops_pop_skey_checksum_writer(ops_create_info_t *);


/* memory writing */
void            ops_setup_memory_write(ops_create_info_t **, ops_memory_t **, size_t);
void            ops_teardown_memory_write(ops_create_info_t *, ops_memory_t *);

/* memory reading */
void 
ops_setup_memory_read(ops_parse_info_t **, ops_memory_t *,
		      void *,
		      ops_parse_cb_return_t callback(const ops_parser_content_t *, ops_parse_cb_info_t *), bool);
void            ops_teardown_memory_read(ops_parse_info_t *, ops_memory_t *);

/* file writing */
int             ops_setup_file_write(ops_create_info_t **, const char *, bool);
void            ops_teardown_file_write(ops_create_info_t *, int);

/* file appending */
int             ops_setup_file_append(ops_create_info_t **, const char *);
void            ops_teardown_file_append(ops_create_info_t *, int);

/* file reading */
int             ops_setup_file_read(ops_parse_info_t **, const char *, void *,
		    ops_parse_cb_return_t callback(const ops_parser_content_t *, ops_parse_cb_info_t *), bool);
void            ops_teardown_file_read(ops_parse_info_t *, int);

bool   ops_reader_set_accumulate(ops_parse_info_t *, bool);

/* useful callbacks */
ops_parse_cb_return_t
literal_data_cb(const ops_parser_content_t *, ops_parse_cb_info_t *);
ops_parse_cb_return_t
pk_session_key_cb(const ops_parser_content_t *, ops_parse_cb_info_t *);
ops_parse_cb_return_t
get_secret_key_cb(const ops_parser_content_t *, ops_parse_cb_info_t *);
ops_parse_cb_return_t
get_passphrase_cb(const ops_parser_content_t * content_, ops_parse_cb_info_t * cbinfo);

/* from reader_fd.c */
void            ops_reader_set_fd(ops_parse_info_t *, int);

#endif				/* OPS_READERWRITER_H__ */
