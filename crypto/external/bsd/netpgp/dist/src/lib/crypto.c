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
#include "config.h"

#include "crypto.h"

#include "readerwriter.h"
#include "memory.h"
#include "parse_local.h"
#include "netpgpdefs.h"
#include "signature.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <fcntl.h>

/**
\ingroup Core_MPI
\brief Decrypt and unencode MPI
\param buf Buffer in which to write decrypted unencoded MPI
\param buflen Length of buffer
\param encmpi
\param skey
\return length of MPI
\note only RSA at present
*/
int 
__ops_decrypt_and_unencode_mpi(unsigned char *buf, unsigned buflen, const BIGNUM * encmpi,
			     const __ops_secret_key_t * skey)
{
	unsigned char   encmpibuf[NETPGP_BUFSIZ];
	unsigned char   mpibuf[NETPGP_BUFSIZ];
	unsigned        mpisize;
	int             n;
	int             i;

	mpisize = BN_num_bytes(encmpi);
	/* MPI can't be more than 65,536 */
	assert(mpisize <= sizeof(encmpibuf));
	BN_bn2bin(encmpi, encmpibuf);

	assert(skey->public_key.algorithm == OPS_PKA_RSA);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "\nDECRYPTING\n");
		fprintf(stderr, "encrypted data     : ");
		for (i = 0; i < 16; i++) {
			fprintf(stderr, "%2x ", encmpibuf[i]);
		}
		fprintf(stderr, "\n");
	}
	n = __ops_rsa_private_decrypt(mpibuf, encmpibuf, (unsigned)(BN_num_bits(encmpi) + 7) / 8,
				 &skey->key.rsa, &skey->public_key.key.rsa);
	assert(n != -1);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "decrypted encoded m buf     : ");
		for (i = 0; i < 16; i++)
			fprintf(stderr, "%2x ", mpibuf[i]);
		fprintf(stderr, "\n");
	}
	if (n <= 0)
		return -1;

	if (__ops_get_debug_level(__FILE__)) {
		printf(" decrypted=%d ", n);
		hexdump(mpibuf, (unsigned)n, "");
		printf("\n");
	}
	/* Decode EME-PKCS1_V1_5 (RFC 2437). */

	if (mpibuf[0] != 0 || mpibuf[1] != 2)
		return false;

	/* Skip the random bytes. */
	for (i = 2; i < n && mpibuf[i]; ++i);

	if (i == n || i < 10)
		return false;

	/* Skip the zero */
	++i;

	/* this is the unencoded m buf */
	if ((unsigned) (n - i) <= buflen)
		(void) memcpy(buf, mpibuf + i, (unsigned)(n - i));

	if (__ops_get_debug_level(__FILE__)) {
		int             j;

		printf("decoded m buf:\n");
		for (j = 0; j < n - i; j++)
			printf("%2x ", buf[j]);
		printf("\n");
	}
	return n - i;
}

/**
\ingroup Core_MPI
\brief RSA-encrypt an MPI
*/
bool 
__ops_rsa_encrypt_mpi(const unsigned char *encoded_m_buf,
		    const size_t sz_encoded_m_buf,
		    const __ops_public_key_t * pkey,
		    __ops_pk_session_key_parameters_t * skp)
{

	unsigned char   encmpibuf[NETPGP_BUFSIZ];
	int             n = 0;

	assert(sz_encoded_m_buf == (size_t) BN_num_bytes(pkey->key.rsa.n));

	n = __ops_rsa_public_encrypt(encmpibuf, encoded_m_buf, sz_encoded_m_buf, &pkey->key.rsa);
	assert(n != -1);

	if (n <= 0)
		return false;

	skp->rsa.encrypted_m = BN_bin2bn(encmpibuf, n, NULL);

	if (__ops_get_debug_level(__FILE__)) {
		int             i;
		fprintf(stderr, "encrypted mpi buf     : ");
		for (i = 0; i < 16; i++) {
			fprintf(stderr, "%2x ", encmpibuf[i]);
		}
		fprintf(stderr, "\n");
	}
	return true;
}

