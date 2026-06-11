/*	$NetBSD: util.c,v 1.1 2026/06/11 08:29:49 rumble Exp $	*/

/*
 * Cribbed from 4.4BSD/Net2 and NetBSD 4.99.42 with some stuff ripped out and
 * cleaned up.
 */

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "util.h"

/* 'thecrc' must initially be 0. */
int
sum1(int thecrc, u_char *buf, uint32_t len)
{
	/*
	 * 16-bit checksum, rotating right before each addition;
	 * overflow is discarded.
	 */
	for (u_char *p = buf; p < &buf[len]; ++p) {
		if (thecrc & 1)
			thecrc |= 0x10000;
		thecrc = ((thecrc >> 1) + *p) & 0xffff;
	}

	return (thecrc);
}

/*
 * Copyright (c) 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * compress.c - File compression ala IEEE Computer, June 1984.
 *
 * Authors:	Spencer W. Thomas	(decvax!utah-cs!thomas)
 *		Jim McKie		(decvax!mcvax!jim)
 *		Steve Davies		(decvax!vax135!petsd!peora!srd)
 *		Ken Turkowski		(decvax!decwrl!turtlevax!ken)
 *		James A. Woods		(decvax!ihnp4!ames!jaw)
 *		Joe Orost		(decvax!vax135!petsd!joe)
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <signal.h>
#include <utime.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define BITS	16
#define HSIZE	69001		/* 95% occupancy */

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef long int	code_int;
typedef long int	count_int;

static const unsigned char magic_header[] = { "\037\235" };	/* 1F 9D */

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80
/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
*/

#define INIT_BITS 9			/* initial number of bits/code */

static int n_bits;			/* number of bits/code */
static int maxbits = BITS;		/* user settable max # bits/code */
static code_int maxcode;		/* maximum code, given n_bits */
static code_int maxmaxcode = 1 << BITS;	/* should NEVER generate this code */
#define MAXCODE(n_bits)	((1 << (n_bits)) - 1)

static count_int htab [HSIZE];
static unsigned short codetab [HSIZE];

#define codetabof(i)	codetab[i]

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i)	codetabof(i)
#define tab_suffixof(i)	((unsigned char *)(htab))[i]
#define de_stack		((unsigned char *)&tab_suffixof(1<<BITS))

static code_int free_ent = 0;			/* first unused entry */

static code_int getcode(FILE *);

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int block_compress = BLOCK_MASK;
static int clear_flg = 0;

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

/*-
 * Algorithm from "A Technique for High Performance Data Compression",
 * Terry A. Welch, IEEE Computer Vol 17, No 6 (June 1984), pp 8-19.
 *
 * Algorithm:
 * 	Modified Lempel-Ziv method (LZW).  Basically finds common
 * substrings and replaces them with a variable size code.  This is
 * deterministic, and can be done on the fly.  Thus, the decompression
 * procedure needs no input table, but tracks the way the table was built.
 */

static const unsigned char rmask[9] = {
	0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};

#ifndef BUFFER_SIZE
#define BUFFER_SIZE	16384
#endif

/* maximum number of bytes to return from getcode(). if 0, no limit. */
static uint32_t getcode_maxreadbytes;

/* number of bytes getcode has returned since decompress() was called */
static uint32_t getcode_readbytes;

/* this shouldn't be here, but it's faster to just do it on the fly */
static int output_checksum;

/*
 * Decompress 'input' to 'output'.  This routine adapts to the codes in
 * the file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  See the definitions above.
 *
 * Returns the number of bytes written to 'output'.
 */
