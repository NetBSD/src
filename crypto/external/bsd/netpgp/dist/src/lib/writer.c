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

/** \file
 * This file contains the base functions used by the writers.
 */
#include "config.h"

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#if defined(__NetBSD__)
__COPYRIGHT("@(#) Copyright (c) 2009 The NetBSD Foundation, Inc. All rights reserved.");
__RCSID("$NetBSD: writer.c,v 1.8 2009/05/16 06:30:38 agc Exp $");
#endif

#include <sys/types.h>

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_OPENSSL_CAST_H
#include <openssl/cast.h>
#endif

#include "create.h"
#include "writer.h"
#include "keyring.h"
#include "signature.h"
#include "packet.h"
#include "packet-parse.h"

#include "readerwriter.h"
#include "memory.h"
#include "netpgpdefs.h"
#include "version.h"


/*
 * return 1 if OK, otherwise 0
 */
static unsigned 
base_write(const void *src, unsigned length, __ops_output_t *out)
{
	return out->writer.writer(src, length, &out->errors, &out->writer);
}

/**
 * \ingroup Core_WritePackets
 *
 * \param src
 * \param length
 * \param output
 * \return 1 if OK, otherwise 0
 */

unsigned 
__ops_write(__ops_output_t *output, const void *src, unsigned length)
{
	return base_write(src, length, output);
}

/**
 * \ingroup Core_WritePackets
 * \param n
 * \param length
 * \param output
 * \return 1 if OK, otherwise 0
 */

unsigned 
__ops_write_scalar(__ops_output_t *output, unsigned n, unsigned length)
{
	unsigned char   c;

	while (length-- > 0) {
		c = n >> (length * 8);
		if (!base_write(&c, 1, output)) {
			return 0;
		}
	}
	return 1;
}

/**
 * \ingroup Core_WritePackets
 * \param bn
 * \param output
 * \return 1 if OK, otherwise 0
 */

unsigned 
__ops_write_mpi(__ops_output_t *output, const BIGNUM *bn)
{
	unsigned char   buf[NETPGP_BUFSIZ];
	unsigned	bits = (unsigned)BN_num_bits(bn);

	if (bits > 65535) {
		(void) fprintf(stderr, "__ops_write_mpi: too large %u\n", bits);
		return 0;
	}
	BN_bn2bin(bn, buf);
	return __ops_write_scalar(output, bits, 2) &&
		__ops_write(output, buf, (bits + 7) / 8);
}

/**
 * \ingroup Core_WritePackets
 * \param tag
 * \param output
 * \return 1 if OK, otherwise 0
 */

unsigned 
__ops_write_ptag(__ops_output_t *output, __ops_content_tag_t tag)
{
	unsigned char   c;

	c = tag | OPS_PTAG_ALWAYS_SET | OPS_PTAG_NEW_FORMAT;
	return base_write(&c, 1, output);
}

/**
 * \ingroup Core_WritePackets
 * \param length
 * \param output
 * \return 1 if OK, otherwise 0
 */

unsigned 
__ops_write_length(__ops_output_t *output, unsigned length)
{
	unsigned char   c[2];

	if (length < 192) {
		c[0] = length;
		return base_write(c, 1, output);
	}
	if (length < 8192 + 192) {
		c[0] = ((length - 192) >> 8) + 192;
		c[1] = (length - 192) % 256;
		return base_write(c, 2, output);
	}
	return __ops_write_scalar(output, 0xff, 1) &&
		__ops_write_scalar(output, length, 4);
}

/*
 * Note that we finalise from the top down, so we don't use writers below
 * that have already been finalised
 */
unsigned 
writer_info_finalise(__ops_error_t **errors, __ops_writer_t *writer)
{
	unsigned   ret = 1;

	if (writer->finaliser) {
		ret = writer->finaliser(errors, writer);
		writer->finaliser = NULL;
	}
	if (writer->next && !writer_info_finalise(errors, writer->next)) {
		writer->finaliser = NULL;
		return 0;
	}
	return ret;
}

void 
writer_info_delete(__ops_writer_t *writer)
{
	/* we should have finalised before deleting */
	if (writer->finaliser) {
		(void) fprintf(stderr, "writer_info_delete: not finalised\n");
		return;
	}
	if (writer->next) {
		writer_info_delete(writer->next);
		free(writer->next);
		writer->next = NULL;
	}
	if (writer->destroyer) {
		writer->destroyer(writer);
		writer->destroyer = NULL;
	}
	writer->writer = NULL;
}

/**
 * \ingroup Core_Writers
 *
 * Set a writer in output. There should not be another writer set.
 *
 * \param output The output structure
 * \param writer
 * \param finaliser
 * \param destroyer
 * \param arg The argument for the writer and destroyer
 */
void 
__ops_writer_set(__ops_output_t * output,
	       __ops_writer_func_t * writer,
	       __ops_writer_finaliser_t * finaliser,
	       __ops_writer_destroyer_t * destroyer,
	       void *arg)
{
	if (output->writer.writer) {
		(void) fprintf(stderr, "__ops_writer_set: already set\n");
	} else {
		output->writer.writer = writer;
		output->writer.finaliser = finaliser;
		output->writer.destroyer = destroyer;
		output->writer.arg = arg;
	}
}

/**
 * \ingroup Core_Writers
 *
 * Push a writer in output. There must already be another writer set.
 *
 * \param output The output structure
 * \param writer
 * \param finaliser
 * \param destroyer
 * \param arg The argument for the writer and destroyer
 */
void 
__ops_writer_push(__ops_output_t * output,
		__ops_writer_func_t * writer,
		__ops_writer_finaliser_t * finaliser,
		__ops_writer_destroyer_t * destroyer,
		void *arg)
{
	__ops_writer_t *copy = calloc(1, sizeof(*copy));

	if (output->writer.writer == NULL) {
		(void) fprintf(stderr, "__ops_writer_push: no orig writer\n");
	} else {
		*copy = output->writer;
		output->writer.next = copy;

		output->writer.writer = writer;
		output->writer.finaliser = finaliser;
		output->writer.destroyer = destroyer;
		output->writer.arg = arg;
	}
}

void 
__ops_writer_pop(__ops_output_t * output)
{
	__ops_writer_t *next;

	/* Make sure the finaliser has been called. */
	if (output->writer.finaliser) {
		(void) fprintf(stderr,
			"__ops_writer_pop: finaliser not called\n");
	} else if (output->writer.next == NULL) {
		(void) fprintf(stderr,
			"__ops_writer_pop: not a stacked writer\n");
	} else {
		if (output->writer.destroyer) {
			output->writer.destroyer(&output->writer);
		}
		next = output->writer.next;
		output->writer = *next;
		free(next);
	}
}

