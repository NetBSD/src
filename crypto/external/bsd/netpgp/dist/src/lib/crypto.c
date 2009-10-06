/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
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

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#if defined(__NetBSD__)
__COPYRIGHT("@(#) Copyright (c) 2009 The NetBSD Foundation, Inc. All rights reserved.");
__RCSID("$NetBSD: crypto.c,v 1.17 2009/10/06 02:26:05 agc Exp $");
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#include "types.h"
#include "crypto.h"
#include "readerwriter.h"
#include "memory.h"
#include "netpgpdefs.h"
#include "signature.h"

/**
\ingroup Core_MPI
\brief Decrypt and unencode MPI
\param buf Buffer in which to write decrypted unencoded MPI
\param buflen Length of buffer
\param encmpi
\param seckey
\return length of MPI
\note only RSA at present
*/
int 
__ops_decrypt_decode_mpi(unsigned char *buf,
				unsigned buflen,
				const BIGNUM *encmpi,
				const __ops_seckey_t *seckey)
{
	unsigned char   encmpibuf[NETPGP_BUFSIZ];
	unsigned char   mpibuf[NETPGP_BUFSIZ];
	unsigned        mpisize;
	int             n;
	int             i;

	mpisize = (unsigned)BN_num_bytes(encmpi);
	/* MPI can't be more than 65,536 */
	if (mpisize > sizeof(encmpibuf)) {
		(void) fprintf(stderr, "mpisize too big %u\n", mpisize);
		return -1;
	}
	BN_bn2bin(encmpi, encmpibuf);

	if (seckey->pubkey.alg != OPS_PKA_RSA) {
		(void) fprintf(stderr, "pubkey algorithm wrong\n");
		return -1;
	}

	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr, "\nDECRYPTING\n");
		(void) fprintf(stderr, "encrypted data     : ");
		for (i = 0; i < 16; i++) {
			(void) fprintf(stderr, "%2x ", encmpibuf[i]);
		}
		(void) fprintf(stderr, "\n");
	}
	n = __ops_rsa_private_decrypt(mpibuf, encmpibuf,
				(unsigned)(BN_num_bits(encmpi) + 7) / 8,
				&seckey->key.rsa, &seckey->pubkey.key.rsa);
	if (n == -1) {
		(void) fprintf(stderr, "ops_rsa_private_decrypt failure\n");
		return -1;
	}

	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr, "decrypted encoded m buf     : ");
		for (i = 0; i < 16; i++) {
			(void) fprintf(stderr, "%2x ", mpibuf[i]);
		}
		(void) fprintf(stderr, "\n");
	}
	if (n <= 0) {
		return -1;
	}

	if (__ops_get_debug_level(__FILE__)) {
		printf(" decrypted=%d ", n);
		hexdump(stdout, mpibuf, (unsigned)n, "");
		printf("\n");
	}
	/* Decode EME-PKCS1_V1_5 (RFC 2437). */

	if (mpibuf[0] != 0 || mpibuf[1] != 2) {
		return -1;
	}

	/* Skip the random bytes. */
	for (i = 2; i < n && mpibuf[i]; ++i) {
	}

	if (i == n || i < 10) {
		return -1;
	}

	/* Skip the zero */
	i += 1;

	/* this is the unencoded m buf */
	if ((unsigned) (n - i) <= buflen) {
		(void) memcpy(buf, mpibuf + i, (unsigned)(n - i)); /* XXX - Flexelint */
	}

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
unsigned 
__ops_rsa_encrypt_mpi(const unsigned char *encoded_m_buf,
		    const size_t sz_encoded_m_buf,
		    const __ops_pubkey_t * pubkey,
		    __ops_pk_sesskey_params_t * skp)
{

	unsigned char   encmpibuf[NETPGP_BUFSIZ];
	int             n;

	if (sz_encoded_m_buf != (size_t)BN_num_bytes(pubkey->key.rsa.n)) {
		(void) fprintf(stderr, "sz_encoded_m_buf wrong\n");
		return 0;
	}

	n = __ops_rsa_public_encrypt(encmpibuf, encoded_m_buf,
				sz_encoded_m_buf, &pubkey->key.rsa);
	if (n == -1) {
		(void) fprintf(stderr, "__ops_rsa_public_encrypt failure\n");
		return 0;
	}

	if (n <= 0)
		return 0;

	skp->rsa.encrypted_m = BN_bin2bn(encmpibuf, n, NULL);

	if (__ops_get_debug_level(__FILE__)) {
		int             i;
		(void) fprintf(stderr, "encrypted mpi buf     : ");
		for (i = 0; i < 16; i++) {
			(void) fprintf(stderr, "%2x ", encmpibuf[i]);
		}
		(void) fprintf(stderr, "\n");
	}
	return 1;
}

