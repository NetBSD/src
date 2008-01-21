/*	$NetBSD: disk.c,v 1.1.18.5 2008/01/21 09:36:24 yamt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
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

int dkopen(struct open_file *, ...);
int dkclose(struct open_file *);
int dkstrategy(void *, int, daddr_t, size_t, void *, size_t *);

struct devsw dkdevsw = {
	"dk", dkstrategy, dkopen, dkclose, noioctl
};

struct disk {
	bool active;
	int type;	/* FD/HD */
	int unit;
	int format;	/* 2D/2HD */
	int partition;
	int offset;
	int (*rw)(uint8_t *, int, int, int);
} __disk;

void sector_init(void);
bool __sector_rw(uint8_t *, int, int, int);
int __hd_rw(uint8_t *, int, int, int);
int __fd_2d_rw(uint8_t *, int, int, int);
int __fd_2hd_rw(uint8_t *, int, int, int);
void __fd_progress_msg(int);

bool
device_attach(int type, int unit, int partition)
{

	/* Inquire boot device type and unit from NVSRAM */
	boot_device(&__disk.type, &__disk.unit, &__disk.format);

	if (type >= 0)
		__disk.type = type;
	if (unit >= 0)
		__disk.unit = unit;

	__disk.partition = partition;

	__disk.active = true;
	__disk.offset = 0;

	if (partition >= 0) {
		if (!find_partition_start(__disk.partition, &__disk.offset)) {
			printf("type %d, unit %d partition %d not found.\n",
			    __disk.type, __disk.unit, __disk.partition);
			return false;
		}
	}
	DEVICE_CAPABILITY.active_device = type;

	/* Set actual read/write routine */
	if (__disk.type == NVSRAM_BOOTDEV_HARDDISK) {
		__disk.rw = __hd_rw;
	} else if (__disk.type == NVSRAM_BOOTDEV_FLOPPYDISK) {
		if (__disk.format == FD_FORMAT_2HD) {
			__disk.rw = __fd_2hd_rw;
		} else if (__disk.format == FD_FORMAT_2D) {
			__disk.rw = __fd_2d_rw;
		} else {
			printf("unknown floppy disk format %d.\n",
			    __disk.format);
			return false;
		}
	} else {
		printf("unknown disk type %d.\n", __disk.type);
		return false;
	}

	return true;
}

int
dkopen(struct open_file *f, ...)
{

	return 0;
}

int
dkclose(struct open_file *f)
{

	return 0;
}

int
dkstrategy(void *devdata, int rw, daddr_t blk, size_t size, void *buf,
    size_t *rsize)
{
	int n;

	if ((int)size < 0) {
		printf("%s: invalid request block %d size %d base %d\n",
		    __func__, blk, size, __disk.offset);
		return -1;
	}

	n = ROUND_SECTOR(size) >> DEV_BSHIFT;
	if (!sector_read_n(0, buf, __disk.offset + blk, n))
		return -1;

	*rsize = size;

	return 0;
}

void
sector_init(void)
{

	if (!__disk.active)
		device_attach(-1, -1, -1);
}

void
sector_fini(void *self)
{

	__disk.active = false;
}

bool
sector_read_n(void *self, uint8_t *buf, int sector, int count)
{

	if (!__sector_rw(buf, sector, 0, count))
		return false;
	return true;
}

bool
sector_read(void *self, uint8_t *buf, int sector)
{

	return __sector_rw(buf, sector, 0, 1);
}

bool
sector_write_n(void *self, uint8_t *buf, int sector, int count)
{

	if (!__sector_rw(buf, sector, 0x1000, count))
		return false;
	return true;
}

bool
sector_write(void *self, uint8_t *buf, int sector)
{

	return __sector_rw(buf, sector, 0x1000, 1);
}

bool
__sector_rw(uint8_t *buf, int block, int flag, int count)
{
	int err;

	if (!__disk.active)
		sector_init();

	if ((err = __disk.rw(buf, block, flag, count)) != 0)
		printf("%s: type=%d unit=%d offset=%d block=%d err=%d\n",
		    __func__, __disk.type, __disk.unit, __disk.offset,
		    block, err);

	return err == 0;
}

int
__hd_rw(uint8_t *buf, int block, int flag, int count)
{

	return (ROM_DK_RW(flag | __disk.unit, block, count, buf) & 0x7f);
}

int
__fd_2d_rw(uint8_t *buf, int block, int flag, int count)
{
	int cnt, i, err;
	uint32_t pos;

	if (!blk_to_2d_position(block, &pos, &cnt)) {
		printf("%s: invalid block #%d.\n", __func__, block);
		return -1;
	}
	__fd_progress_msg(pos);

	for (i = 0; i < count; i++, buf += DEV_BSIZE) {
		err = ROM_FD_RW(flag | __disk.unit, pos, cnt, buf);
		if (err)
			return err;
	}
	return 0;
}

int
__fd_2hd_rw(uint8_t *buf, int block, int flag, int count)
{
	int cnt, i, err;
	uint32_t pos;

	if (!blk_to_2hd_position(block, &pos, &cnt)) {
		printf("%s: invalid block #%d.\n", __func__, block);
		return -1;
	}
	__fd_progress_msg(pos);

	for (i = 0; i < count; i++, buf += DEV_BSIZE) {
		err = ROM_FD_RW(flag | __disk.unit | 0x1000000, pos, cnt, buf);
		if (err)
			return err;
	}
	return 0;
}

void
__fd_progress_msg(int pos)
{
	char msg[16];

	memset(msg, 0, sizeof msg);
	sprintf(msg, "C%d H%d S%d\r", (pos >> 16) & 0xff, (pos >> 8) & 0xff,
	    pos & 0xff);
	printf("%s", msg);
}