/**
 * \ingroup Core_Writers
 *
 * Close the writer currently set in output.
 *
 * \param output The output structure
 */
unsigned 
__ops_writer_close(__ops_output_t * output)
{
	unsigned   ret = writer_info_finalise(&output->errors, &output->writer);

	writer_info_delete(&output->writer);
	return ret;
}

/**
 * \ingroup Core_Writers
 *
 * Get the arg supplied to __ops_createinfo_set_writer().
 *
 * \param writer The writer_info structure
 * \return The arg
 */
void           *
__ops_writer_get_arg(__ops_writer_t *writer)
{
	return writer->arg;
}

/**
 * \ingroup Core_Writers
 *
 * Write to the next writer down in the stack.
 *
 * \param src The data to write.
 * \param length The length of src.
 * \param errors A place to store errors.
 * \param writer The writer_info structure.
 * \return Success - if 0, then errors should contain the error.
 */
unsigned 
__ops_stacked_write(const void *src, unsigned length,
		  __ops_error_t ** errors, __ops_writer_t *writer)
{
	return writer->next->writer(src, length, errors, writer->next);
}

/**
 * \ingroup Core_Writers
 *
 * Free the arg. Many writers just have a calloc()ed lump of storage, this
 * function releases it.
 *
 * \param writer the info structure.
 */
void 
__ops_writer_generic_destroyer(__ops_writer_t *writer)
{
	(void) free(__ops_writer_get_arg(writer));
}

/**
 * \ingroup Core_Writers
 *
 * A writer that just writes to the next one down. Useful for when you
 * want to insert just a finaliser into the stack.
 */
unsigned 
__ops_writer_passthrough(const unsigned char *src,
		       unsigned length,
		       __ops_error_t **errors,
		       __ops_writer_t *writer)
{
	return __ops_stacked_write(src, length, errors, writer);
}

/**************************************************************************/

/**
 * \struct dash_escaped_t
 */
typedef struct {
	unsigned   		 seen_nl:1;
	unsigned		 seen_cr:1;
	__ops_create_sig_t	*sig;
	__ops_memory_t		*trailing;
} dash_escaped_t;

static unsigned 
dash_escaped_writer(const unsigned char *src,
		    unsigned length,
		    __ops_error_t **errors,
		    __ops_writer_t *writer)
{
	dash_escaped_t *dash = __ops_writer_get_arg(writer);
	unsigned        n;

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;

		fprintf(stderr, "dash_escaped_writer writing %d:\n", length);
		for (i = 0; i < length; i++) {
			fprintf(stderr, "0x%02x ", src[i]);
			if (!((i + 1) % 16)) {
				fprintf(stderr, "\n");
			} else if (!((i + 1) % 8)) {
				fprintf(stderr, "  ");
			}
		}
		fprintf(stderr, "\n");
	}
	/* XXX: make this efficient */
	for (n = 0; n < length; ++n) {
		unsigned        l;

		if (dash->seen_nl) {
			if (src[n] == '-' &&
			    !__ops_stacked_write("- ", 2, errors, writer)) {
				return 0;
			}
			dash->seen_nl = 0;
		}
		dash->seen_nl = src[n] == '\n';

		if (dash->seen_nl && !dash->seen_cr) {
			if (!__ops_stacked_write("\r", 1, errors, writer)) {
				return 0;
			}
			__ops_sig_add_data(dash->sig, "\r", 1);
		}
		dash->seen_cr = src[n] == '\r';

		if (!__ops_stacked_write(&src[n], 1, errors, writer)) {
			return 0;
		}

		/* trailing whitespace isn't included in the signature */
		if (src[n] == ' ' || src[n] == '\t') {
			__ops_memory_add(dash->trailing, &src[n], 1);
		} else {
			if ((l = __ops_mem_len(dash->trailing)) != 0) {
				if (!dash->seen_nl && !dash->seen_cr) {
					__ops_sig_add_data(dash->sig,
					__ops_mem_data(dash->trailing), l);
				}
				__ops_memory_clear(dash->trailing);
			}
			__ops_sig_add_data(dash->sig, &src[n], 1);
		}
	}
	return 1;
}

/**
 * \param writer
 */
static void 
dash_escaped_destroyer(__ops_writer_t * writer)
{
	dash_escaped_t	*dash;

	dash = __ops_writer_get_arg(writer);
	__ops_memory_free(dash->trailing);
	(void) free(dash);
}

/**
 * \ingroup Core_WritersNext
 * \brief Push Clearsigned Writer onto stack
 * \param output
 * \param sig
 */
unsigned 
__ops_writer_push_clearsigned(__ops_output_t * output,
			    __ops_create_sig_t * sig)
{
	static const char     header[] =
		"-----BEGIN PGP SIGNED MESSAGE-----\r\nHash: ";
	const char     *hash = __ops_text_from_hash(__ops_sig_get_hash(sig));
	dash_escaped_t *dash = calloc(1, sizeof(*dash));
	unsigned	ret;

	ret = (__ops_write(output, header, sizeof(header) - 1) &&
		__ops_write(output, hash, strlen(hash)) &&
		__ops_write(output, "\r\n\r\n", 4));

	if (ret == 0) {
		OPS_ERROR(&output->errors, OPS_E_W,
			"Error pushing clearsigned header");
		free(dash);
		return ret;
	}
	dash->seen_nl = 1;
	dash->sig = sig;
	dash->trailing = __ops_memory_new();
	__ops_writer_push(output, dash_escaped_writer, NULL,
			dash_escaped_destroyer, dash);
	return ret;
}


/**
 * \struct base64_t
 */
typedef struct {
	unsigned        pos;
	unsigned char   t;
	unsigned        checksum;
}               base64_t;

