/*	$NetBSD: bootxx.c,v 1.11 2014/03/29 05:07:25 ozaki-r Exp $	*/

/*-
 * Copyright (c) 1999 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <machine/romcall.h>

#include <sys/bootblock.h>

void bootxx(uint32_t, uint32_t, uint32_t, uint32_t);

struct shared_bbinfo bbinfo = {
	{ NEWS68K_BBINFO_MAGIC },	/* bbi_magic[] */
	0,				/* bbi_block_size */
	SHARED_BBINFO_MAXBLOCKS,	/* bbi_block_count */
	{ 0 },				/* bbi_block_table[] */
};

#ifndef DEFAULT_ENTRY_POINT
#define DEFAULT_ENTRY_POINT	0x003e0000
#endif
void (*entry_point)(uint32_t, uint32_t, uint32_t, uint32_t) =
    (void *)DEFAULT_ENTRY_POINT;

#ifdef BOOTXX_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

char *devs[] = { "hd", "fh", "fd", NULL, NULL, "rd", "st" };

void
bootxx(uint32_t d4, uint32_t d5, uint32_t d6, uint32_t d7)
{
	int fd, blk, bs;
	int ctlr, unit, part, type;
	int i;
	int bootdev = d6;
	char *addr;
	char devname[32];

	printf("NetBSD/news68k Primary Boot\n");

	DPRINTF("\n");
	DPRINTF("d4 %x\n", d4);
	DPRINTF("d5 %x (%s)\n", d5, (char *)d5);
	DPRINTF("d6 %x\n", d6);
	DPRINTF("d7 %x\n", d7);

	DPRINTF("block_size  = %d\n", bbinfo.bbi_block_size);
	DPRINTF("block_count = %d\n", bbinfo.bbi_block_count);
	DPRINTF("entry_point = %p\n", entry_point);

	/* sd(ctlr, lun, part, bus?, host) */

	ctlr = BOOTDEV_CTLR(bootdev);
	unit = BOOTDEV_UNIT(bootdev);
	part = BOOTDEV_PART(bootdev);
	type = BOOTDEV_TYPE(bootdev);

	if (devs[type] == NULL) {
		printf("unknown bootdev (0x%x)\n", bootdev);
		return;
	}

	snprintf(devname, sizeof(devname), "%s(%d,%d,%d)",
	    devs[type], ctlr, unit, part);

	fd = rom_open(devname, 0);
	if (fd == -1) {
		printf("cannot open %s\n", devname);
		return;
	}

	addr = (char *)entry_point;
	bs = bbinfo.bbi_block_size;
	DPRINTF("reading block:");
	for (i = 0; i < bbinfo.bbi_block_count; i++) {
		blk = bbinfo.bbi_block_table[i];

		DPRINTF(" %d", blk);

		rom_lseek(fd, blk * 512, 0);
		rom_read(fd, addr, bs);
		addr += bs;
	}
	DPRINTF(" done\n");
	rom_close(fd);

	(*entry_point)(d4, d5, d6, d7);
	DPRINTF("bootxx returned?\n");
}
