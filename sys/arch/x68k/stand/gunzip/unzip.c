/*
 *	Extract gzip'ed data on the memory
 *
 *	Written by Yasha (ITOH Yasufumi)
 *	This code is in the public domain.
 */

#include <string.h>
#include "gzip.h"

const uch *csrc;	/* compressed data */
uch *udst;		/* uncompressed data */
long udst_cnt;		/* output byte counter */

unsigned outcnt;	/* data length to be flushed */

void
flush_window()
{
    register unsigned cnt = outcnt;

    udst_cnt += cnt;
    memcpy(udst, window, cnt);
    udst += cnt;
    outcnt = 0;
}

#ifndef TEST
#include "inflate.c"
#endif

/*
 * decompress gzip'ed data
 *
 * src: compressed data area for input
 * dst: decompressed data area for output
 */
int
unzip(src, dst)
  const uch *src;
  uch *dst;
{
    uch flags;

    udst_cnt = 0;
    udst = dst;

    /* skip magic (must be checked before) */
    src += 2;

    /* check compression method */
    if (*src++ != GZIP_METHOD_DEFLATED) {
	error("unknown compression method");
	return 1;
    }

    /* check flags */
    flags = *src++;
    if (flags &
	  (GZIP_FLAG_MULTIPART | GZIP_FLAG_ENCRYPTED | GZIP_FLAGS_RESERVED)) {
	error("unsupported compression flag");
	return 1;
    }

    /* skip mod time (4bytes), extra flags, operating system */
    src += (4 + 1 + 1);

    /* part number (2bytes) may exist here but not supported */

    /* skip extra field if present */
    if (flags & GZIP_FLAG_EXTRA) {
	/* its length (LSB first) */
	unsigned extralen;
	extralen = *src++;
	extralen += (*src++ << 8);
	src += extralen;
    }

    /* skip original file name if present */
    if (flags & GZIP_FLAG_FILE_NAME)
	while (*src++)
	    ;

    /* skip file comment if present */
    if (flags & GZIP_FLAG_COMMENT)
	while (*src++)
	    ;

    /* encryption header (12bytes) may exist here but not supported */

    /*
     * Here must exist compressed data...
     */
    csrc = src;

    switch (inflate()) {
    case 0:
	/* no error */
	break;
    case 3:
	error("out of memory");
	return 1;
    default:
    invdata:
	error("invalid compressed data");
	return 1;
    }

    src = csrc;

    /* 32bit CRC is not checked --- skip */
    src += 4;

    /* check uncompressed input size (LSB first) */
    {
	uint32 len;

	len = *src++;
	len += *src++ << 8;
	len += *src++ << 16;
	len += *src << 24;
	if (len != (uint32) udst_cnt)
	    goto invdata;
    }

    return 0;
}