static const char     b64map[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned 
base64_writer(const unsigned char *src,
	      unsigned length, __ops_error_t ** errors,
	      __ops_writer_t * writer)
{
	base64_t   *base64 = __ops_writer_get_arg(writer);
	unsigned        n;

	for (n = 0; n < length;) {
		base64->checksum = __ops_crc24(base64->checksum, src[n]);
		if (base64->pos == 0) {
			/* XXXXXX00 00000000 00000000 */
			if (!__ops_stacked_write(&b64map[(unsigned)src[n] >> 2],
					1, errors, writer)) {
				return 0;
			}

			/* 000000XX xxxx0000 00000000 */
			base64->t = (src[n++] & 3) << 4;
			base64->pos = 1;
		} else if (base64->pos == 1) {
			/* 000000xx XXXX0000 00000000 */
			base64->t += (unsigned)src[n] >> 4;
			if (!__ops_stacked_write(&b64map[base64->t], 1,
					errors, writer)) {
				return 0;
			}

			/* 00000000 0000XXXX xx000000 */
			base64->t = (src[n++] & 0xf) << 2;
			base64->pos = 2;
		} else if (base64->pos == 2) {
			/* 00000000 0000xxxx XX000000 */
			base64->t += (unsigned)src[n] >> 6;
			if (!__ops_stacked_write(&b64map[base64->t], 1,
					errors, writer)) {
				return 0;
			}

			/* 00000000 00000000 00XXXXXX */
			if (!__ops_stacked_write(&b64map[src[n++] & 0x3f], 1,
					errors, writer)) {
				return 0;
			}

			base64->pos = 0;
		}
	}

	return 1;
}

static unsigned 
sig_finaliser(__ops_error_t ** errors,
		    __ops_writer_t * writer)
{
	base64_t   *base64 = __ops_writer_get_arg(writer);
	static const char     trailer[] = "\r\n-----END PGP SIGNATURE-----\r\n";
	unsigned char   c[3];

	if (base64->pos) {
		if (!__ops_stacked_write(&b64map[base64->t], 1, errors,
				writer)) {
			return 0;
		}
		if (base64->pos == 1 && !__ops_stacked_write("==", 2, errors,
				writer)) {
			return 0;
		}
		if (base64->pos == 2 && !__ops_stacked_write("=", 1, errors,
				writer)) {
			return 0;
		}
	}
	/* Ready for the checksum */
	if (!__ops_stacked_write("\r\n=", 3, errors, writer)) {
		return 0;
	}

	base64->pos = 0;		/* get ready to write the checksum */

	c[0] = base64->checksum >> 16;
	c[1] = base64->checksum >> 8;
	c[2] = base64->checksum;
	/* push the checksum through our own writer */
	if (!base64_writer(c, 3, errors, writer)) {
		return 0;
	}

	return __ops_stacked_write(trailer, sizeof(trailer) - 1, errors, writer);
}

/**
 * \struct linebreak_t
 */
typedef struct {
	unsigned        pos;
}               linebreak_t;

#define BREAKPOS	76

static unsigned 
linebreak_writer(const unsigned char *src,
		 unsigned length,
		 __ops_error_t ** errors,
		 __ops_writer_t * writer)
{
	linebreak_t *linebreak = __ops_writer_get_arg(writer);
	unsigned        n;

	for (n = 0; n < length; ++n, ++linebreak->pos) {
		if (src[n] == '\r' || src[n] == '\n') {
			linebreak->pos = 0;
		}

		if (linebreak->pos == BREAKPOS) {
			if (!__ops_stacked_write("\r\n", 2, errors, writer)) {
				return 0;
			}
			linebreak->pos = 0;
		}
		if (!__ops_stacked_write(&src[n], 1, errors, writer)) {
			return 0;
		}
	}

	return 1;
}

/**
 * \ingroup Core_WritersNext
 * \brief Push armoured signature on stack
 * \param output
 */
unsigned 
__ops_writer_use_armored_sig(__ops_output_t * output)
{
	static const char     header[] =
			"\r\n-----BEGIN PGP SIGNATURE-----\r\nVersion: "
			NETPGP_VERSION_STRING
			"\r\n\r\n";
	base64_t   *base64;

	__ops_writer_pop(output);
	if (__ops_write(output, header, sizeof(header) - 1) == 0) {
		OPS_ERROR(&output->errors, OPS_E_W,
			"Error switching to armoured signature");
		return 0;
	}
	__ops_writer_push(output, linebreak_writer, NULL,
			__ops_writer_generic_destroyer,
			calloc(1, sizeof(linebreak_t)));
	base64 = calloc(1, sizeof(*base64));
	if (!base64) {
		OPS_MEMORY_ERROR(&output->errors);
		return 0;
	}
	base64->checksum = CRC24_INIT;
	__ops_writer_push(output, base64_writer, sig_finaliser,
			__ops_writer_generic_destroyer, base64);
	return 1;
}

static unsigned 
armoured_message_finaliser(__ops_error_t ** errors,
			   __ops_writer_t * writer)
{
	/* TODO: This is same as sig_finaliser apart from trailer. */
	base64_t   *base64 = __ops_writer_get_arg(writer);
	static const char     trailer[] = "\r\n-----END PGP MESSAGE-----\r\n";
	unsigned char   c[3];

	if (base64->pos) {
		if (!__ops_stacked_write(&b64map[base64->t], 1, errors,
				writer)) {
			return 0;
		}
		if (base64->pos == 1 &&
		    !__ops_stacked_write("==", 2, errors, writer)) {
			return 0;
		}
		if (base64->pos == 2 &&
		    !__ops_stacked_write("=", 1, errors, writer)) {
			return 0;
		}
	}
	/* Ready for the checksum */
	if (!__ops_stacked_write("\r\n=", 3, errors, writer)) {
		return 0;
	}

	base64->pos = 0;		/* get ready to write the checksum */

	c[0] = base64->checksum >> 16;
	c[1] = base64->checksum >> 8;
	c[2] = base64->checksum;
	/* push the checksum through our own writer */
	if (!base64_writer(c, 3, errors, writer)) {
		return 0;
	}

	return __ops_stacked_write(trailer, sizeof(trailer)-1, errors, writer);
}

/**
 \ingroup Core_WritersNext
 \brief Write a PGP MESSAGE
 \todo replace with generic function
*/
void 
__ops_writer_push_armor_msg(__ops_output_t * output)
{
	static const char	 header[] = "-----BEGIN PGP MESSAGE-----\r\n";
	base64_t		*base64;

	__ops_write(output, header, sizeof(header) - 1);
	__ops_write(output, "\r\n", 2);
	base64 = calloc(1, sizeof(*base64));
	base64->checksum = CRC24_INIT;
	__ops_writer_push(output, base64_writer, armoured_message_finaliser,
		__ops_writer_generic_destroyer, base64);
}

static unsigned 
armoured_finaliser(__ops_armor_type_t type, __ops_error_t ** errors,
		   __ops_writer_t * writer)
{
	static const char     tail_pubkey[] =
			"\r\n-----END PGP PUBLIC KEY BLOCK-----\r\n";
	static const char     tail_private_key[] =
			"\r\n-----END PGP PRIVATE KEY BLOCK-----\r\n";
	unsigned char		 c[3];
	unsigned int		 sz_tail = 0;
	const char		*tail = NULL;
	base64_t		*base64;

	switch (type) {
	case OPS_PGP_PUBLIC_KEY_BLOCK:
		tail = tail_pubkey;
		sz_tail = sizeof(tail_pubkey) - 1;
		break;

	case OPS_PGP_PRIVATE_KEY_BLOCK:
		tail = tail_private_key;
		sz_tail = sizeof(tail_private_key) - 1;
		break;

	default:
		(void) fprintf(stderr, "armoured_finaliser: unusual type\n");
		return 0;
	}

	base64 = __ops_writer_get_arg(writer);

	if (base64->pos) {
		if (!__ops_stacked_write(&b64map[base64->t], 1, errors,
				writer)) {
			return 0;
		}
		if (base64->pos == 1 && !__ops_stacked_write("==", 2, errors,
				writer)) {
			return 0;
		}
		if (base64->pos == 2 && !__ops_stacked_write("=", 1, errors,
				writer)) {
			return 0;
		}
	}
	/* Ready for the checksum */
	if (!__ops_stacked_write("\r\n=", 3, errors, writer)) {
		return 0;
	}

	base64->pos = 0;		/* get ready to write the checksum */

	c[0] = base64->checksum >> 16;
	c[1] = base64->checksum >> 8;
	c[2] = base64->checksum;
	/* push the checksum through our own writer */
	if (!base64_writer(c, 3, errors, writer)) {
		return 0;
	}

	return __ops_stacked_write(tail, sz_tail, errors, writer);
}

static unsigned 
armoured_pubkey_finaliser(__ops_error_t ** errors,
			      __ops_writer_t * writer)
{
	return armoured_finaliser(OPS_PGP_PUBLIC_KEY_BLOCK, errors, writer);
}

static unsigned 
armoured_private_key_finaliser(__ops_error_t ** errors,
			       __ops_writer_t * writer)
{
	return armoured_finaliser(OPS_PGP_PRIVATE_KEY_BLOCK, errors, writer);
}

/* \todo use this for other armoured types */
/**
 \ingroup Core_WritersNext
 \brief Push Armoured Writer on stack (generic)
*/
void 
__ops_writer_push_armoured(__ops_output_t * output, __ops_armor_type_t type)
{
	static char     hdr_pubkey[] =
			"-----BEGIN PGP PUBLIC KEY BLOCK-----\r\nVersion: "
			NETPGP_VERSION_STRING
			"\r\n\r\n";
	static char     hdr_private_key[] =
			"-----BEGIN PGP PRIVATE KEY BLOCK-----\r\nVersion: "
			NETPGP_VERSION_STRING
			"\r\n\r\n";
	unsigned int    sz_hdr = 0;
	unsigned	(*finaliser) (__ops_error_t **, __ops_writer_t *);
	base64_t	*base64;
	char           *header = NULL;

	finaliser = NULL;
	switch (type) {
	case OPS_PGP_PUBLIC_KEY_BLOCK:
		header = hdr_pubkey;
		sz_hdr = sizeof(hdr_pubkey) - 1;
		finaliser = armoured_pubkey_finaliser;
		break;

	case OPS_PGP_PRIVATE_KEY_BLOCK:
		header = hdr_private_key;
		sz_hdr = sizeof(hdr_private_key) - 1;
		finaliser = armoured_private_key_finaliser;
		break;

	default:
		(void) fprintf(stderr,
			"__ops_writer_push_armoured: unusual type\n");
		return;
	}

	__ops_write(output, header, sz_hdr);

	__ops_writer_push(output, linebreak_writer, NULL,
			__ops_writer_generic_destroyer,
			calloc(1, sizeof(linebreak_t)));

	base64 = calloc(1, sizeof(*base64));
	base64->checksum = CRC24_INIT;
	__ops_writer_push(output, base64_writer, finaliser,
			__ops_writer_generic_destroyer, base64);
}

/**************************************************************************/

typedef struct {
	__ops_crypt_t    *crypt;
	int             free_crypt;
}               crypt_t;

/*
 * This writer simply takes plaintext as input,
 * encrypts it with the given key
 * and outputs the resulting encrypted text
 */
static unsigned 
encrypt_writer(const unsigned char *src,
	       unsigned length,
	       __ops_error_t ** errors,
	       __ops_writer_t * writer)
{

#define BUFSZ 1024		/* arbitrary number */
	unsigned char   encbuf[BUFSZ];
	unsigned        remaining = length;
	unsigned        done = 0;

	crypt_t    *pgp_encrypt = (crypt_t *) __ops_writer_get_arg(writer);

	if (!__ops_is_sa_supported(pgp_encrypt->crypt->alg)) {
		(void) fprintf(stderr, "encrypt_writer: not supported\n");
		return 0;
	}

	while (remaining) {
		unsigned        len = remaining < BUFSZ ? remaining : BUFSZ;
		/* memcpy(buf,src,len); // \todo copy needed here? */

		pgp_encrypt->crypt->cfb_encrypt(pgp_encrypt->crypt, encbuf,
					src + done, len);

		if (__ops_get_debug_level(__FILE__)) {
			int             i = 0;

			fprintf(stderr, "WRITING:\nunencrypted: ");
			for (i = 0; i < 16; i++) {
				fprintf(stderr, "%2x ", src[done + i]);
			}
			fprintf(stderr, "\n");
			fprintf(stderr, "encrypted:   ");
			for (i = 0; i < 16; i++) {
				fprintf(stderr, "%2x ", encbuf[i]);
			}
			fprintf(stderr, "\n");
		}
		if (!__ops_stacked_write(encbuf, len, errors, writer)) {
			if (__ops_get_debug_level(__FILE__)) {
				fprintf(stderr,
"encrypted_writer got error from stacked write, returning\n");
			}
			return 0;
		}
		remaining -= len;
		done += len;
	}

	return 1;
}

static void 
encrypt_destroyer(__ops_writer_t * writer)
{
	crypt_t    *pgp_encrypt = (crypt_t *) __ops_writer_get_arg(writer);

	if (pgp_encrypt->free_crypt) {
		free(pgp_encrypt->crypt);
	}
	free(pgp_encrypt);
}

/**
\ingroup Core_WritersNext
\brief Push Encrypted Writer onto stack (create SE packets)
*/
void 
__ops_writer_push_encrypt_crypt(__ops_output_t *output,
			      __ops_crypt_t *pgp_crypt)
{
	/* Create encrypt to be used with this writer */
	/* Remember to free this in the destroyer */

	crypt_t    *pgp_encrypt = calloc(1, sizeof(*pgp_encrypt));

	/* Setup the encrypt */
	pgp_encrypt->crypt = pgp_crypt;
	pgp_encrypt->free_crypt = 0;

	/* And push writer on stack */
	__ops_writer_push(output, encrypt_writer, NULL, encrypt_destroyer,
			pgp_encrypt);
}

/**************************************************************************/

typedef struct {
	__ops_crypt_t    *crypt;
} encrypt_se_ip_t;

static unsigned	encrypt_se_ip_writer(const unsigned char *,
		     unsigned,
		     __ops_error_t **,
		     __ops_writer_t *);
static void     encrypt_se_ip_destroyer(__ops_writer_t *);

/* */

/**
\ingroup Core_WritersNext
\brief Push Encrypted SE IP Writer onto stack
*/
void 
__ops_writer_push_encrypt_se_ip(__ops_output_t *output,
			      const __ops_keydata_t *pubkey)
{
	unsigned char	*iv = NULL;
	__ops_crypt_t	*encrypted;

	/* Create se_ip to be used with this writer */
	/* Remember to free this in the destroyer */
	encrypt_se_ip_t *se_ip = calloc(1, sizeof(*se_ip));

	/* Create and write encrypted PK session key */
	__ops_pk_sesskey_t *encrypted_pk_sesskey;
	encrypted_pk_sesskey = __ops_create_pk_sesskey(pubkey);
	__ops_write_pk_sesskey(output, encrypted_pk_sesskey);

	/* Setup the se_ip */
	encrypted = calloc(1, sizeof(*encrypted));
	__ops_crypt_any(encrypted, encrypted_pk_sesskey->symm_alg);
	iv = calloc(1, encrypted->blocksize);
	encrypted->set_iv(encrypted, iv);
	encrypted->set_key(encrypted, &encrypted_pk_sesskey->key[0]);
	__ops_encrypt_init(encrypted);

	se_ip->crypt = encrypted;

	/* And push writer on stack */
	__ops_writer_push(output, encrypt_se_ip_writer, NULL,
			encrypt_se_ip_destroyer, se_ip);
	/* tidy up */
	(void) free(encrypted_pk_sesskey);
	(void) free(iv);
}

static unsigned 
encrypt_se_ip_writer(const unsigned char *src,
		     unsigned len,
		     __ops_error_t **errors,
		     __ops_writer_t *writer)
{
	const unsigned int	 bufsz = 128;
	encrypt_se_ip_t		*se_ip = __ops_writer_get_arg(writer);
	__ops_output_t		*litoutput;
	__ops_output_t		*zoutput;
	__ops_output_t		*output;
	__ops_memory_t		*litmem;
	__ops_memory_t		*zmem;
	__ops_memory_t		*localmem;
	unsigned		 ret = 1;

	__ops_setup_memory_write(&litoutput, &litmem, bufsz);
	__ops_setup_memory_write(&zoutput, &zmem, bufsz);
	__ops_setup_memory_write(&output, &localmem, bufsz);

	/* create literal data packet from source data */
	__ops_write_litdata(litoutput, src, (const int)len, OPS_LDT_BINARY);
	if (__ops_mem_len(litmem) <= len) {
		(void) fprintf(stderr, "encrypt_se_ip_writer: bad length\n");
		return 0;
	}

	/* create compressed packet from literal data packet */
	__ops_writez(__ops_mem_data(litmem), __ops_mem_len(litmem), zoutput);

	/* create SE IP packet set from this compressed literal data */
	__ops_write_se_ip_pktset(__ops_mem_data(zmem),
			       __ops_mem_len(zmem),
			       se_ip->crypt, output);
	if (__ops_mem_len(localmem) <= __ops_mem_len(zmem)) {
		(void) fprintf(stderr,
				"encrypt_se_ip_writer: bad comp length\n");
		return 0;
	}

	/* now write memory to next writer */
	ret = __ops_stacked_write(__ops_mem_data(localmem),
				__ops_mem_len(localmem),
				errors, writer);

	__ops_memory_free(localmem);
	__ops_memory_free(zmem);
	__ops_memory_free(litmem);

	return ret;
}

static void 
encrypt_se_ip_destroyer(__ops_writer_t * writer)
{
	encrypt_se_ip_t	*se_ip;

	se_ip = __ops_writer_get_arg(writer);
	(void) free(se_ip->crypt);
	(void) free(se_ip);
}

unsigned 
__ops_write_se_ip_pktset(const unsigned char *data,
		       const unsigned int len,
		       __ops_crypt_t *crypted,
		       __ops_output_t *output)
{
	__ops_output_t	*mdcoutput;
	__ops_memory_t	*mdcmem;
	unsigned char	 hashed[OPS_SHA1_HASH_SIZE];
	unsigned char	*preamble;
	const size_t	 sz_mdc = 1 + 1 + OPS_SHA1_HASH_SIZE;
	size_t		 sz_preamble;
	size_t		 sz_buf;

	sz_preamble = crypted->blocksize + 2;
	preamble = calloc(1, sz_preamble);
	sz_buf = sz_preamble + len + sz_mdc;

	if (!__ops_write_ptag(output, OPS_PTAG_CT_SE_IP_DATA) ||
	    !__ops_write_length(output, 1 + sz_buf) ||
	    !__ops_write_scalar(output, SE_IP_DATA_VERSION, 1)) {
		free(preamble);
		return 0;
	}
	__ops_random(preamble, crypted->blocksize);
	preamble[crypted->blocksize] = preamble[crypted->blocksize - 2];
	preamble[crypted->blocksize + 1] = preamble[crypted->blocksize - 1];

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;

		fprintf(stderr, "\npreamble: ");
		for (i = 0; i < sz_preamble; i++) {
			fprintf(stderr, " 0x%02x", preamble[i]);
		}
		fprintf(stderr, "\n");
	}

	/* now construct MDC packet and add to the end of the buffer */
	__ops_setup_memory_write(&mdcoutput, &mdcmem, sz_mdc);
	__ops_calc_mdc_hash(preamble, sz_preamble, data, len, &hashed[0]);
	__ops_write_mdc(hashed, mdcoutput);

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;
		size_t          sz_plaintext = len;
		size_t          sz_mdc2 = 1 + 1 + OPS_SHA1_HASH_SIZE;
		unsigned char  *mdc = NULL;

		(void) fprintf(stderr, "\nplaintext: ");
		for (i = 0; i < sz_plaintext; i++) {
			(void) fprintf(stderr, " 0x%02x", data[i]);
		}
		(void) fprintf(stderr, "\n");

		(void) fprintf(stderr, "\nmdc: ");
		mdc = __ops_mem_data(mdcmem);
		for (i = 0; i < sz_mdc2; i++) {
			(void) fprintf(stderr, " 0x%02x", mdc[i]);
		}
		(void) fprintf(stderr, "\n");
	}

	/* and write it out */
	__ops_writer_push_encrypt_crypt(output, crypted);
	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr,
			"writing %" PRIsize "u + %d + %" PRIsize "u\n",
			sz_preamble, len, __ops_mem_len(mdcmem));
	}
	if (!__ops_write(output, preamble, sz_preamble) ||
	    !__ops_write(output, data, len) ||
	    !__ops_write(output, __ops_mem_data(mdcmem),
	    		__ops_mem_len(mdcmem))) {
		/* \todo fix cleanup here and in old code functions */
		return 0;
	}

	__ops_writer_pop(output);

	/* cleanup  */
	__ops_teardown_memory_write(mdcoutput, mdcmem);
	(void) free(preamble);

	return 1;
}

