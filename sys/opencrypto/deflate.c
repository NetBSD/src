/*	$NetBSD: deflate.c,v 1.11.14.1 2009/05/13 17:22:56 jym Exp $ */
/*	$FreeBSD: src/sys/opencrypto/deflate.c,v 1.1.2.1 2002/11/21 23:34:23 sam Exp $	*/
/* $OpenBSD: deflate.c,v 1.3 2001/08/20 02:45:22 hugh Exp $ */

/*
 * Copyright (c) 2001 Jean-Jacques Bernard-Gundol (jj@wabbitt.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file contains a wrapper around the deflate algo compression
 * functions using the zlib library (see net/zlib.{c,h})
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: deflate.c,v 1.11.14.1 2009/05/13 17:22:56 jym Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <net/zlib.h>

#include <opencrypto/cryptodev.h>
#include <opencrypto/deflate.h>


int window_inflate = -1 * MAX_WBITS;
int window_deflate = -12;

/*
 * This function takes a block of data and (de)compress it using the deflate
 * algorithm
 */

static void *
ocf_zalloc(void *nil, u_int type, u_int size)
{
	void *ptr;

	ptr = malloc(type *size, M_CRYPTO_DATA, M_NOWAIT);
	return ptr;
}

static void
ocf_zfree(void *nil, void *ptr)
{
	free(ptr, M_CRYPTO_DATA);
}

u_int32_t
deflate_global(u_int8_t *data, u_int32_t size, int decomp, u_int8_t **out)
{
	/* decomp indicates whether we compress (0) or decompress (1) */

	z_stream zbuf;
	u_int8_t *output;
	u_int32_t count, result;
	int error, i = 0, j;
	struct deflate_buf *buf, *tmp;
	size_t len, old_len;

	DPRINTF(("deflate_global: size %d\n", size));

	len = ZBUF;
	buf = malloc(len*sizeof(struct deflate_buf), M_CRYPTO_DATA, M_NOWAIT);
	if (buf == NULL)
		return 0;

	memset(&zbuf, 0, sizeof(z_stream));
	for (j = 0; j < len; j++)
		buf[j].flag = 0;

	zbuf.next_in = data;	/* data that is going to be processed */
	zbuf.zalloc = ocf_zalloc;
	zbuf.zfree = ocf_zfree;
	zbuf.opaque = Z_NULL;
	zbuf.avail_in = size;	/* Total length of data to be processed */

	if (!decomp) {
		buf[i].out = malloc(size, M_CRYPTO_DATA, M_NOWAIT);
		if (buf[i].out == NULL)
			goto bad;
		buf[i].size = size;
		buf[i].flag = 1;
		i++;
	} else {
		/*
	 	 * Choose a buffer with 4x the size of the input buffer
	 	 * for the size of the output buffer in the case of
	 	 * decompression. If it's not sufficient, it will need to be
	 	 * updated while the decompression is going on
	 	 */

		buf[i].size = size * 4;
		buf[i].out = malloc(buf[i].size, M_CRYPTO_DATA, M_NOWAIT);
		if (buf[i].out == NULL)
			goto bad;
		buf[i].flag = 1;
		i++;
	}

	zbuf.next_out = buf[0].out;
	zbuf.avail_out = buf[0].size;

	error = decomp ? inflateInit2(&zbuf, window_inflate) :
	    deflateInit2(&zbuf, Z_DEFAULT_COMPRESSION, Z_METHOD,
		    window_deflate, Z_MEMLEVEL, Z_DEFAULT_STRATEGY);

	if (error != Z_OK)
		goto bad;
	for (;;) {
		error = decomp ? inflate(&zbuf, Z_PARTIAL_FLUSH) :
				 deflate(&zbuf, Z_PARTIAL_FLUSH);
		if (error != Z_OK && error != Z_STREAM_END)
			goto bad;
		else if (zbuf.avail_in == 0 && zbuf.avail_out != 0)
			goto end;
		else if (zbuf.avail_out == 0) {
			if (i == (len-1)) {
				old_len = i;
				len += ZBUF;
				tmp = realloc(buf,len*sizeof(struct deflate_buf),
							  M_CRYPTO_DATA, M_NOWAIT);
				if (tmp == NULL)
					goto bad;
				buf = tmp;
				for (j = old_len; j < len; j++)
					buf[j].flag = 0;
			}
			/* we need more output space, allocate size */
			buf[i].out = malloc(size, M_CRYPTO_DATA, M_NOWAIT);
			if (buf[i].out == NULL)
				goto bad;
			zbuf.next_out = buf[i].out;
			buf[i].size = size;
			buf[i].flag = 1;
			zbuf.avail_out = buf[i].size;
			i++;
		} else
			goto bad;
	}

end:
	result = count = zbuf.total_out;

	*out = malloc(result, M_CRYPTO_DATA, M_NOWAIT);
	if (*out == NULL)
		goto bad;
	if (decomp)
		inflateEnd(&zbuf);
	else
		deflateEnd(&zbuf);
	output = *out;
	for (j = 0; buf[j].flag != 0; j++) {
		if (count > buf[j].size) {
			memcpy(*out, buf[j].out, buf[j].size);
			*out += buf[j].size;
			free(buf[j].out, M_CRYPTO_DATA);
			count -= buf[j].size;
		} else {
			/* it should be the last buffer */
			memcpy(*out, buf[j].out, count);
			*out += count;
			free(buf[j].out, M_CRYPTO_DATA);
			count = 0;
		}
	}
	free(buf, M_CRYPTO_DATA);
	*out = output;
	return result;

bad:
	*out = NULL;
	for (j = 0; buf[j].flag != 0; j++)
		free(buf[j].out, M_CRYPTO_DATA);
	free(buf, M_CRYPTO_DATA);
	if (decomp)
		inflateEnd(&zbuf);
	else
		deflateEnd(&zbuf);
	return 0;
}