static          __ops_parse_cb_return_t
callback_write_parsed(const __ops_parser_content_t * contents, __ops_parse_cb_info_t * cbinfo);

/**
\ingroup HighLevel_Crypto
Encrypt a file
\param input_filename Name of file to be encrypted
\param output_filename Name of file to write to. If NULL, name is constructed from input_filename
\param pub_key Public Key to encrypt file for
\param use_armour Write armoured text, if set
\param allow_overwrite Allow output file to be overwrwritten if it exists
\return true if OK; else false
*/
bool 
__ops_encrypt_file(const char *input_filename, const char *output_filename, const __ops_keydata_t * pub_key, const bool use_armour, const bool allow_overwrite)
{
	int             fd_in = 0;
	int             fd_out = 0;

	__ops_create_info_t *create;

	unsigned char  *buf;
	size_t          bufsz;
	size_t		done;

#ifdef O_BINARY
	fd_in = open(input_filename, O_RDONLY | O_BINARY);
#else
	fd_in = open(input_filename, O_RDONLY);
#endif
	if (fd_in < 0) {
		perror(input_filename);
		return false;
	}
	fd_out = __ops_setup_file_write(&create, output_filename, allow_overwrite);
	if (fd_out < 0)
		return false;

	/* set armoured/not armoured here */
	if (use_armour)
		__ops_writer_push_armoured_message(create);

	/* Push the encrypted writer */
	__ops_writer_push_encrypt_se_ip(create, pub_key);

	/* Do the writing */

	buf = NULL;
	bufsz = 16;
	done = 0;
	for (;;) {
		int             n = 0;

		buf = realloc(buf, done + bufsz);

		n = read(fd_in, buf + done, bufsz);
		if (!n)
			break;
		assert(n >= 0);
		done += n;
	}

	/* This does the writing */
	__ops_write(buf, done, create);

	/* tidy up */
	close(fd_in);
	free(buf);
	__ops_teardown_file_write(create, fd_out);

	return true;
}

/**
   \ingroup HighLevel_Crypto
   \brief Decrypt a file.
   \param input_filename Name of file to be decrypted
   \param output_filename Name of file to write to. If NULL, the filename is constructed from the input filename, following GPG conventions.
   \param keyring Keyring to use
   \param use_armour Expect armoured text, if set
   \param allow_overwrite Allow output file to overwritten, if set.
   \param cb_get_passphrase Callback to use to get passphrase
*/