typedef struct {
	int             fd;
} writer_fd_t;

static unsigned 
fd_writer(const unsigned char *src, unsigned length,
	  __ops_error_t **errors,
	  __ops_writer_t *writer)
{
	writer_fd_t	*writerfd;
	int              n;

	writerfd = __ops_writer_get_arg(writer);
	n = write(writerfd->fd, src, length);
	if (n == -1) {
		OPS_SYSTEM_ERROR_1(errors, OPS_E_W_WRITE_FAILED, "write",
				   "file descriptor %d", writerfd->fd);
		return 0;
	}
	if ((unsigned) n != length) {
		OPS_ERROR_1(errors, OPS_E_W_WRITE_TOO_SHORT,
			    "file descriptor %d", writerfd->fd);
		return 0;
	}
	return 1;
}

static void 
writer_fd_destroyer(__ops_writer_t * writer)
{
	(void) free(__ops_writer_get_arg(writer));
}

/**
 * \ingroup Core_WritersFirst
 * \brief Write to a File
 *
 * Set the writer in output to be a stock writer that writes to a file
 * descriptor. If another writer has already been set, then that is
 * first destroyed.
 *
 * \param output The output structure
 * \param fd The file descriptor
 *
 */

void 
__ops_writer_set_fd(__ops_output_t *output, int fd)
{
	writer_fd_t	*writer;

	writer = calloc(1, sizeof(*writer));
	writer->fd = fd;
	__ops_writer_set(output, fd_writer, NULL, writer_fd_destroyer, writer);
}

