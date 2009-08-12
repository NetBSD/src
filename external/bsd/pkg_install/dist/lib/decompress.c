/*	$NetBSD: decompress.c,v 1.1.1.1.8.1 2009/05/30 16:21:37 snj Exp $	*/

/*-
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <nbcompat.h>

#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

__RCSID("$NetBSD: decompress.c,v 1.1.1.1.8.1 2009/05/30 16:21:37 snj Exp $");

#ifdef BOOTSTRAP
#include "lib.h"

int
decompress_buffer(const char *input, size_t input_len, char **output,
    size_t *output_len)
{
	return 0;
}

#else

#include <bzlib.h>
#if HAVE_ERR_H
#include <err.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <zlib.h>

#include "lib.h"

static void
decompress_bzip2(const char *in, size_t in_len, char **out, size_t *out_len)
{
	bz_stream stream;
	size_t output_produced;

	if (in_len < SSIZE_MAX / 10)
		*out_len = in_len * 10;
	else
		*out_len = in_len;
	*out = xmalloc(*out_len + 1);

	stream.next_in = (char *)in;
	stream.avail_in = in_len;
	stream.next_out = *out;
	stream.avail_out = *out_len;
	output_produced = 0;
	stream.bzalloc = NULL;
	stream.bzfree = NULL;
	stream.opaque = NULL;

	if (BZ2_bzDecompressInit(&stream, 0, 0) != BZ_OK)
		errx(EXIT_FAILURE, "BZ2_bzDecompressInit failed");

	for (;;) {
		switch (BZ2_bzDecompress(&stream)) {
		case BZ_STREAM_END:
			if (BZ2_bzDecompressEnd(&stream) != Z_OK)
				errx(EXIT_FAILURE, "inflateEnd failed");
			output_produced = *out_len - stream.avail_out;
			*out = xrealloc(*out, output_produced + 1);
			*out_len = output_produced;
			(*out)[*out_len] = '\0';
			return;
		case BZ_OK:
			output_produced = *out_len - stream.avail_out;
			if (*out_len <= SSIZE_MAX / 2)
				*out_len *= 2;
			else
				errx(EXIT_FAILURE, "input too large");
			*out = xrealloc(*out, *out_len + 1);
			stream.next_out = *out + output_produced;
			stream.avail_out = *out_len - output_produced;
			break;
		default:
			errx(EXIT_FAILURE, "inflate failed");
		}
	}
}

static void
decompress_zlib(const char *in, size_t in_len, char **out, size_t *out_len)
{
	z_stream stream;
	size_t output_produced;

	if (in_len < SSIZE_MAX / 10)
		*out_len = in_len * 10;
	else
		*out_len = in_len;
	*out = xmalloc(*out_len + 1);

	stream.next_in = (unsigned char *)in;
	stream.avail_in = in_len;
	stream.next_out = (unsigned char *)*out;
	stream.avail_out = *out_len;
	output_produced = 0;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = NULL;

	if (inflateInit2(&stream, 47) != Z_OK)
		errx(EXIT_FAILURE, "inflateInit failed");

	for (;;) {
		switch (inflate(&stream, Z_FINISH)) {
		case Z_STREAM_END:
			if (inflateEnd(&stream) != Z_OK)
				errx(EXIT_FAILURE, "inflateEnd failed");
			output_produced = *out_len - stream.avail_out;
			*out = xrealloc(*out, output_produced + 1);
			*out_len = output_produced;
			(*out)[*out_len] = '\0';
			return;
		case Z_OK:
			output_produced = *out_len - stream.avail_out;
			if (*out_len < SSIZE_MAX / 2)
				*out_len *= 2;
			else if (*out_len == SSIZE_MAX - 1)
				errx(EXIT_FAILURE, "input too large");
			else
				*out_len = SSIZE_MAX - 1;
			*out = xrealloc(*out, *out_len + 1);
			stream.next_out = (unsigned char *)*out + output_produced;
			stream.avail_out = *out_len - output_produced;
			break;
		default:
			errx(EXIT_FAILURE, "inflate failed");
		}
	}
}

int
decompress_buffer(const char *input, size_t input_len, char **output,
    size_t *output_len)
{
	if (input_len < 4)
		return 0;
	if (input[0] == 'B' && input[1] == 'Z' && input[2] == 'h' &&
	    input[3] >= '1' && input[3] <= '9') {
		/* Normal bzip2. */
		decompress_bzip2(input, input_len, output, output_len);
	} else if (input[0] == 037 && (unsigned char)input[1] == 139 &&
	    input[2] == 8 && (input[3] & 0xe0) == 0) {
		/* gzip header with Deflate method */
		decompress_zlib(input, input_len, output, output_len);
	} else /* plain text */
		return 0;
	return 1;
}
#endif /* BOOTSTRAP */