bool 
__ops_decrypt_file(const char *input_filename, const char *output_filename, __ops_keyring_t * keyring, const bool use_armour, const bool allow_overwrite, __ops_parse_cb_t * cb_get_passphrase)
{
	int             fd_in = 0;
	int             fd_out = 0;
	char           *myfilename = NULL;
	__ops_parse_info_t *parse = NULL;

	/* setup for reading from given input file */
	fd_in = __ops_setup_file_read(&parse, input_filename,
				    NULL,
				    callback_write_parsed,
				    false);
	if (fd_in < 0) {
		perror(input_filename);
		return false;
	}
	/* setup output filename */

	if (output_filename) {
		fd_out = __ops_setup_file_write(&parse->cbinfo.cinfo, output_filename, allow_overwrite);

		if (fd_out < 0) {
			perror(output_filename);
			__ops_teardown_file_read(parse, fd_in);
			return false;
		}
	} else {
		int             suffixlen = 4;
		const char     *defaultsuffix = ".decrypted";
		const char     *suffix = input_filename + strlen(input_filename) - suffixlen;
		if (strcmp(suffix, ".gpg") == 0 ||
		    strcmp(suffix, ".asc") == 0) {
			myfilename = calloc(1, strlen(input_filename) - suffixlen + 1);
			strncpy(myfilename, input_filename, strlen(input_filename) - suffixlen);
		} else {
			unsigned        filenamelen = strlen(input_filename) + strlen(defaultsuffix) + 1;

			myfilename = calloc(1, filenamelen);
			snprintf(myfilename, filenamelen, "%s%s", input_filename, defaultsuffix);
		}

		fd_out = __ops_setup_file_write(&parse->cbinfo.cinfo, myfilename, allow_overwrite);

		if (fd_out < 0) {
			perror(myfilename);
			free(myfilename);
			__ops_teardown_file_read(parse, fd_in);
			return false;
		}
		free(myfilename);
	}

	/* \todo check for suffix matching armour param */

	/* setup for writing decrypted contents to given output file */

	/* setup keyring and passphrase callback */
	parse->cbinfo.cryptinfo.keyring = keyring;
	parse->cbinfo.cryptinfo.cb_get_passphrase = cb_get_passphrase;

	/* Set up armour/passphrase options */

	if (use_armour)
		__ops_reader_push_dearmour(parse);

	/* Do it */

	__ops_parse_and_print_errors(parse);

	/* Unsetup */

	if (use_armour)
		__ops_reader_pop_dearmour(parse);

	__ops_teardown_file_write(parse->cbinfo.cinfo, fd_out);
	__ops_teardown_file_read(parse, fd_in);
	/* \todo cleardown crypt */

	return true;
}

static          __ops_parse_cb_return_t
callback_write_parsed(const __ops_parser_content_t *contents, __ops_parse_cb_info_t * cbinfo)
{
	const __ops_parser_content_union_t *content = &contents->u;
	static bool skipping;

	OPS_USED(cbinfo);

	if (__ops_get_debug_level(__FILE__)) {
		printf("callback_write_parsed: ");
		__ops_print_packet(contents);
	}
	if (contents->tag != OPS_PTAG_CT_UNARMOURED_TEXT && skipping) {
		puts("...end of skip");
		skipping = false;
	}
	switch (contents->tag) {
	case OPS_PTAG_CT_UNARMOURED_TEXT:
		printf("OPS_PTAG_CT_UNARMOURED_TEXT\n");
		if (!skipping) {
			puts("Skipping...");
			skipping = true;
		}
		fwrite(content->unarmoured_text.data, 1,
		       content->unarmoured_text.length, stdout);
		break;

	case OPS_PTAG_CT_PK_SESSION_KEY:
		return pk_session_key_cb(contents, cbinfo);

	case OPS_PARSER_CMD_GET_SECRET_KEY:
		return get_secret_key_cb(contents, cbinfo);

	case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
		/*
		 * return
		 * get_secret_key_cb(contents,cbinfo);
		 */
		return cbinfo->cryptinfo.cb_get_passphrase(contents, cbinfo);

	case OPS_PTAG_CT_LITERAL_DATA_BODY:
		return literal_data_cb(contents, cbinfo);

	case OPS_PTAG_CT_ARMOUR_HEADER:
	case OPS_PTAG_CT_ARMOUR_TRAILER:
	case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
	case OPS_PTAG_CT_COMPRESSED:
	case OPS_PTAG_CT_LITERAL_DATA_HEADER:
	case OPS_PTAG_CT_SE_IP_DATA_BODY:
	case OPS_PTAG_CT_SE_IP_DATA_HEADER:
	case OPS_PTAG_CT_SE_DATA_BODY:
	case OPS_PTAG_CT_SE_DATA_HEADER:

		/* Ignore these packets  */
		/* They're handled in __ops_parse_packet() */
		/* and nothing else needs to be done */
		break;

	default:
		/* return callback_general(contents,cbinfo); */
		if (__ops_get_debug_level(__FILE__)) {
			fprintf(stderr, "Unexpected packet tag=%d (0x%x)\n",
				contents->tag,
				contents->tag);
		}
		break;
	}

	return OPS_RELEASE_MEMORY;
}