static          __ops_cb_ret_t
callback_write_parsed(const __ops_packet_t *, __ops_cbdata_t *);

/**
\ingroup HighLevel_Crypto
Encrypt a file
\param infile Name of file to be encrypted
\param outfile Name of file to write to. If NULL, name is constructed from infile
\param pubkey Public Key to encrypt file for
\param use_armour Write armoured text, if set
\param allow_overwrite Allow output file to be overwrwritten if it exists
\return 1 if OK; else 0
*/
unsigned 
__ops_encrypt_file(__ops_io_t *io,
			const char *infile,
			const char *outfile,
			const __ops_key_t *pubkey,
			const unsigned use_armour,
			const unsigned allow_overwrite)
{
	__ops_output_t	*output;
	__ops_memory_t	*inmem;
	int		 fd_out;

	__OPS_USED(io);
	inmem = __ops_memory_new();
	if (!__ops_mem_readfile(inmem, infile)) {
		return 0;
	}
	fd_out = __ops_setup_file_write(&output, outfile, allow_overwrite);
	if (fd_out < 0) {
		__ops_memory_free(inmem);
		return 0;
	}

	/* set armoured/not armoured here */
	if (use_armour) {
		__ops_writer_push_armor_msg(output);
	}

	/* Push the encrypted writer */
	__ops_push_enc_se_ip(output, pubkey);

	/* This does the writing */
	__ops_write(output, __ops_mem_data(inmem), __ops_mem_len(inmem));

	/* tidy up */
	__ops_memory_free(inmem);
	__ops_teardown_file_write(output, fd_out);

	return 1;
}

/**
   \ingroup HighLevel_Crypto
   \brief Decrypt a file.
   \param infile Name of file to be decrypted
   \param outfile Name of file to write to. If NULL, the filename is constructed from the input filename, following GPG conventions.
   \param keyring Keyring to use
   \param use_armour Expect armoured text, if set
   \param allow_overwrite Allow output file to overwritten, if set.
   \param getpassfunc Callback to use to get passphrase
*/

