/*	$NetBSD: bootblock.h,v 1.1 2002/05/06 13:32:19 lukem Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifdef _KERNEL
#include <sys/stdint.h>
#else
#include <stdint.h>
#endif

/*
 * sparc
 */

#define SPARC_BOOT_BLOCK_OFFSET		512
#define SPARC_BOOT_BLOCK_BLOCKSIZE	512
#define SPARC_BOOT_BLOCK_MAX_SIZE	(512 * 15)

	/* Maximum # of 8KB blocks, enough for a 2MB boot program */
#define	SPARC_BBINFO_MAXBLOCKS		256

	/* Magic string -- 32 bytes long (including the NUL) */
#define	SPARC_BBINFO_MAGIC		"NetBSD/sparc bootxx            "
#define	SPARC_BBINFO_MAGICSIZE		sizeof(SPARC_BBINFO_MAGIC)

struct sparc_bbinfo {
	uint8_t bbi_magic[SPARC_BBINFO_MAGICSIZE];
	int32_t bbi_block_size;
	int32_t bbi_block_count;
	int32_t bbi_block_table[SPARC_BBINFO_MAXBLOCKS];
};


/*
 * sun68k (sun2, sun3)
 */

#define SUN68K_BOOT_BLOCK_OFFSET	512
#define SUN68K_BOOT_BLOCK_BLOCKSIZE	512
#define SUN68K_BOOT_BLOCK_MAX_SIZE	(512 * 15)

	/* Maximum # of 512 byte blocks, enough for a 32K boot program */
#define	SUN68K_BBINFO_MAXBLOCKS		64

	/* Magic string -- 32 bytes long (including the NUL) */
#define	SUN68K_BBINFO_MAGIC		"NetBSD/sun68k bootxx           "
#define	SUN68K_BBINFO_MAGICSIZE		sizeof(SUN68K_BBINFO_MAGIC)

struct sun68k_bbinfo {
	uint8_t bbi_magic[SUN68K_BBINFO_MAGICSIZE];
	int32_t bbi_block_size;
	int32_t bbi_block_count;
	int32_t bbi_block_table[SUN68K_BBINFO_MAXBLOCKS];
};
