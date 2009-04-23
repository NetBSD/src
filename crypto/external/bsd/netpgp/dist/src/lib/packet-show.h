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

#include "packet.h"

/** __ops_list_t
 */
typedef struct {
	unsigned int    size;	/* num of array slots allocated */
	unsigned int    used;	/* num of array slots currently used */
	char          **strings;
}               __ops_list_t;

/** __ops_text_t
 */
typedef struct {
	__ops_list_t      known;
	__ops_list_t      unknown;
}               __ops_text_t;

/** __ops_bit_map_t
 */
typedef struct {
	unsigned char   mask;
	const char     *string;
}               __ops_bit_map_t;

void            __ops_text_init(__ops_text_t * text);
void            __ops_text_free(__ops_text_t * text);

const char     *__ops_show_packet_tag(__ops_packet_tag_t packet_tag);
const char     *__ops_show_ss_type(__ops_ss_type_t ss_type);

const char     *__ops_show_sig_type(__ops_sig_type_t sig_type);
const char     *__ops_show_pka(__ops_public_key_algorithm_t pka);

__ops_text_t     *__ops_showall_ss_preferred_compression(__ops_ss_preferred_compression_t ss_preferred_compression);
const char     *__ops_show_ss_preferred_compression(unsigned char octet);

__ops_text_t     *__ops_showall_ss_preferred_hash(__ops_ss_preferred_hash_t ss_preferred_hash);
const char     *__ops_show_hash_algorithm(unsigned char octet);
const char     *__ops_show_symmetric_algorithm(unsigned char hash);

__ops_text_t     *__ops_showall_ss_preferred_ska(__ops_ss_preferred_ska_t ss_preferred_ska);
const char     *__ops_show_ss_preferred_ska(unsigned char octet);

const char     *__ops_show_ss_rr_code(__ops_ss_rr_code_t ss_rr_code);

__ops_text_t     *__ops_showall_ss_features(__ops_ss_features_t ss_features);

__ops_text_t     *__ops_showall_ss_key_flags(__ops_ss_key_flags_t ss_key_flags);
const char     *__ops_show_ss_key_flag(unsigned char octet, __ops_bit_map_t * map);

__ops_text_t     *__ops_showall_ss_key_server_prefs(__ops_ss_key_server_prefs_t ss_key_server_prefs);
const char     *
__ops_show_ss_key_server_prefs(unsigned char octet,
			     __ops_bit_map_t * map);

__ops_text_t     *__ops_showall_ss_notation_data_flags(__ops_ss_notation_data_t ss_notation_data);

#endif				/* OPS_PACKET_TO_TEXT_H */