static unsigned 
memory_writer(const unsigned char *src, unsigned length,
	      __ops_error_t **errors,
	      __ops_writer_t *writer)
{
	__ops_memory_t   *mem;

	__OPS_USED(errors);
	mem = __ops_writer_get_arg(writer);
	__ops_memory_add(mem, src, length);
	return 1;
}

/**
 * \ingroup Core_WritersFirst
 * \brief Write to memory
 *
 * Set a memory writer.
 *
 * \param output The output structure
 * \param mem The memory structure
 * \note It is the caller's responsiblity to call __ops_memory_free(mem)
 * \sa __ops_memory_free()
 */

void 
__ops_writer_set_memory(__ops_output_t *output, __ops_memory_t *mem)
{
	__ops_writer_set(output, memory_writer, NULL, NULL, mem);
}

/**************************************************************************/

typedef struct {
	__ops_hash_alg_t hash_alg;
	__ops_hash_t      hash;
	unsigned char  *hashed;
} skey_checksum_t;

static unsigned 
skey_checksum_writer(const unsigned char *src,
	const unsigned length,
	__ops_error_t **errors,
	__ops_writer_t *writer)
{
	skey_checksum_t	*sum;
	unsigned	 ret = 1;

	sum = __ops_writer_get_arg(writer);
	/* add contents to hash */
	sum->hash.add(&sum->hash, src, length);
	/* write to next stacked writer */
	ret = __ops_stacked_write(src, length, errors, writer);
	/* tidy up and return */
	return ret;
}

