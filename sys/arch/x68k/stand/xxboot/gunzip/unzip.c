/*	$NetBSD: unzip.c,v 1.2 2001/06/12 16:57:29 minoura Exp $	*/

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
#if defined(__GNUC__) && defined(__m68k__)
#ifdef __ELF__
   asm("lea	outcnt,%a0\n\
	movel	%a0@,%d0\n\
	addl	%d0,udst_cnt\n\
	movel	%d0,%sp@-\n\
	pea	window\n\
	lea	udst,%a1\n\
	movel	%a1@,%sp@-\n\
	addl	%d0,%a1@\n\
	clrl	%a0@\n\
	jbsr	memcpy\n\
	lea	%sp@(12),%sp");
#else
   asm("lea	_outcnt,%a0\n\
	movel	%a0@,%d0\n\
	addl	%d0,_udst_cnt\n\
	movel	%d0,%sp@-\n\
	pea	_window\n\
	lea	_udst,%a1\n\
	movel	%a1@,%sp@-\n\
	addl	%d0,%a1@\n\
	clrl	%a0@\n\
	jbsr	_memcpy\n\
	lea	%sp@(12),%sp");
#endif
#else
    register unsigned cnt = outcnt;

    udst_cnt += cnt;
    memcpy(udst, window, cnt);
    udst += cnt;
    outcnt = 0;
#endif
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
long
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
	/* NOTREACHED */
    }

    /* check flags */
    flags = *src++;
    if (flags &
	  (GZIP_FLAG_MULTIPART | GZIP_FLAG_ENCRYPTED | GZIP_FLAGS_RESERVED)) {
	error("unsupported compression flag");
	/* NOTREACHED */
    }

    /* skip mod time (4bytes), extra flags, operating system */
    src += (4 + 1 + 1);

    /* part number (2bytes) may exist here but not supported */

    /* skip extra field if present */
    if (flags & GZIP_FLAG_EXTRA) {
	/* its length (LSB first) */
	unsigned extralen;
#if defined(__GNUC__) && defined(__m68k__)
	/*
	 * This doesn't work on 68000/010 since it does an unaligned access.
	 */
   asm("movew	%1@+,%0\n\
	rolw	#8,%0" : "=d" (extralen), "=a" (src) : "0" (0), "1" (src));
#else
	extralen = *src++;
	extralen += (*src++ << 8);
#endif
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
	/* NOTREACHED */
    default:
    invdata:
	error("invalid compressed data");
	/* NOTREACHED */
    }

    src = csrc;

    /* 32bit CRC is not checked --- skip */
    src += 4;

    /* check uncompressed input size (LSB first) */
    {
	uint32 len;

#if defined(__GNUC__) && defined(__m68k__)
	/*
	 * I think this is the smallest for MC68020 and later
	 * This doesn't work on 68000/010 since it does an unaligned access.
	 */
   asm("rolw	#8,%0\n\
	swap	%0\n\
	rolw	#8,%0" : "=d" (len) : "0" (*(uint32 *)src));
#else
	len = *src++;
	len += *src++ << 8;
	len += *src++ << 16;
	len += *src << 24;
#endif
	if (len != (uint32) udst_cnt)
	    goto invdata;
    }

    return udst_cnt;
}
