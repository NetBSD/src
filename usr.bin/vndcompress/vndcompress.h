/* $Id: vndcompress.h,v 1.1.1.1 2005/07/25 12:17:59 hubertf Exp $ */

/*
 * Copyright (c) 2005 by Florian Stoehr
 * All rights reserved.
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
 *        This product includes software developed by Florian Stoehr
 * 4. The name of Florian Stoehr may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FLORIAN STOEHR ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
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
  * cloop2: File format
  * 
  * All numbers are in network byte order!
  *
  * Offset  Size Description
  * 0       128  Preamble (actually Linux load instructions)
  * 128     4    Block size
  * 131     4    Number of blocks
  * 135     8*x  64-bit compressed block start offsets (rel. to FILE (!))
  * 135+8*x y    Compressed blocks
  */
#ifndef __CLCONFIG_H__
#define __CLCONFIG_H__

#include <machine/bswap.h>

struct cloop_header;

void usage(void);
void vndcompress(const char *, const char *, uint32_t);
uint64_t * readheader(int, struct cloop_header *, off_t *);
void vnduncompress(const char *, const char *);

/* Size of the shell command (Linux compatibility) */
#define CLOOP_SH_SIZE 128

/* Atomic BSD block is always 512 bytes */
#define ATOMBLOCK 512

/* The default blocksize (compressed chunk) is 64 KB */
#define DEF_BLOCKSIZE 128*ATOMBLOCK

/*
 * Convert a 64-bit value between network and local byte order
 */
#if (_BYTE_ORDER == _BIG_ENDIAN)
#define SWAPPER(arg) (arg)
#else
#define SWAPPER(arg) bswap64(arg)
#endif

struct cloop_header {
	char sh[CLOOP_SH_SIZE];
	uint32_t block_size;
	uint32_t num_blocks;
};

#endif