static unsigned
skey_checksum_finaliser(__ops_error_t **errors, __ops_writer_t *writer)
{
	skey_checksum_t *sum;

	sum = __ops_writer_get_arg(writer);
	if (errors) {
		printf("errors in skey_checksum_finaliser\n");
	}
	sum->hash.finish(&sum->hash, sum->hashed);
	return 1;
}

static void 
skey_checksum_destroyer(__ops_writer_t * writer)
{
	skey_checksum_t *sum;

	sum = __ops_writer_get_arg(writer);
	(void) free(sum);
}

/**
\ingroup Core_WritersNext
\param output
\param seckey
*/
void 
__ops_push_skey_checksum_writer(__ops_output_t *output,
		__ops_seckey_t *seckey)
{
	/* XXX: push a SHA-1 checksum writer (and change s2k to 254). */
	skey_checksum_t *sum;

	sum = calloc(1, sizeof(*sum));
	/* configure the arg */
	sum->hash_alg = seckey->hash_alg;
	sum->hashed = &seckey->checkhash[0];
	/* init the hash */
	__ops_hash_any(&sum->hash, sum->hash_alg);
	sum->hash.init(&sum->hash);
	__ops_writer_push(output, skey_checksum_writer,
		skey_checksum_finaliser, skey_checksum_destroyer, sum);
}

/**************************************************************************/

#define MAX_PARTIAL_DATA_LENGTH 1073741824

typedef struct {
	__ops_crypt_t	*crypt;
	__ops_memory_t	*mem_data;
	__ops_memory_t	*litmem;
	__ops_output_t	*litoutput;
	__ops_memory_t	*se_ip_mem;
	__ops_output_t	*se_ip_out;
	__ops_hash_t	 hash;
} str_enc_se_ip_t;