/*
 * Initial version will perform a single gzip encapsulation,
 * filling in the header,
 * and appending the crc and uncompressed length.
 *
 * Later version will support multiple buffers with 
 * a flag indication final buffer.  The crc is maintained
 * over all buffers and appended to the output along with 
 * the uncompressed length after the final data buffer
 * has been compressed and output.
 *
 * Ditto for uncompress - CRC is extracted from the final packed
 * and compared against CRC of uncompressed data.
 *
 */

/* constant header for the gzip */
static const char gzip_header[10] = {
	0x1f, 0x8b, 	/* ID1 ID2	*/
	Z_DEFLATED, 	/* CM		*/
	0,		/* FLG		*/
	0, 0, 0, 0,	/* MTIME	*/
	0,		/* XFL		*/
	0x03		/* OS (Unix)	*/
};

/* Followed by compressed payload */
/* Followed by uint32_t CRC32 and uint32_t ISIZE */
#define GZIP_TAIL_SIZE	8

u_int32_t
gzip_global(u_int8_t *data, u_int32_t size,
	int decomp, u_int8_t **out)
{
	/* decomp indicates whether we compress (0) or decompress (1) */
	z_stream zbuf;
	u_int8_t *output;
	u_int32_t count, result;
	int error, i = 0, j;
	struct deflate_buf *buf, *tmp;
	size_t nbufs, old_nbufs;
	u_int32_t crc;
	u_int32_t isize;

	DPRINTF(("gzip_global: decomp %d, size %d\n", decomp, size));

	nbufs = ZBUF;
	buf = malloc(nbufs*sizeof(struct deflate_buf), M_CRYPTO_DATA, M_NOWAIT);
	if (buf == NULL) {
		DPRINTF(("gzip_global.%d: failed to malloc %d\n",
				__LINE__, nbufs*sizeof(struct deflate_buf)));
		return 0;
	}

	memset(&zbuf, 0, sizeof(z_stream));
	for (j = 0; j < nbufs; j++)
		buf[j].flag = 0;

	zbuf.zalloc = ocf_zalloc;
	zbuf.zfree = ocf_zfree;
	zbuf.opaque = Z_NULL;

	crc = crc32(0, NULL, 0);	/* get initial crc value */

	zbuf.avail_in = size;	/* Total length of data to be processed */
	zbuf.next_in = data;	/* data that is going to be processed */

	if (!decomp) {
		/* compress */
		DPRINTF(("gzip_global: compress[%d] malloc %d + %d + %d = %d\n",
				i, size, sizeof(gzip_header), GZIP_TAIL_SIZE,
				size + sizeof(gzip_header) + GZIP_TAIL_SIZE));

		buf[i].out = malloc(size, M_CRYPTO_DATA, M_NOWAIT);
		if (buf[i].out == NULL)
			goto bad2;
		buf[i].size = size;
		buf[i].flag = 1;

		zbuf.next_out = buf[i].out;
		zbuf.avail_out = buf[i].size;
		i++;
		
		crc = crc32(crc, data, size);
		DPRINTF(("gzip_compress: size %d, crc 0x%x\n", size, crc));
	} else {
		/* decompress */
		/* check the gzip header */
		if (zbuf.avail_in <= 0) {
			/* Not enough data for the header & tail */
			DPRINTF(("gzip_global: not enough data (%d)\n",
					size));
			goto bad2;
		}

		/* XXX this is pretty basic,
		 * needs to be expanded to ignore MTIME, OS, 
		 * but still ensure flags are 0.
		 * Q. Do we need to support the flags and 
		 * optional header fields? Likely.
		 * XXX add flag and field support too.
		 */
		if (memcmp(data, gzip_header, sizeof(gzip_header)) != 0) {
			DPRINTF(("gzip_global: unsupported gzip header (%02x%02x)\n",
					data[0], data[1]));
			goto bad2;
		} else {
			DPRINTF(("gzip_global.%d: gzip header ok\n",__LINE__));
		}

		isize = *((uint32_t *)&data[size-sizeof(uint32_t)]);

		DPRINTF(("gzip_global: isize = %d (%02x %02x %02x %02x)\n",
				isize,
				data[size-4],
				data[size-3],
				data[size-2],
				data[size-1]));

		buf[i].size = isize;
		buf[i].out = malloc(buf[i].size, M_CRYPTO_DATA, M_NOWAIT);
		if (buf[i].out == NULL)
			goto bad2;
		buf[i].flag = 1;
		zbuf.next_out = buf[i].out;
		zbuf.avail_out = buf[i].size;
		i++;

		/* skip over the gzip header */
		zbuf.next_in = data + sizeof(gzip_header);

		/* actual payload size stripped of gzip header and tail */
		zbuf.avail_in = size - sizeof(gzip_header) - GZIP_TAIL_SIZE;
		DPRINTF(("zbuf avail_in %d, avail_out %d\n",
					zbuf.avail_in, zbuf.avail_out));

	}


	error = decomp ? inflateInit2(&zbuf, window_inflate) :
	    deflateInit2(&zbuf, Z_DEFAULT_COMPRESSION, Z_METHOD,
		    window_deflate, Z_MEMLEVEL, Z_DEFAULT_STRATEGY);

	if (error != Z_OK) {
		printf("deflateInit2() failed\n");
		goto bad;
	}
	for (;;) {
		DPRINTF(("pre: %s in:%d out:%d\n", decomp ? "deflate()" : "inflate()", 
				zbuf.avail_in, zbuf.avail_out));
		error = decomp ? inflate(&zbuf, Z_PARTIAL_FLUSH) :
				 deflate(&zbuf, Z_PARTIAL_FLUSH);
		DPRINTF(("post: %s in:%d out:%d\n", decomp ? "deflate()" : "inflate()", 
				zbuf.avail_in, zbuf.avail_out));
		if (error != Z_OK && error != Z_STREAM_END) {
			printf("deflate() or inflate() failed, error=%d\n", error);
			goto bad;
		} else if (zbuf.avail_in == 0 && zbuf.avail_out != 0) {
			DPRINTF(("gzip_global: avail_in == 0, ending\n"));
			goto end;
		} else if (zbuf.avail_in == 0 && zbuf.avail_out == 0) {
			DPRINTF(("gzip_global: avail_in == 0, avail_out == 0, ending\n"));
			goto end;
		} else if (zbuf.avail_out == 0) {
			if (i == (nbufs-1)) {
				old_nbufs = i;
				nbufs += ZBUF;
				tmp = realloc(buf,nbufs*sizeof(struct deflate_buf),
							  M_CRYPTO_DATA, M_NOWAIT);
				if (tmp == NULL)
					goto bad;
				buf = tmp;
				for (j = old_nbufs; j < nbufs; j++)
					buf[j].flag = 0;
			}
			/* we need more output space, allocate size */
			buf[i].out = malloc(size, M_CRYPTO_DATA, M_NOWAIT);
			if (buf[i].out == NULL)
				goto bad;
			zbuf.next_out = buf[i].out;
			buf[i].size = size;
			buf[i].flag = 1;
			zbuf.avail_out = buf[i].size;
			i++;
		} else
			goto bad;
	}

end:
	if (decomp) {
		count = result = zbuf.total_out;
	} else {
		/* need room for header, CRC, and ISIZE */
		result = zbuf.total_out + sizeof(gzip_header) + GZIP_TAIL_SIZE;
		count = zbuf.total_out;
	}

	DPRINTF(("gzip_global: in %d -> out %d\n", size, result));

	*out = malloc(result, M_CRYPTO_DATA, M_NOWAIT);
	if (*out == NULL)
		goto bad;
	output = *out;
	if (decomp)
		inflateEnd(&zbuf);
	else {
		deflateEnd(&zbuf);

		/* fill in gzip header */
		memcpy(output, gzip_header, sizeof(gzip_header));
		output += sizeof(gzip_header);
	}
	for (j = 0; buf[j].flag != 0; j++) {
		if (decomp) {
			/* update crc for decompressed data */
			crc = crc32(crc, buf[j].out, buf[j].size);
		}
		if (count > buf[j].size) {
			memcpy(output, buf[j].out, buf[j].size);
			output += buf[j].size;
			free(buf[j].out, M_CRYPTO_DATA);
			count -= buf[j].size;
		} else {
			/* it should be the last buffer */
			memcpy(output, buf[j].out, count);
			output += count;
			free(buf[j].out, M_CRYPTO_DATA);
			count = 0;
		}
	}
	free(buf, M_CRYPTO_DATA);

	if (!decomp) {
		/* fill in CRC and ISIZE */
		((uint32_t *)output)[0] = crc;
		((uint32_t *)output)[1] = size;

		DPRINTF(("gzip_global: size = 0x%x (%02x %02x %02x %02x)\n",
				size,
				output[7],
				output[3],
				output[5],
				output[4]));
	}

	return result;

bad:
	if (decomp)
		inflateEnd(&zbuf);
	else
		deflateEnd(&zbuf);
bad2:
	*out = NULL;
	for (j = 0; buf[j].flag != 0; j++)
		free(buf[j].out, M_CRYPTO_DATA);
	free(buf, M_CRYPTO_DATA);
	return 0;
}