unsigned 
__ops_decrypt_file(__ops_io_t *io,
			const char *infile,
			const char *outfile,
			__ops_keyring_t *keyring,
			const unsigned use_armour,
			const unsigned allow_overwrite,
			void *passfp,
			__ops_cbfunc_t *getpassfunc)
{
	__ops_stream_t	*parse = NULL;
	const int		 printerrors = 1;
	char			*filename = NULL;
	int			 fd_in;
	int			 fd_out;

	/* setup for reading from given input file */
	fd_in = __ops_setup_file_read(io, &parse, infile,
				    NULL,
				    callback_write_parsed,
				    0);
	if (fd_in < 0) {
		perror(infile);
		return 0;
	}
	/* setup output filename */
	if (outfile) {
		fd_out = __ops_setup_file_write(&parse->cbinfo.output, outfile,
				allow_overwrite);
		if (fd_out < 0) {
			perror(outfile);
			__ops_teardown_file_read(parse, fd_in);
			return 0;
		}
	} else {
		const int	suffixlen = 4;
		const char     *suffix = infile + strlen(infile) - suffixlen;
		unsigned	filenamelen;

		if (strcmp(suffix, ".gpg") == 0 ||
		    strcmp(suffix, ".asc") == 0) {
			filenamelen = strlen(infile) - strlen(suffix);
			if ((filename = calloc(1, filenamelen + 1)) == NULL) {
				(void) fprintf(stderr, "can't allocate %" PRIsize "d bytes\n",
					(size_t)(filenamelen + 1));
				return 0;
			}
			(void) strncpy(filename, infile, filenamelen);
			filename[filenamelen] = 0x0;
		}

		fd_out = __ops_setup_file_write(&parse->cbinfo.output,
					filename, allow_overwrite);
		if (fd_out < 0) {
			perror(filename);
			free(filename);
			__ops_teardown_file_read(parse, fd_in);
			return 0;
		}
	}

	/* \todo check for suffix matching armour param */

	/* setup for writing decrypted contents to given output file */

	/* setup keyring and passphrase callback */
	parse->cbinfo.cryptinfo.keyring = keyring;
	parse->cbinfo.passfp = passfp;
	parse->cbinfo.cryptinfo.getpassphrase = getpassfunc;

	/* Set up armour/passphrase options */
	if (use_armour) {
		__ops_reader_push_dearmour(parse);
	}

	/* Do it */
	__ops_parse(parse, printerrors);

	/* Unsetup */
	if (use_armour) {
		__ops_reader_pop_dearmour(parse);
	}

	if (filename) {
		__ops_teardown_file_write(parse->cbinfo.output, fd_out);
		free(filename);
	}
	__ops_teardown_file_read(parse, fd_in);
	/* \todo cleardown crypt */

	return 1;
}

static __ops_cb_ret_t
callback_write_parsed(const __ops_packet_t *pkt, __ops_cbdata_t *cbinfo)
{
	const __ops_contents_t	*content = &pkt->u;
	static unsigned		 skipping;	/* XXX - put skipping into pkt? */

	if (__ops_get_debug_level(__FILE__)) {
		printf("callback_write_parsed: ");
		__ops_print_packet(pkt);
	}
	if (pkt->tag != OPS_PTAG_CT_UNARMOURED_TEXT && skipping) {
		puts("...end of skip");
		skipping = 0;
	}
	switch (pkt->tag) {
	case OPS_PTAG_CT_UNARMOURED_TEXT:
		printf("OPS_PTAG_CT_UNARMOURED_TEXT\n");
		if (!skipping) {
			puts("Skipping...");
			skipping = 1;
		}
		fwrite(content->unarmoured_text.data, 1,
		       content->unarmoured_text.length, stdout);
		break;

	case OPS_PTAG_CT_PK_SESSION_KEY:
		return pk_sesskey_cb(pkt, cbinfo);

	case OPS_GET_SECKEY:
		return get_seckey_cb(pkt, cbinfo);

	case OPS_GET_PASSPHRASE:
		return cbinfo->cryptinfo.getpassphrase(pkt, cbinfo);

	case OPS_PTAG_CT_LITDATA_BODY:
		return litdata_cb(pkt, cbinfo);

	case OPS_PTAG_CT_ARMOUR_HEADER:
	case OPS_PTAG_CT_ARMOUR_TRAILER:
	case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
	case OPS_PTAG_CT_COMPRESSED:
	case OPS_PTAG_CT_LITDATA_HEADER:
	case OPS_PTAG_CT_SE_IP_DATA_BODY:
	case OPS_PTAG_CT_SE_IP_DATA_HEADER:
	case OPS_PTAG_CT_SE_DATA_BODY:
	case OPS_PTAG_CT_SE_DATA_HEADER:
		/* Ignore these packets  */
		/* They're handled in __ops_parse_packet() */
		/* and nothing else needs to be done */
		break;

	default:
		if (__ops_get_debug_level(__FILE__)) {
			fprintf(stderr, "Unexpected packet tag=%d (0x%x)\n",
				pkt->tag,
				pkt->tag);
		}
		break;
	}

	return OPS_RELEASE_MEMORY;
}
