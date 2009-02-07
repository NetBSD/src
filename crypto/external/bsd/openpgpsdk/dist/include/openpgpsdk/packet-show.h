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

#ifndef OPS_PACKET_TO_TEXT_H
#define OPS_PACKET_TO_TEXT_H

#ifndef OPS_PACKET_H
#include "packet.h"
#endif

/** ops_list_t
 */
typedef struct {
	unsigned int    size;	/* num of array slots allocated */
	unsigned int    used;	/* num of array slots currently used */
	char          **strings;
}               ops_list_t;

/** ops_text_t
 */
typedef struct {
	ops_list_t      known;
	ops_list_t      unknown;
}               ops_text_t;

/** ops_bit_map_t
 */
typedef struct {
	unsigned char   mask;
	const char     *string;
}               ops_bit_map_t;

void            ops_text_init(ops_text_t *);
void            ops_text_free(ops_text_t *);

const char     *ops_show_packet_tag(ops_packet_tag_t);
const char     *ops_show_ss_type(ops_ss_type_t);

const char     *ops_show_sig_type(ops_sig_type_t);
const char     *ops_show_pka(ops_public_key_algorithm_t);

ops_text_t     *ops_showall_ss_preferred_compression(ops_ss_preferred_compression_t);
const char     *ops_show_ss_preferred_compression(unsigned char);

ops_text_t     *ops_showall_ss_preferred_hash(ops_ss_preferred_hash_t);
const char     *ops_show_hash_algorithm(unsigned char);
const char     *ops_show_symmetric_algorithm(unsigned char);

ops_text_t     *ops_showall_ss_preferred_ska(ops_ss_preferred_ska_t);
const char     *ops_show_ss_preferred_ska(unsigned char);

const char     *ops_show_ss_rr_code(ops_ss_rr_code_t);

ops_text_t     *ops_showall_ss_features(ops_ss_features_t);

ops_text_t     *ops_showall_ss_key_flags(ops_ss_key_flags_t);
const char     *ops_show_ss_key_flag(unsigned char, ops_bit_map_t *);

ops_text_t     *ops_showall_ss_key_server_prefs(ops_ss_key_server_prefs_t);
const char     *
ops_show_ss_key_server_prefs(unsigned char, ops_bit_map_t *);

ops_text_t     *ops_showall_ss_notation_data_flags(ops_ss_notation_data_t);

#endif				/* OPS_PACKET_TO_TEXT_H */
