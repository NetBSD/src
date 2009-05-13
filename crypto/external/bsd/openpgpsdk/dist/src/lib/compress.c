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

#include <zlib.h>
#include <bzlib.h>
#include <assert.h>
#include <string.h>

#include <openpgpsdk/compress.h>
#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/errors.h>
#include "parse_local.h"
#include <openpgpsdk/final.h>

#define DECOMPRESS_BUFFER	1024

typedef struct {
	ops_compression_type_t type;
	ops_region_t   *region;
	unsigned char   in[DECOMPRESS_BUFFER];
	unsigned char   out[DECOMPRESS_BUFFER];
	z_stream        zstream;/* ZIP and ZLIB */
	bz_stream       bzstream;	/* BZIP2 */
	size_t          offset;
	int             inflate_ret;
}               decompress_arg_t;

typedef struct {
	z_stream        stream;
	unsigned char  *src;
	unsigned char  *dst;
}               compress_arg_t;

/*
 * \todo remove code duplication between this and
 * bzip2_compressed_data_reader
 */
static int 
zlib_compressed_data_reader(void *dest, size_t length,
			    ops_error_t ** errors,
			    ops_reader_info_t * rinfo,
			    ops_parse_cb_info_t * cbinfo)
{
	decompress_arg_t *arg = ops_reader_get_arg(rinfo);

	/* ops_parser_content_t content; */
	int             saved = length;

	assert(arg->type == OPS_C_ZIP || arg->type == OPS_C_ZLIB);

	if ( /* arg->region->indeterminate && */ arg->inflate_ret == Z_STREAM_END
	    && arg->zstream.next_out == &arg->out[arg->offset])
		return 0;

	if (arg->region->length_read == arg->region->length) {
		if (arg->inflate_ret != Z_STREAM_END)
			OPS_ERROR(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR, "Compressed data didn't end when region ended.");
		/*
	          else
	          return 0;
	          www.zlib.org
	        */
	}
	while (length > 0) {
		size_t          len;

		if (&arg->out[arg->offset] == arg->zstream.next_out) {
			int             ret;

			arg->zstream.next_out = arg->out;
			arg->zstream.avail_out = sizeof(arg->out);
			arg->offset = 0;
			if (arg->zstream.avail_in == 0) {
				unsigned        n = arg->region->length;

				if (!arg->region->indeterminate) {
					n -= arg->region->length_read;
					if (n > sizeof(arg->in))
						n = sizeof(arg->in);
				} else
					n = sizeof(arg->in);

				if (!ops_stacked_limited_read(arg->in, n, arg->region,
						     errors, rinfo, cbinfo))
					return -1;

				arg->zstream.next_in = arg->in;
				arg->zstream.avail_in = arg->region->indeterminate
					? arg->region->last_read : n;
			}
			ret = inflate(&arg->zstream, Z_SYNC_FLUSH);
			if (ret == Z_STREAM_END) {
				if (!arg->region->indeterminate
				    && arg->region->length_read != arg->region->length)
					OPS_ERROR(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR, "Compressed stream ended before packet end.");
			} else if (ret != Z_OK) {
				fprintf(stderr, "ret=%d\n", ret);
				OPS_ERROR(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR, arg->zstream.msg);
			}
			arg->inflate_ret = ret;
		}
		assert(arg->zstream.next_out > &arg->out[arg->offset]);
		len = arg->zstream.next_out - &arg->out[arg->offset];
		if (len > length)
			len = length;
		memcpy(dest, &arg->out[arg->offset], len);
		arg->offset += len;
		length -= len;
	}

	return saved;
}

static int 
bzip2_compressed_data_reader(void *dest, size_t length,
			     ops_error_t ** errors,
			     ops_reader_info_t * rinfo,
			     ops_parse_cb_info_t * cbinfo)
{
	decompress_arg_t *arg = ops_reader_get_arg(rinfo);

	/* ops_parser_content_t content; */
	size_t          saved = length;

	assert(arg->type == OPS_C_BZIP2);

	if (arg->inflate_ret == BZ_STREAM_END &&
	    arg->bzstream.next_out == (char *)&arg->out[arg->offset])
		return 0;

	if (arg->region->length_read == arg->region->length) {
		if (arg->inflate_ret != BZ_STREAM_END)
			OPS_ERROR(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR,
			"Compressed data didn't end when region ended.");
	}
	while (length > 0) {
		size_t          len;

		if ((char *)&arg->out[arg->offset] == arg->bzstream.next_out) {
			int             ret;

			arg->bzstream.next_out = (char *) arg->out;
			arg->bzstream.avail_out = sizeof(arg->out);
			arg->offset = 0;
			if (arg->bzstream.avail_in == 0) {
				unsigned        n = arg->region->length;

				if (!arg->region->indeterminate) {
					n -= arg->region->length_read;
					if (n > sizeof(arg->in))
						n = sizeof(arg->in);
				} else
					n = sizeof(arg->in);

				if (!ops_stacked_limited_read(
						(unsigned char *) arg->in,
						n, arg->region,
						errors, rinfo, cbinfo))
					return -1;

				arg->bzstream.next_in = (char *)arg->in;
				arg->bzstream.avail_in = arg->region->indeterminate
					? arg->region->last_read : n;
			}
			ret = BZ2_bzDecompress(&arg->bzstream);
			if (ret == BZ_STREAM_END) {
				if (!arg->region->indeterminate &&
				    arg->region->length_read != arg->region->length)
					OPS_ERROR(cbinfo->errors,
						OPS_E_P_DECOMPRESSION_ERROR,
						"Compressed stream ended before packet end.");
			} else if (ret != BZ_OK) {
				OPS_ERROR_1(cbinfo->errors,
					OPS_E_P_DECOMPRESSION_ERROR,
					"Invalid return %d from BZ2_bzDecompress", ret);
			}
			arg->inflate_ret = ret;
		}
		assert(arg->bzstream.next_out > (char *)&arg->out[arg->offset]);
		len = (size_t)(arg->bzstream.next_out -
				(char *)&arg->out[arg->offset]);
		if (len > length)
			len = length;
		memcpy(dest, &arg->out[arg->offset], len);
		arg->offset += len;
		length -= len;
	}

	return saved;
}