static unsigned 
str_enc_se_ip_writer(const unsigned char *src,
			    unsigned length,
			    __ops_error_t **errors,
			    __ops_writer_t *writer);

static unsigned 
str_enc_se_ip_finaliser(__ops_error_t ** errors,
			       __ops_writer_t * writer);

static void     str_enc_se_ip_destroyer(__ops_writer_t *writer);

/* */

/**
\ingroup Core_WritersNext
\param output
\param pubkey
*/
void 
__ops_push_stream_enc_se_ip(__ops_output_t *output,
				     const __ops_keydata_t *pubkey)
{
	__ops_pk_sesskey_t	*encrypted_pk_sesskey;
	const unsigned int	 bufsz = 1024;
	str_enc_se_ip_t		*se_ip = calloc(1, sizeof(*se_ip));
	__ops_crypt_t		*encrypted;
	unsigned char		*iv = NULL;

	encrypted_pk_sesskey = __ops_create_pk_sesskey(pubkey);
	__ops_write_pk_sesskey(output, encrypted_pk_sesskey);

	/* Setup the se_ip */
	encrypted = calloc(1, sizeof(*encrypted));
	__ops_crypt_any(encrypted, encrypted_pk_sesskey->symm_alg);
	iv = calloc(1, encrypted->blocksize);
	encrypted->set_iv(encrypted, iv);
	encrypted->set_key(encrypted, &encrypted_pk_sesskey->key[0]);
	__ops_encrypt_init(encrypted);

	se_ip->crypt = encrypted;

	se_ip->mem_data = __ops_memory_new();
	__ops_memory_init(se_ip->mem_data, bufsz);

	se_ip->litmem = NULL;
	se_ip->litoutput = NULL;

	__ops_setup_memory_write(&se_ip->se_ip_out, &se_ip->se_ip_mem, bufsz);

	/* And push writer on stack */
	__ops_writer_push(output,
			str_enc_se_ip_writer,
			str_enc_se_ip_finaliser,
			str_enc_se_ip_destroyer, se_ip);
	/* tidy up */
	(void) free(encrypted_pk_sesskey);
	(void) free(iv);
}


static unsigned int 
__ops_calc_partial_data_length(unsigned int len)
{
	unsigned int    mask = MAX_PARTIAL_DATA_LENGTH;
	int             i;

	if (len == 0) {
		(void) fprintf(stderr,
				"__ops_calc_partial_data_length: 0 len\n");
		return 0;
	}
	if (len > MAX_PARTIAL_DATA_LENGTH) {
		return MAX_PARTIAL_DATA_LENGTH;
	}
	for (i = 0; i <= 30; i++) {
		if (mask & len) {
			break;
		}
		mask >>= 1;
	}
	return mask;
}

static unsigned 
__ops_write_partial_data_length(unsigned int len, __ops_output_t *output)
{
	/* len must be a power of 2 from 0 to 30 */
	unsigned char   c;
	int             i;

	for (i = 0; i <= 30; i++) {
		if ((len >> i) & 1) {
			break;
		}
	}
	c = 224 + i;
	return __ops_write(output, &c, 1);
}

static unsigned 
stream_write_litdata(__ops_output_t *output,
			const unsigned char *data,
			unsigned len)
{
	while (len > 0) {
		size_t          pdlen = __ops_calc_partial_data_length(len);

		__ops_write_partial_data_length(pdlen, output);
		__ops_write(output, data, pdlen);
		data += pdlen;
		len -= pdlen;
	}
	return 1;
}

static          unsigned
stream_write_litdata_first(__ops_output_t *output,
				const unsigned char *data,
				unsigned int len,
				const __ops_litdata_type_t type)
{
	/* \todo add filename  */
	/* \todo add date */
	/* \todo do we need to check text data for <cr><lf> line endings ? */

	size_t          sz_towrite = 1 + 1 + 4 + len;
	size_t          sz_pd = __ops_calc_partial_data_length(sz_towrite);

	if (sz_pd < 512) {
		(void) fprintf(stderr,
			"stream_write_litdata_first: bad sz_pd\n");
		return 0;
	}

	__ops_write_ptag(output, OPS_PTAG_CT_LITERAL_DATA);
	__ops_write_partial_data_length(sz_pd, output);
	__ops_write_scalar(output, (unsigned)type, 1);
	__ops_write_scalar(output, 0, 1);
	__ops_write_scalar(output, 0, 4);
	__ops_write(output, data, sz_pd - 6);

	data += (sz_pd - 6);
	sz_towrite -= sz_pd;

	return stream_write_litdata(output, data, sz_towrite);
}

static unsigned
stream_write_litdata_last(__ops_output_t * output,
				const unsigned char *data,
				unsigned int len)
{
	__ops_write_length(output, len);
	return __ops_write(output, data, len);
}

static unsigned
stream_write_se_ip(__ops_output_t * output,
			const unsigned char *data,
			unsigned int len,
			str_enc_se_ip_t * se_ip)
{
	size_t          pdlen;

	while (len > 0) {
		pdlen = __ops_calc_partial_data_length(len);
		__ops_write_partial_data_length(pdlen, output);

		__ops_writer_push_encrypt_crypt(output, se_ip->crypt);
		__ops_write(output, data, pdlen);
		__ops_writer_pop(output);

		se_ip->hash.add(&se_ip->hash, data, pdlen);

		data += pdlen;
		len -= pdlen;
	}
	return 1;
}

