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

/** \file */

#include "types.h"
#include "crypto.h"

/** __ops_reader_info */
struct __ops_reader_info {
	__ops_reader_t   *reader;/* !< the reader function to use to get the
				 * data to be parsed */
	__ops_reader_destroyer_t *destroyer;
	void           *arg;	/* !< the args to pass to the reader function */
	unsigned   accumulate:1;	/* !< set to accumulate packet data */
	unsigned char  *accumulated;	/* !< the accumulated data */
	unsigned        asize;	/* !< size of the buffer */
	unsigned        alength;/* !< used buffer */
	/* XXX: what do we do about offsets into compressed packets? */
	unsigned        position;	/* !< the offset from the beginning
					 * (with this reader) */

	__ops_reader_info_t *next;
	__ops_parse_info_t *pinfo;/* !< A pointer back to the parent parse_info
				 * structure */
};


/** __ops_crypt_info
 Encrypt/decrypt settings
*/
struct __ops_crypt_info {
	char           *passphrase;	/* <! passphrase to use, this is set
					 * by cb_get_passphrase */
	__ops_keyring_t  *keyring;/* <! keyring to use */
	const __ops_keydata_t *keydata;	/* <! keydata to use */
	__ops_parse_cb_t *cb_get_passphrase;	/* <! callback to use to get
						 * the passphrase */
};

/** __ops_parse_cb_info */
struct __ops_parse_cb_info {
	__ops_parse_cb_t *cb;	/* !< the callback function to use when
				 * parsing */
	void           *arg;	/* !< the args to pass to the callback
				 * function */
	__ops_error_t   **errors;	/* !< the address of the error stack to use */

	__ops_callback_data_t *next;

	__ops_create_info_t *cinfo;	/* !< used if writing out parsed info */
	__ops_crypt_info_t cryptinfo;	/* !< used when decrypting */
};

/** __ops_parse_hash_info_t */
typedef struct {
	__ops_hash_t      hash;	/* !< hashes we should hash data with */
	unsigned char   keyid[OPS_KEY_ID_SIZE];
}               __ops_parse_hash_info_t;

#define NTAGS	0x100
/** \brief Structure to hold information about a packet parse.
 *
 *  This information includes options about the parse:
 *  - whether the packet contents should be accumulated or not
 *  - whether signature subpackets should be parsed or left raw
 *
 *  It contains options specific to the parsing of armoured data:
 *  - whether headers are allowed in armoured data without a gap
 *  - whether a blank line is allowed at the start of the armoured data
 *
 *  It also specifies :
 *  - the callback function to use and its arguments
 *  - the reader function to use and its arguments
 *
 *  It also contains information about the current state of the parse:
 *  - offset from the beginning
 *  - the accumulated data, if any
 *  - the size of the buffer, and how much has been used
 *
 *  It has a linked list of errors.
 */

struct __ops_parse_info {
	unsigned char   ss_raw[NTAGS / 8];	/* !< one bit per
						 * signature-subpacket type;
						 * set to get raw data */
	unsigned char   ss_parsed[NTAGS / 8];	/* !< one bit per
						 * signature-subpacket type;
						 * set to get parsed data */
	__ops_reader_info_t	 rinfo;
	__ops_callback_data_t	 cbinfo;
	__ops_error_t		*errors;
	__ops_crypt_t		 decrypt;
	__ops_crypt_info_t	 cryptinfo;
	size_t			 nhashes;
	__ops_parse_hash_info_t *hashes;
	unsigned		 reading_v3_secret:1;
	unsigned		 reading_mpi_length:1;
	unsigned		 exact_read:1;
	void			*synthsig;	/* synthetic sig */
	void			*synthlit;	/* synthetic literal data */
};