/**
 * \ingroup Core_Compress
 *
 * \param *region 	Pointer to a region
 * \param *parse_info 	How to parse
 * \param type Which compression type to expect
*/

int 
ops_decompress(ops_region_t * region, ops_parse_info_t * parse_info,
	       ops_compression_type_t type)
{
	decompress_arg_t arg;
	int             ret;

	switch (type) {
	case OPS_C_ZIP:
	case OPS_C_ZLIB:
		memset(&arg, '\0', sizeof(arg));

		arg.region = region;
		arg.offset = 0;
		arg.type = type;

		arg.zstream.next_in = Z_NULL;
		arg.zstream.avail_in = 0;
		arg.zstream.next_out = arg.out;
		arg.zstream.zalloc = Z_NULL;
		arg.zstream.zfree = Z_NULL;
		arg.zstream.opaque = Z_NULL;
		break;

	case OPS_C_BZIP2:
		memset(&arg, '\0', sizeof(arg));

		arg.region = region;
		arg.offset = 0;
		arg.type = type;

		arg.bzstream.next_in = NULL;
		arg.bzstream.avail_in = 0;
		arg.bzstream.next_out = (char *)arg.out;
		arg.bzstream.bzalloc = NULL;
		arg.bzstream.bzfree = NULL;
		arg.bzstream.opaque = NULL;
		break;

	default:
		OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG, "Compression algorithm %d is not yet supported", type);
		return 0;
	}

	switch (type) {
	case OPS_C_ZIP:
		ret = inflateInit2(&arg.zstream, -15);
		break;

	case OPS_C_ZLIB:
		ret = inflateInit(&arg.zstream);
		break;

	case OPS_C_BZIP2:
		/*
	        OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG, "Compression algorithm %s is not yet supported", "BZIP2");
	        return 0;
	        */
		ret = BZ2_bzDecompressInit(&arg.bzstream, 1, 0);
		break;

	default:
		OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG, "Compression algorithm %d is not yet supported", type);
		return 0;
	}

	switch (type) {
	case OPS_C_ZIP:
	case OPS_C_ZLIB:
		if (ret != Z_OK) {
			OPS_ERROR_1(&parse_info->errors, OPS_E_P_DECOMPRESSION_ERROR, "Cannot initialise ZIP or ZLIB stream for decompression: error=%d", ret);
			return 0;
		}
		ops_reader_push(parse_info, zlib_compressed_data_reader, NULL, &arg);
		break;

	case OPS_C_BZIP2:
		if (ret != BZ_OK) {
			OPS_ERROR_1(&parse_info->errors, OPS_E_P_DECOMPRESSION_ERROR, "Cannot initialise BZIP2 stream for decompression: error=%d", ret);
			return 0;
		}
		ops_reader_push(parse_info, bzip2_compressed_data_reader, NULL, &arg);
		break;

	default:
		OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG, "Compression algorithm %d is not yet supported", type);
		return 0;
	}

	ret = ops_parse(parse_info);

	ops_reader_pop(parse_info);

	return ret;
}

/**
\ingroup Core_WritePackets
\brief Writes Compressed packet
\param data Data to write out
\param len Length of data
\param cinfo Write settings
\return true if OK; else false
*/

bool 
ops_write_compressed(const unsigned char *data,
		     const unsigned int len,
		     ops_create_info_t * cinfo)
{
	int             r = 0;
	int             sz_in = 0;
	int             sz_out = 0;
	compress_arg_t *compression = calloc(1, sizeof(compress_arg_t));

	/* compress the data */
	const int       level = Z_DEFAULT_COMPRESSION;	/* \todo allow varying
							 * levels */
	compression->stream.zalloc = Z_NULL;
	compression->stream.zfree = Z_NULL;
	compression->stream.opaque = NULL;

	/* all other fields set to zero by use of calloc */

	if (deflateInit(&compression->stream, level) != Z_OK) {
		/* can't initialise */
		assert(0);
	}
	/* do necessary transformation */
	/* copy input to maintain const'ness of src */
	assert(compression->src == NULL);
	assert(compression->dst == NULL);

	sz_in = len * sizeof(unsigned char);
	sz_out = (sz_in * 1.01) + 12;	/* from zlib webpage */
	compression->src = calloc(1, sz_in);
	compression->dst = calloc(1, sz_out);
	memcpy(compression->src, data, len);

	/* setup stream */
	compression->stream.next_in = compression->src;
	compression->stream.avail_in = sz_in;
	compression->stream.total_in = 0;

	compression->stream.next_out = compression->dst;
	compression->stream.avail_out = sz_out;
	compression->stream.total_out = 0;

	r = deflate(&compression->stream, Z_FINISH);
	assert(r == Z_STREAM_END);	/* need to loop if not */

	/* write it out */
	return (ops_write_ptag(OPS_PTAG_CT_COMPRESSED, cinfo)
		&& ops_write_length(1 + compression->stream.total_out, cinfo)
		&& ops_write_scalar(OPS_C_ZLIB, 1, cinfo)
		&& ops_write(compression->dst, compression->stream.total_out, cinfo));
}
