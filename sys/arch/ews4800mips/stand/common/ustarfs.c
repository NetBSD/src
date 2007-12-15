/*	$NetBSD: ustarfs.c,v 1.6 2007/12/15 00:39:17 perry Exp $	*/

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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/param.h>
#include <machine/sbd.h>
#include <machine/sector.h>

#include "local.h"
#include "common.h"

bool __ustarfs_file(int, char *, size_t *);
bool __block_read(uint8_t *, int);
bool __block_read_n(uint8_t *, int, int);
void __change_volume(int);

enum { USTAR_BLOCK_SIZE = 8192 };/* Check src/distrib/common/buildfloppies.sh */
struct volume {
	int max_block;
	int current_volume;
	int block_offset;
} __volume;

bool
ustarfs_load(const char *file, void **addrp, size_t *sizep)
{
	char fname[16];
	int block = 16;
	size_t sz;
	int maxblk;

	if (DEVICE_CAPABILITY.active_device == NVSRAM_BOOTDEV_HARDDISK)
		maxblk = 0x1fffffff;	/* no limit */
	else if (DEVICE_CAPABILITY.active_device == NVSRAM_BOOTDEV_FLOPPYDISK)
		/*
		 * Although phisical format isn't 2D, volume size is
		 * limited to size of 2D.
		 */
		maxblk = (77 + 76) * 13;
	else {
		printf("not supported device.\n");
		return false;
	}

	/* Truncate to ustar block boundary */
	__volume.max_block = (maxblk / (USTAR_BLOCK_SIZE >> DEV_BSHIFT)) *
	    (USTAR_BLOCK_SIZE >> DEV_BSHIFT);
	__volume.block_offset = 0;
	__volume.current_volume = 0;

	/* Find file */
	while (/*CONSTCOND*/1) {
		if (!__ustarfs_file(block, fname, &sz))
			return false;

		if (strcmp(file, fname) == 0)
			break;
		block += (ROUND_SECTOR(sz) >> DEV_BSHIFT) + 1;
	}
	block++;	/* skip tar header */
	*sizep = sz;

	/* Load file */
	sz = ROUND_SECTOR(sz);
	if ((*addrp = alloc(sz)) == 0) {
		printf("%s: can't allocate memory.\n", __func__);
		return false;
	}

	if (!__block_read_n(*addrp, block, sz >> DEV_BSHIFT)) {
		printf("%s: can't load file.\n", __func__);
		dealloc(*addrp, sz);
		return false;
	}

	return true;
}

bool
__ustarfs_file(int start_block, char *file, size_t *size)
{
	uint8_t buf[512];

	if (!__block_read(buf, start_block)) {
		printf("can't read tar header.\n");
		return false;
	}
	if (((*(uint32_t *)(buf + 256)) & 0xffffff) != 0x757374) {
		printf("bad tar magic.\n");
		return false;
	}
	*size = strtoul((char *)buf + 124, 0, 0);
	strncpy(file, (char *)buf, 16);

	return true;
}

bool
__block_read_n(uint8_t *buf, int blk, int count)
{
	int i;

	for (i = 0; i < count; i++, buf += DEV_BSIZE)
		if (!__block_read(buf, blk + i))
			return false;

	return true;
}

bool
__block_read(uint8_t *buf, int blk)
{
	int vol;

	if ((vol = blk / __volume.max_block) != __volume.current_volume) {
		__change_volume(vol);
		__volume.current_volume = vol;
		/* volume offset + ustarfs header (8k) */
		__volume.block_offset = vol * __volume.max_block - 16;
	}

	return sector_read(0, buf, blk - __volume.block_offset);
}

void
__change_volume(int volume)
{
	uint8_t buf[512];
	int i;

	while (/*CONSTCOND*/1) {
		printf("insert disk %d, and press return...", volume + 1);
		while (getchar() != '\r')
			;
		printf("\n");
		if (!sector_read(0, buf, 0))
			continue;
		if (*(uint32_t *)buf != 0x55535441) {	/* "USTAR" */
			printf("invalid magic.\n");
			continue;
		}
		if ((i = buf[8] - '0') != volume + 1) {
			printf("invalid volume number. disk=%d requested=%d\n",
			    i, volume + 1);
			continue;
		}
		return;
	}
}