static unsigned
stream_write_se_ip_first(__ops_output_t *output,
			const unsigned char *data,
			unsigned int len,
			str_enc_se_ip_t * se_ip)
{
	unsigned char  *preamble;
	size_t          sz_preamble;
	size_t          sz_towrite;
	size_t          sz_pd;

	sz_preamble = se_ip->crypt->blocksize + 2;
	sz_towrite = sz_preamble + 1 + len;
	preamble = calloc(1, sz_preamble);
	sz_pd = __ops_calc_partial_data_length(sz_towrite);
	if (sz_pd < 512) {
		(void) fprintf(stderr,
			"stream_write_se_ip_first: bad sz_pd\n");
		return 0;
	}

	__ops_write_ptag(output, OPS_PTAG_CT_SE_IP_DATA);
	__ops_write_partial_data_length(sz_pd, output);
	__ops_write_scalar(output, SE_IP_DATA_VERSION, 1);

	__ops_writer_push_encrypt_crypt(output, se_ip->crypt);

	__ops_random(preamble, se_ip->crypt->blocksize);
	preamble[se_ip->crypt->blocksize] =
			preamble[se_ip->crypt->blocksize - 2];
	preamble[se_ip->crypt->blocksize + 1] =
			preamble[se_ip->crypt->blocksize - 1];

	__ops_hash_any(&se_ip->hash, OPS_HASH_SHA1);
	se_ip->hash.init(&se_ip->hash);

	__ops_write(output, preamble, sz_preamble);
	se_ip->hash.add(&se_ip->hash, preamble, sz_preamble);

	__ops_write(output, data, sz_pd - sz_preamble - 1);
	se_ip->hash.add(&se_ip->hash, data, sz_pd - sz_preamble - 1);

	data += (sz_pd - sz_preamble - 1);
	sz_towrite -= sz_pd;

	__ops_writer_pop(output);

	stream_write_se_ip(output, data, sz_towrite, se_ip);

	(void) free(preamble);

	return 1;
}

static unsigned
stream_write_se_ip_last(__ops_output_t * output,
			const unsigned char *data,
			unsigned int len,
			str_enc_se_ip_t * se_ip)
{
	__ops_output_t	*mdcoutput;
	__ops_memory_t	*mdcmem;
	unsigned char	 c;
	unsigned char	 hashed[OPS_SHA1_HASH_SIZE];
	const size_t	 sz_mdc = 1 + 1 + OPS_SHA1_HASH_SIZE;
	size_t		 sz_buf = len + sz_mdc;

	se_ip->hash.add(&se_ip->hash, data, len);

	/* MDC packet tag */
	c = 0xD3;
	se_ip->hash.add(&se_ip->hash, &c, 1);

	/* MDC packet len */
	c = OPS_SHA1_HASH_SIZE;
	se_ip->hash.add(&se_ip->hash, &c, 1);

	/* finish */
	se_ip->hash.finish(&se_ip->hash, hashed);

	__ops_setup_memory_write(&mdcoutput, &mdcmem, sz_mdc);
	__ops_write_mdc(hashed, mdcoutput);

	/* write length of last se_ip chunk */
	__ops_write_length(output, sz_buf);

	/* encode everting */
	__ops_writer_push_encrypt_crypt(output, se_ip->crypt);

	__ops_write(output, data, len);
	__ops_write(output, __ops_mem_data(mdcmem), __ops_mem_len(mdcmem));

	__ops_writer_pop(output);

	__ops_teardown_memory_write(mdcoutput, mdcmem);

	return 1;
}

static unsigned 
str_enc_se_ip_writer(const unsigned char *src,
			    unsigned length,
			    __ops_error_t **errors,
			    __ops_writer_t *writer)
{
	str_enc_se_ip_t	*se_ip = __ops_writer_get_arg(writer);
	unsigned	 ret = 1;

	if (se_ip->litoutput == NULL) {	/* first literal data chunk
						 * is not yet written */
		size_t          datalength;

		__ops_memory_add(se_ip->mem_data, src, length);
		datalength = __ops_mem_len(se_ip->mem_data);

		/* 4.2.2.4. Partial Body Lengths */
		/* The first partial length MUST be at least 512 octets long. */
		if (datalength < 512) {
			return 1;	/* will wait for more data or
						 * end of stream             */
		}
		__ops_setup_memory_write(&se_ip->litoutput,
				&se_ip->litmem, datalength + 32);
		stream_write_litdata_first(se_ip->litoutput,
				__ops_mem_data(se_ip->mem_data),
				datalength,
				OPS_LDT_BINARY);

		stream_write_se_ip_first(se_ip->se_ip_out,
				__ops_mem_data(se_ip->litmem),
				__ops_mem_len(se_ip->litmem), se_ip);
	} else {
		stream_write_litdata(se_ip->litoutput, src, length);
		stream_write_se_ip(se_ip->se_ip_out,
				__ops_mem_data(se_ip->litmem),
				__ops_mem_len(se_ip->litmem), se_ip);
	}

	/* now write memory to next writer */
	ret = __ops_stacked_write(__ops_mem_data(se_ip->se_ip_mem),
				__ops_mem_len(se_ip->se_ip_mem),
				errors, writer);

	__ops_memory_clear(se_ip->litmem);
	__ops_memory_clear(se_ip->se_ip_mem);

	return ret;
}

static unsigned 
str_enc_se_ip_finaliser(__ops_error_t **errors, __ops_writer_t *writer)
{
	str_enc_se_ip_t *se_ip = __ops_writer_get_arg(writer);
	/* write last chunk of data */

	if (se_ip->litoutput == NULL) {
		/* first literal data chunk was not written */
		/* so we know the total length of data, write a simple packet */

		/* create literal data packet from buffered data */
		__ops_setup_memory_write(&se_ip->litoutput, &se_ip->litmem,
				 __ops_mem_len(se_ip->mem_data) + 32);

		__ops_write_litdata(se_ip->litoutput,
			__ops_mem_data(se_ip->mem_data),
			(const int)__ops_mem_len(se_ip->mem_data),
			OPS_LDT_BINARY);

		/* create SE IP packet set from this literal data */
		__ops_write_se_ip_pktset(
				__ops_mem_data(se_ip->litmem),
				__ops_mem_len(se_ip->litmem),
				se_ip->crypt, se_ip->se_ip_out);

	} else {
		/* finish writing */
		stream_write_litdata_last(se_ip->litoutput, NULL, 0);
		stream_write_se_ip_last(se_ip->se_ip_out,
				__ops_mem_data(se_ip->litmem),
				__ops_mem_len(se_ip->litmem), se_ip);
	}

	/* now write memory to next writer */
	return __ops_stacked_write(__ops_mem_data(se_ip->se_ip_mem),
				 __ops_mem_len(se_ip->se_ip_mem),
				 errors, writer);
}

static void 
str_enc_se_ip_destroyer(__ops_writer_t *writer)
{
	str_enc_se_ip_t *se_ip = __ops_writer_get_arg(writer);

	__ops_memory_free(se_ip->mem_data);
	__ops_teardown_memory_write(se_ip->litoutput, se_ip->litmem);
	__ops_teardown_memory_write(se_ip->se_ip_out, se_ip->se_ip_mem);

	se_ip->crypt->decrypt_finish(se_ip->crypt);

	(void) free(se_ip->crypt);
	(void) free(se_ip);
}