static uint32_t
do_decompression(FILE *input, FILE *output) {
    unsigned char *stackp;
    int finchar;
    code_int code, oldcode, incode;
    int    n, nwritten, offset;		/* Variables for buffered write */
    char buff[BUFFER_SIZE];		/* Buffer for buffered write */
    uint32_t byteswritten;

    byteswritten = 0;

    /*
     * As above, initialize the first 256 entries in the table.
     */
    maxcode = MAXCODE(n_bits = INIT_BITS);
    for ( code = 255; code >= 0; code-- ) {
	tab_prefixof(code) = 0;
	tab_suffixof(code) = (unsigned char)code;
    }
    free_ent = ((block_compress) ? FIRST : 256 );

    finchar = oldcode = getcode(input);
    if(oldcode == -1)	/* EOF already? */
	return (0);

    /* first code must be 8 bits = char */
    n=0;
    buff[n++] = (char)finchar;

    stackp = de_stack;

    while ( (code = getcode(input)) > -1 ) {

	if ( (code == CLEAR) && block_compress ) {
	    for ( code = 255; code >= 0; code-- )
		tab_prefixof(code) = 0;
	    clear_flg = 1;
	    free_ent = FIRST - 1;
	    if ( (code = getcode(input)) == -1 )	/* O, untimely death! */
		break;
	}
	incode = code;
	/*
	 * Special case for KwKwK string.
	 */
	if ( code >= free_ent ) {
            *stackp++ = finchar;
	    code = oldcode;
	}

	/*
	 * Generate output characters in reverse order
	 */
	while ( code >= 256 ) {
	    *stackp++ = tab_suffixof(code);
	    code = tab_prefixof(code);
	}
	*stackp++ = finchar = tab_suffixof(code);

	/*
	 * And put them out in forward order
	 */
	do {
	    /*
	     * About 60% of the time is spent in the putchar() call
	     * that appeared here.  It was originally
	     *		putchar ( *--stackp );
	     * If we buffer the writes ourselves, we can go faster (about
	     * 30%).
	     *
	     * At this point, the next line is the next *big* time
	     * sink in the code.  It takes up about 10% of the time.
	     */
	     buff[n++] = *--stackp;
	     if (n == BUFFER_SIZE) {
		 offset = 0;
		 do {
		     output_checksum = sum1(output_checksum,
			 (u_char *)&buff[offset], n);
		     if (output == NULL)
			nwritten = n;
		     else
			nwritten = write(fileno(output), &buff[offset], n);
		     if (nwritten < 0)
			 return (0);
		     offset += nwritten;
		     byteswritten += nwritten;
		 } while ((n -= nwritten) > 0);
	      }
	} while ( stackp > de_stack );

	/*
	 * Generate the new entry.
	 */
	if ( (code=free_ent) < maxmaxcode ) {
	    tab_prefixof(code) = (unsigned short)oldcode;
	    tab_suffixof(code) = finchar;
	    free_ent = code+1;
	} 
	/*
	 * Remember previous code.
	 */
	oldcode = incode;
    }
    /*
     * Flush the stuff remaining in our buffer...
     */
    offset = 0;
    while (n > 0) {
	output_checksum = sum1(output_checksum, (u_char *)&buff[offset], n);
	if (output == NULL)
	    nwritten = n;
	else
	    nwritten = write(fileno(output), &buff[offset], n);
	if (nwritten < 0)
	    return (0);
	offset += nwritten;
	n -= nwritten;
	byteswritten += nwritten;
    }

    return (byteswritten);
}

/*-
 * Read one code from the standard input.  If EOF, return -1.
 * Inputs:
 *  	FILE stream.
 * Outputs:
 * 	code or -1 is returned.
 */
static code_int
getcode(FILE *input)
{
    code_int code;
    static int offset = 0, size = 0;
    static unsigned char buf[BITS];
    int r_off, bits;
    size_t readbytes;
    unsigned char *bp = buf;

    if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 *
	 * Bail immediately if we've read in our maximum.
	 */
	if (getcode_maxreadbytes != 0 &&
	    getcode_readbytes == getcode_maxreadbytes)
		return (-1);


	if ( free_ent > maxcode ) {
	    n_bits++;
	    if ( n_bits == maxbits )
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    else
		maxcode = MAXCODE(n_bits);
	}
	if ( clear_flg > 0) {
    	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	}

	if (getcode_maxreadbytes != 0 &&
	    n_bits > (getcode_maxreadbytes - getcode_readbytes))
		readbytes = getcode_maxreadbytes - getcode_readbytes;
	else
		readbytes = n_bits;

	size = fread( buf, 1, readbytes, input );
	if ( size <= 0 )
	    return -1;			/* end of file */
	getcode_readbytes += size;
	offset = 0;
	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
    }
    r_off = offset;
    bits = n_bits;
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/* Get first part (low order bits) */
	code = (*bp++ >> r_off);
	bits -= (8 - r_off);
	r_off = 8 - r_off;		/* now, offset into code word */
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
	    code |= *bp++ << r_off;
	    r_off += 8;
	    bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
    offset += n_bits;

    return code;
}

/*
 * Decompress from stream 'input' into stream 'output', reading at most
 * 'maxinput' bytes from 'input'.
 *
 * The special value 0 for maxinput indicates no limit.
 *
 * Returns the number of bytes outputted.
 */
uint32_t
decompress(FILE *input, FILE *output, uint32_t maxinput, int *cksum)
{
	uint32_t ret;

	if (maxinput != 0 && maxinput < 4)
		return (0);

	if ((getc(input) & 0xff) != magic_header[0] ||
	    (getc(input) & 0xff) != magic_header[1])
		return (0);

	clear_flg = 0;
	maxbits = getc(input);
	block_compress = maxbits & BLOCK_MASK;
	maxbits &= BIT_MASK;
	maxmaxcode = 1 << maxbits;
	if (maxbits > BITS)
		return (0);

	getcode_maxreadbytes = (maxinput == 0) ? 0 : maxinput - 3;
	getcode_readbytes = 0;
	output_checksum = 0;
	ret = do_decompression(input, output);
	*cksum = output_checksum;

	return (ret);
}
