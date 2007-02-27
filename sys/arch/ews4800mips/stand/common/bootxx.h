/*	$NetBSD: bootxx.h,v 1.1.28.1 2007/02/27 16:50:26 yamt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifdef _STANDALONE
#include <lib/libkern/libkern.h>
#endif

enum {
	BERR_NONE,
	BERR_PDINFO,
	BERR_RDVTOC,
	BERR_VTOC,
	BERR_NOBFS,
	BERR_RDINODE,
	BERR_NOROOT,
	BERR_RDDIRENT,
	BERR_NOFILE,
	BERR_RDFILE,
	BERR_FILEMAGIC,
	BERR_AOUTHDR,
	BERR_ELFHDR,
	BERR_TARHDR,
	BERR_TARMAGIC,
};
int dk_read(int, int, void *);
int fileread(const char *, size_t *);

#ifdef _STANDALONE
#define	BASSERT(x)		((void)0)
#define	DPRINTF(arg...)		((void)0)
#define	SDBOOT_LOADADDR		0xa0190000
#define	SDBOOT_PDINFOADDR	0xa0191000
#define	SDBOOT_VTOCADDR		0xa0191400
#define	SDBOOT_INODEADDR	0xa0191600
#define	SDBOOT_DIRENTADDR	0xa0191800
#define	SDBOOT_SCRATCHADDR	0xa0200000
#else
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define	BASSERT(x)		assert(x)
#define	DPRINTF(fmt, args...)	printf(fmt, ##args)
uint8_t __buf0[512];
uint8_t __buf1[512];
uint8_t __buf2[512];
uint8_t __buf3[512];
uint8_t __buf4[4 * 1024 * 1024];
#define	SDBOOT_PDINFOADDR	__buf0
#define	SDBOOT_VTOCADDR		__buf1
#define	SDBOOT_INODEADDR	__buf2
#define	SDBOOT_DIRENTADDR	__buf3
#define	SDBOOT_SCRATCHADDR	__buf4
void sector_init(const char *, int);
#endif
